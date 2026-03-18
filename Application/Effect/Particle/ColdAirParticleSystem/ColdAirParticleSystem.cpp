#include "ColdAirParticleSystem.h"
#include "../../../Scene/BaseScene/BaseScene.h"
#include "../Src/Framework/Component/CameraComponent/ICameraComponent/ICameraComponent.h"

ColdAirParticleSystem::ColdAirParticleSystem(BaseScene* pOwnerScene, int maxParticles)
	: ParticleSystem(pOwnerScene, maxParticles)
{
}

ColdAirParticleSystem::~ColdAirParticleSystem()
{
}

void ColdAirParticleSystem::Draw()
{
	//カメラ情報の取得
	auto iCamera = m_pOwnerScene->GetActiveCamera();
	if (!iCamera) return;
	const auto& camera = iCamera->GetCamera();
	if (!camera) return;

	//ビルボード行列の計算
	//カメラのビュー行列から回転成分のみを取り出し、逆行列にすることで
	//「常にカメラの方を向く」回転行列を作成します
	Math::Matrix viewMat = camera->GetCameraViewMatrix();
	Math::Matrix billMat = viewMat;
	billMat.Translation(Math::Vector3::Zero); // 位置情報をリセット
	billMat.Invert();

	//描画ステート設定
	KdShaderManager::Instance().ChangeBlendState(KdBlendState::Alpha);
	KdShaderManager::Instance().ChangeRasterizerState(KdRasterizerState::CullNone);

	//光る表現（高輝度抽出）のためBrightシェーダーを開始
	KdShaderManager::Instance().m_postProcessShader.BeginBright();
	{
		for (auto& p : m_particles)
		{
			if (p.lifetime <= 0.0f) continue;

			//ワールド行列の合成
			//1. 拡大縮小 (Scale)
			//2. 回転 (Billboard: カメラ対面)
			//3. 平行移動 (Translation)
			Math::Matrix scaleMat = Math::Matrix::CreateScale({ p.size, p.size, p.size });
			Math::Matrix transMat = Math::Matrix::CreateTranslation(p.pos);
			Math::Matrix worldMat = scaleMat * billMat * transMat;

			//フェードアウト計算
			//寿命が尽きるにつれてアルファ値を0に近づける
			float lifeRatio = std::max(0.0f, p.lifetime / p.maxLifetime);
			Math::Color drawColor = p.color;
			drawColor.A(p.color.A() * lifeRatio);

			KdShaderManager::Instance().m_StandardShader.DrawModel(*m_model, worldMat, drawColor);
		}
	}
	KdShaderManager::Instance().m_postProcessShader.EndBright();

	KdShaderManager::Instance().UndoBlendState();
	KdShaderManager::Instance().UndoRasterizerState();
}

void ColdAirParticleSystem::LoadAssets()
{
	m_model = KdAssets::Instance().m_modeldatas.GetData("Asset/Models/Particle/tyou.gltf");
}

void ColdAirParticleSystem::InitializeParticle(Particle& p, const Math::Vector3& pos, const Math::Vector3& baseDirection, float spread)
{
	p.pos = pos;

	//進行方向のランダム化
	//基準となる方向(baseDirection)に対し、XYZ各軸にランダムな値を加算して拡散させる
	Math::Vector3 dir = baseDirection;
	dir.x += KdRandom::GetFloat(-spread, spread);
	dir.y += KdRandom::GetFloat(-spread, spread);
	dir.z += KdRandom::GetFloat(-spread, spread);
	dir.Normalize();

	//速度決定（Y軸に少し持ち上げる力を加えてふわっとさせる）
	p.velocity = dir * KdRandom::GetFloat(0.3f, 0.8f);
	p.velocity.y += 0.5f;

	p.color = { 1.0f, 1.0f, 1.0f, 0.5f };
	p.lifetime = KdRandom::GetFloat(1.5f, 2.5f);
	p.maxLifetime = p.lifetime;
	p.size = KdRandom::GetFloat(0.1f, 0.3f);
	p.rot = Math::Vector3::Zero;
	p.angularVelocity = Math::Vector3::Zero;
}

void ColdAirParticleSystem::UpdateParticle(Particle& p, float deltatime)
{
	//物理挙動の更新
	//1. 重力を加算（下方向への加速）
	p.velocity.y += m_gravity * deltatime;

	//2. 空気抵抗の適用（現在の速度に減衰率を掛けて徐々に減速させる）
	p.velocity *= (1.0f - m_damping * deltatime);

	//3. 速度から座標を更新
	p.pos += p.velocity * deltatime;
}