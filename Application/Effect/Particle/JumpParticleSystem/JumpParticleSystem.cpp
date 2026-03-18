#include "JumpParticleSystem.h"
#include"../../../Scene/BaseScene/BaseScene.h"
#include"../Src/Framework/Component/CameraComponent/ICameraComponent/ICameraComponent.h"

JumpParticleSystem::JumpParticleSystem(BaseScene* pOwnerScene, int maxParticles)
	: ParticleSystem(pOwnerScene, maxParticles)
{
}

JumpParticleSystem::~JumpParticleSystem()
{
}

void JumpParticleSystem::Draw()
{
	//カメラ情報取得
	auto iCamera = m_pOwnerScene->GetActiveCamera();
	if (!iCamera)return;
	const auto& camera = iCamera->GetCamera();
	if (!camera)return;

	//ビルボード行列の計算。カメラのビュー行列から回転成分のみを取り出し、逆行列にすることで常にカメラの方を向く回転行列を作成
	Math::Matrix viewMat = camera->GetCameraViewMatrix();
	Math::Matrix billMat = viewMat;
	billMat.Translation(Math::Vector3::Zero);
	billMat.Invert();

	//描画設定
	KdShaderManager::Instance().ChangeBlendState(KdBlendState::Alpha);
	KdShaderManager::Instance().ChangeRasterizerState(KdRasterizerState::CullNone);


	KdShaderManager::Instance().m_postProcessShader.BeginBright();
	{
		for (auto& p : m_particles)
		{
			if (p.lifetime <= 0.0f) continue;

			//ワールド行列の合成(拡大縮小->回転->平行移動)
			Math::Matrix scaleMat = Math::Matrix::CreateScale({ p.size,p.size,p.size });
			Math::Matrix transMat = Math::Matrix::CreateTranslation(p.pos);
			Math::Matrix worldMat = scaleMat * billMat * transMat;

			//寿命の割合を計算しフェードアウト
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

void JumpParticleSystem::LoadAssets()
{
	m_model = KdAssets::Instance().m_modeldatas.GetData("Asset/Models/Particle/tyou.gltf");
}

void JumpParticleSystem::InitializeParticle(Particle& p, const Math::Vector3& pos, const Math::Vector3& baseDirection, float spread)
{
	p.pos = pos;

	//基準方向に対してランダムな拡散(Spread)を加える
	Math::Vector3 dir = baseDirection;
	dir.x += KdRandom::GetFloat(-spread, spread);
	dir.y += KdRandom::GetFloat(-spread, spread);
	dir.z += KdRandom::GetFloat(-spread, spread);
	dir.Normalize();

	//ジャンプの衝撃を表現するため、比較的高速な初速を与える
	p.velocity = dir * KdRandom::GetFloat(8.0f, 12.0f);
	p.color = { 0.5f, 0.9f, 1.0f, 1.0f };//水色
	p.lifetime = KdRandom::GetFloat(0.3f, 0.6f);
	p.maxLifetime = p.lifetime;
	p.size = KdRandom::GetFloat(0.1f, 0.2f);
	p.initialSize = p.size;
	p.rot = Math::Vector3::Zero;
	p.angularVelocity = Math::Vector3::Zero;
}

void JumpParticleSystem::UpdateParticle(Particle& p, float deltatime)
{
	//重力の加算と空気抵抗による減速処理
	p.velocity.y += m_gravity * deltatime;
	p.velocity *= (1.0f - m_damping * deltatime);
	p.pos += p.velocity * deltatime;

	//寿命に応じてサイズを縮小させる(発生時は大きく、消えるときは小さく)
	float lifeRatio = std::max(0.0f, p.lifetime / p.maxLifetime);
	p.size = p.initialSize * lifeRatio;
}