#include "TPSCameraComponent.h"
#include"../Src/Application/SettingsManager/SettingsManager.h"
#include"../Src/Application/JsonHelper/JsonHelper.h"
#include"../Src/Application/main.h"
#include"../Src/Application/Scene/SceneManager.h"
#include"../../TransformComponent/TransformComponent.h"
#include"../Src/Application/System/PhysicsSystem.h"

void TPSCameraComponent::Awake()
{
	CameraComponent::Awake();

	// 設定ファイルからカメラパラメータを読み込み
	const auto& settings = SettingsManager::Instance().GetGameSetting();
	const auto& camSettings = settings["camera_settings"];
	m_sensitivity = JsonHelper::GetFloat(camSettings, "tps_sensitivity", 0.2f);
	float height = JsonHelper::GetFloat(camSettings, "tps_height", 3.0f);
	float distance = JsonHelper::GetFloat(camSettings, "tps_distance", -7.0f);

	//ターゲットからの相対的な初期座標
	m_localMat = Math::Matrix::CreateTranslation(0.0f, height, distance);

	//画面の中心を計算して保存
	RECT clientRect;
	GetClientRect(Application::Instance().GetWindowHandle(), &clientRect);
	m_fixMousePos.x = clientRect.right / 2;
	m_fixMousePos.y = clientRect.bottom / 2;
	//アプリケーションのクライアント座標をスクリーン座標に変換
	ClientToScreen(Application::Instance().GetWindowHandle(), &m_fixMousePos);
}

void TPSCameraComponent::Update()
{
	//CameraComponent::Update();

	//ゲームモードではない、またはターゲットがなければ何もしない
	if (SceneManager::Instance().GetCurrentMode() != SceneManager::SceneMode::Game || m_wpTarget.expired())return;

	//マウスの現在の座標を取得
	POINT nowMousePos;
	GetCursorPos(&nowMousePos);

	//前フレームからのマウスの移動量を計算
	float deltaX = static_cast<float>(nowMousePos.x - m_fixMousePos.x);
	float deltaY = static_cast<float>(nowMousePos.y - m_fixMousePos.y);

	//カーソルを画面中央に戻す
	SetCursorPos(m_fixMousePos.x, m_fixMousePos.y);

	//移動量をカメラの回転角度に変換
	const float sensitivity = 0.2f;
	m_yaw += DirectX::XMConvertToRadians(deltaX * sensitivity);
	m_pitch += DirectX::XMConvertToRadians(deltaY * sensitivity);

	//pitchに制限
	m_pitch = std::clamp(m_pitch, DirectX::XMConvertToRadians(-80.0f), DirectX::XMConvertToRadians(80.0f));
}

void TPSCameraComponent::PostUpdate()
{
	if (!m_transform) return;

	//強制移動
	if (m_isForceMoving)
	{
		m_forceMoveTimer += Application::Instance().GetDeltaTime();
		float t = std::min(m_forceMoveTimer / m_forceMoveDuration, 1.0f);

		//イージング(SmoothStep)で滑らかに補間
		t = t * t * (3.0f - 2.0f * t);

		Math::Vector3 currentPos = Math::Vector3::Lerp(m_forceMoveStartPos, m_forceMoveEndPos, t);
		m_transform->SetPos(currentPos);

		m_spCamera->SetCameraMatrix(m_transform->GetMatrix());

		if (t >= 1.0f)
		{
			m_isForceMoving = false;
			//強制移動終了後、即座に通常追従に戻るとカクつく場合があるので注意
		}

		//Y軸追従も強制的に移動
		if (t >= 1.0f) {
			m_isForceMoving = false;
			m_isFirstFrame = true; //次回ターゲット追従時に位置をリセットさせる
		}

		return;
	}

	auto spTarget = m_wpTarget.lock();

	//ターゲットが存在し、かつ自分自身ではないことを確認
	if (!spTarget || spTarget.get() == m_transform.get()) return;

	Math::Vector3 targetPos = spTarget->GetPos();

	if (m_isFirstFrame)//初回は瞬時に移動
	{
		m_currentTarget = targetPos;
		m_isFirstFrame = false;
	}
	else
	{
		//XZ軸は即座に追従
		m_currentTarget.z = targetPos.z;
		m_currentTarget.x = targetPos.x;

		//Y軸のみ線形補間(Lerp)で遅延させて追従し、カメラの上下揺れを軽減
		float lerpSpeed = 5.0f;
		float deltatime = Application::Instance().GetDeltaTime();
		float lerp = lerpSpeed * deltatime;

		m_currentTarget.y += (targetPos.y - m_currentTarget.y) * lerp;
	}

	//行列合成: ローカル座標(オフセット) * X回転 * Y回転 * ターゲット位置
	Math::Matrix targetMat = Math::Matrix::CreateTranslation(m_currentTarget);
	Math::Matrix rotY = Math::Matrix::CreateRotationY(m_yaw);
	Math::Matrix rotX = Math::Matrix::CreateRotationX(m_pitch);
	Math::Matrix finalMat = m_localMat * rotX * rotY * targetMat;

	//カメラがオブジェクトと衝突(壁めり込み)することを防ぐためのレイキャスト
	RayInfo ray;
	ray.m_start = targetMat.Translation() + Math::Vector3(0.0f, 1.0f, 0.0f);
	ray.m_dir = finalMat.Translation() - ray.m_start;

	float rayDistance = ray.m_dir.Length();
	if (rayDistance > 0.0f)
	{
		ray.m_maxDistance = rayDistance;
		ray.m_dir.Normalize();

		RayResult result;
		//ターゲットからカメラ位置へレイを飛ばし、障害物に当たったらカメラ位置を補正
		if (PhysicsSystem::Instance().Raycast(ray, result, LayerAll, spTarget->GetOwner()))
		{
			//ヒット位置より少し手前に配置
			finalMat.Translation(result.m_hitPos - ray.m_dir * 0.1f);
		}
	}

	//計算した最終行列から座標と回転を分解
	Math::Vector3 finalPos, dummyScale, finalRot;
	Math::Quaternion dummyQuat;
	finalMat.Decompose(dummyScale, dummyQuat, finalPos);

	finalRot = {
		DirectX::XMConvertToDegrees(m_pitch),
		DirectX::XMConvertToDegrees(m_yaw),
		0.0f
	};

	m_transform->SetPos(finalPos);
	m_transform->SetRot(finalRot);

	m_spCamera->SetCameraMatrix(m_transform->GetMatrix());

}

void TPSCameraComponent::SetTarget(const std::shared_ptr<TransformComponent>& target)
{
	if (target)
	{
		m_wpTarget = target;
		m_isFirstFrame = true;
	}
}

void TPSCameraComponent::SetForceMoveTarget(const Math::Vector3& targetPos, float duration)
{
	if (!m_transform)return;

	m_isForceMoving = true;
	m_forceMoveDuration = duration;
	m_forceMoveTimer = 0.0f;

	m_forceMoveStartPos = m_transform->GetPos();

	//カメラの理想位置を計算(移動先のplayerの座標+現在の相対距離)
	Math::Matrix targetMat = Math::Matrix::CreateTranslation(targetPos);
	Math::Matrix rotY = Math::Matrix::CreateRotationY(m_yaw);
	Math::Matrix rotX = Math::Matrix::CreateRotationX(m_pitch);
	Math::Matrix idealMat = m_localMat * rotX * rotY * targetMat;

	m_forceMoveEndPos = idealMat.Translation();
}