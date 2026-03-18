#include "PlayerInputComponent.h"
#include"../GameObject.h"
#include"../RigidbodyComponent/RigidbodyComponent.h"
#include"../../Command/CommandInvoker/CommandInvoker.h"
#include"../../Command/MoveCommand/MoveCommand.h"
#include"../GravityComponent/GravityComponent.h"
#include"../Src/Application/main.h"
#include"../Src/Application/Scene/SceneManager.h"
#include"../Src/Application/Scene/BaseScene/BaseScene.h"
#include"../CameraComponent/TPSCameraComponent/TPSCameraComponent.h"
#include"../TransformComponent/TransformComponent.h"
#include"../Src/Application/SettingsManager/SettingsManager.h"
#include"../Src/Application/JsonHelper/JsonHelper.h"
#include"../RenderComponent/RenderComponent.h"

void PlayerInputComponent::Awake()
{
	const auto& settings = SettingsManager::Instance().GetGameSetting()["player_settings"];
	m_maxSpeed = JsonHelper::GetFloat(settings, "max_speed", 5.0f);
	m_moveForce = JsonHelper::GetFloat(settings, "move_force", 15.0f);
	m_controlForce = JsonHelper::GetFloat(settings, "control_force", 15.0f);
	m_brakeForce = JsonHelper::GetFloat(settings, "brake_force", 20.0f);
	m_jumpPower = JsonHelper::GetFloat(settings, "jump_power", 800.0f);
	m_coyoteTimeDuration = JsonHelper::GetFloat(settings, "coyote_time", 0.1f);
}

void PlayerInputComponent::Start()
{
	//自身がアタッチされているGameObjectから必要なコンポーネントを取得
	m_rigid = m_owner->GetComponent<RigidbodyComponent>();
	m_gravity = m_owner->GetComponent<GravityComponent>();
	m_wpRender = m_owner->GetComponent<RenderComponent>();

	//初期アニメーション
	ChangeAnimation(AnimState::Walk);
}

void PlayerInputComponent::Update()
{
	if (!m_invoker || !m_rigid || !m_gravity)return;

	if (SceneManager::Instance().GetCurrentMode() != SceneManager::SceneMode::Game)return;

	AnimState nextState = m_currState;

	if (m_currState == AnimState::Action)
	{
		//アクションアニメーションが終わるとIdleへ
		if (m_animator.IsAnimationEnd())
		{
			nextState = AnimState::Idle;
		}

		if (nextState == AnimState::Action)
		{
			//地面にいるなら横移動はゼロ
			if (m_gravity->IsOnGround())
			{
				Math::Vector3 velocity = m_rigid->m_velocity;
				velocity.x = 0.0f;
				velocity.z = 0.0f;
				m_rigid->SetVelocity(velocity);
			}

			UpdateAnimation();
			return;
		}
	}

	//入力処理
	if (!m_inputEnabled)return;

	bool isOnGround = m_gravity->IsOnGround();
	float verticalVelocity = m_rigid->m_velocity.y;

	//--コヨーテタイム(空中に飛び出しても少しの間ジャンプできる猶予時間)の更新--
	if (isOnGround && verticalVelocity <= 0.0f)
	{
		//地面判定の間はカウンターを最大値にリセットし続ける
		m_coyoteTimeCounter = m_coyoteTimeDuration;
	}
	else
	{
		//空中にいる場合はカウンターを減らしていく
		if (m_coyoteTimeCounter > 0.0f)
		{
			float deltatime = Application::Instance().GetDeltaTime();
			m_coyoteTimeCounter -= deltatime;
		}
	}

	//カウンターが残っている＝擬似的に接地しているとみなしてジャンプ可能
	bool canJump = m_coyoteTimeCounter > 0.0f;

	//ジャンプ
	if (KdInputManager::Instance().IsPress("Jump"))
	{
		if (canJump)
		{
			m_rigid->m_velocity.y = 0.0f;
			m_rigid->m_isJumpRequested = true;//ジャンプリクエストで速度を直接上書きの予約
			m_rigid->m_jumpVelocity = { m_rigid->m_velocity.x,m_jumpPower,m_rigid->m_velocity.z };

			//ジャンプしたら即座にコヨーテタイムを消費(2段ジャンプ防止)
			m_coyoteTimeCounter = 0.0f;

			nextState = AnimState::Jump;
		}
	}

	//水平方向の移動入力
	Math::Vector3 moveDir = Math::Vector3::Zero;
	if (KdInputManager::Instance().IsHold("MoveForward"))
	{
		moveDir.z = 1.0f;
	}
	if (KdInputManager::Instance().IsHold("MoveBack"))
	{
		moveDir.z = -1.0f;
	}
	if (KdInputManager::Instance().IsHold("MoveLeft"))
	{
		moveDir.x = -1.0f;
	}
	if (KdInputManager::Instance().IsHold("MoveRight"))
	{
		moveDir.x = 1.0f;
	}

	Math::Vector3 currentVelocity = m_rigid->m_velocity;
	currentVelocity.y = 0;

	if (moveDir.LengthSquared() > 0)
	{
		//ジャンプ中でなければ歩き
		if (nextState != AnimState::Jump && isOnGround)
		{
			nextState = AnimState::Walk;
		}

		moveDir.Normalize();

		//カメラ基準の移動方向へ変換 
		if (auto spCam = m_wpCamera.lock())
		{
			//カメラのY軸回転(Yaw)のみを抽出した回転行列を作成
			Math::Matrix camRotY = Math::Matrix::CreateRotationY(spCam->GetYaw());

			//入力ベクトルを回転させて、カメラが見ている方向を基準とした移動ベクトルにする
			moveDir = Math::Vector3::TransformNormal(moveDir, camRotY);
		}

		//キャラクターの向き補正
		//atan2(x, z) でZ方向(奥)を0度とした時計回りの角度が求まる
		float targetAngle = DirectX::XMConvertToDegrees(atan2(moveDir.x, moveDir.z));
		targetAngle += 180.0f;

		//現在の角度を取得
		float	currentAngle = m_owner->GetComponent<TransformComponent>()->GetRot().y;

		//差分角度を計算
		float angDiff = targetAngle - currentAngle;

		//角度の正規化(-180～180の範囲に収めて最短回転ルートにする)
		while (angDiff > 180.0f) angDiff -= 360.0f;
		while (angDiff < -180.0f) angDiff += 360.0f;

		float rotSpeed = 10.0f;
		float deltaTime = Application::Instance().GetDeltaTime();

		//徐々に目標角度へ回転させる
		currentAngle += angDiff * std::min(rotSpeed * deltaTime, 1.0f);

		m_owner->GetComponent<TransformComponent>()->SetRot({ 0.0f, currentAngle, 0.0f });

		//目標速度に近づける力を計算(P制御のようなもの)
		float maxSpeed = 5.0f;
		Math::Vector3 targetVelocity = moveDir * maxSpeed;
		Math::Vector3 velocityDiff = targetVelocity - currentVelocity;

		float controlForce = 15.0f;
		if (m_rigid->m_isOnSlipperySurface)
		{
			//滑る床の上では制御力を弱める
			controlForce = 10.0f;
		}

		//移動コマンドを生成して実行を依頼(物理挙動へ力を加える)
		auto command = std::make_unique<MoveCommand>(m_rigid, velocityDiff * controlForce);
		m_invoker->ExecuteCommand(std::move(command));
	}
	else if (isOnGround && !m_rigid->m_isOnSlipperySurface)
	{
		//入力がなく、かつ滑る床でなければブレーキをかける
		auto command = std::make_unique<MoveCommand>(m_rigid, -currentVelocity * m_brakeForce);
		m_invoker->ExecuteCommand(std::move(command));

		//ジャンプ中でなければアイドル
		if (nextState != AnimState::Jump && isOnGround)
		{
			nextState = AnimState::Idle;
		}
	}
	else
	{
		//空中で入力なし
		if (!isOnGround)
		{
			if (m_currState != AnimState::Jump)
			{
				nextState = AnimState::Idle;
			}
		}
	}

	//着地判定
	if (m_currState == AnimState::Jump && isOnGround && verticalVelocity <= 0.0f)
	{
		//移動入力があればWalkなければIdleへ遷移
		if (moveDir.LengthSquared() > 0) nextState = AnimState::Walk;
		else nextState = AnimState::Idle;
	}

	//状態変更適用
	if (nextState != m_currState)
	{
		ChangeAnimation(nextState);
	}

	UpdateAnimation();
}

void PlayerInputComponent::SetInvoker(std::shared_ptr<CommandInvoker> invoker)
{
	m_invoker = invoker;
}

void PlayerInputComponent::PlayAction()
{
	ChangeAnimation(AnimState::Action);
}

void PlayerInputComponent::ChangeAnimation(AnimState nextState)
{
	m_currState = nextState;

	auto spRender = m_wpRender.lock();
	if (!spRender) return;
	auto spModel = spRender->GetModelWork();
	if (!spModel) return;

	std::shared_ptr<KdAnimationData> animData = nullptr;
	bool isLoop = true;

	switch (nextState)
	{
	case PlayerInputComponent::AnimState::Idle:
		animData = spModel->GetAnimation("Idle");
		isLoop = true;
		break;
	case PlayerInputComponent::AnimState::Walk:
		animData = spModel->GetAnimation("Walk");
		isLoop = true;
		break;
	case PlayerInputComponent::AnimState::Jump:
		animData = spModel->GetAnimation("Jump");
		isLoop = false;
		break;
	case PlayerInputComponent::AnimState::Action:
		animData = spModel->GetAnimation("Action");
		isLoop = false;
		break;
	}

	if (animData)
	{
		m_animator.SetAnimation(animData, isLoop);

		if (nextState == AnimState::Jump)
		{
			//ジャンプアニメーションは予備動作をスキップして、飛び上がる瞬間から再生する
			float skipFrame = 25.0f;
			m_animator.AdvanceTime(spModel->WorkNodes(), skipFrame);
		}
	}
}

void PlayerInputComponent::UpdateAnimation()
{
	auto spRender = m_wpRender.lock();
	if (!spRender) return;
	auto spModel = spRender->GetModelWork();
	if (!spModel) return;

	//アニメーション時間を進めてボーンを変形させる
	float playbackSpeed = 1.5f;
	m_animator.AdvanceTime(spModel->WorkNodes(), playbackSpeed);

	//変形後の行列計算(親子関係の反映)
	spModel->CalcNodeMatrices();
}