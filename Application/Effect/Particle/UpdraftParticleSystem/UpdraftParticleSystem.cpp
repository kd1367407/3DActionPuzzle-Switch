#include "UpdraftParticleSystem.h"
#include"../../../Scene/BaseScene/BaseScene.h"
#include"../Src/Framework/Component/CameraComponent/ICameraComponent/ICameraComponent.h"

UpdraftParticleSystem::UpdraftParticleSystem(BaseScene* pOwnerScene, int maxParticles)
	: ParticleSystem(pOwnerScene, maxParticles)
{
}

UpdraftParticleSystem::~UpdraftParticleSystem()
{
}

void UpdraftParticleSystem::Draw()
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

			//ビルボード行列計算。Y軸方向だけ長く、XZは細くすることで、上昇気流の「筋」のような見た目にする
			Math::Matrix scaleMat = Math::Matrix::CreateScale(p.size * 0.1f, p.size * 1.0f, p.size * 0.1f);
			Math::Matrix transMat = Math::Matrix::CreateTranslation(p.pos);
			Math::Matrix worldMat = scaleMat * billMat * transMat;

			//フェードアウト
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

void UpdraftParticleSystem::LoadAssets()
{
	m_model = KdAssets::Instance().m_modeldatas.GetData("Asset/Models/Particle/tyou.gltf");
}

void UpdraftParticleSystem::InitializeParticle(Particle& p, const Math::Vector3& pos, const Math::Vector3& baseDirection, float spread)
{
	p.pos = pos;

	//基準方向に対し、水平方向(XZ)にランダムなばらつきを持たせる
	Math::Vector3 dir = baseDirection;
	dir.x += KdRandom::GetFloat(-spread * 0.5f, spread * 0.5f);
	dir.z += KdRandom::GetFloat(-spread * 0.5f, spread * 0.5f);
	dir.Normalize();

	p.velocity = dir * KdRandom::GetFloat(2.0f, 4.0f);

	p.color = { 0.8f, 1.0f, 0.9f, 0.8f };

	p.lifetime = KdRandom::GetFloat(0.8f, 1.2f);
	p.maxLifetime = p.lifetime;

	p.size = KdRandom::GetFloat(0.1f, 0.2f);
	p.initialSize = p.size;

	p.rot = Math::Vector3::Zero;
	p.angularVelocity = Math::Vector3::Zero;
}

void UpdraftParticleSystem::UpdateParticle(Particle& p, float deltatime)
{
	//移動更新
	p.pos += p.velocity * deltatime;

	//水平方向の速度にランダムな値を加算し、ゆらゆらと揺れながら昇っていく空気の流れを表現
	p.velocity.x += KdRandom::GetFloat(-1.0f, 1.0f) * deltatime;
	p.velocity.z += KdRandom::GetFloat(-1.0f, 1.0f) * deltatime;
}