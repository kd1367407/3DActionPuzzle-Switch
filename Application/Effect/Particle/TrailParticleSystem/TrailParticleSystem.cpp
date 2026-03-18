#include "TrailParticleSystem.h"
#include"../../../Scene/BaseScene/BaseScene.h"
#include"../Src/Framework/Component/CameraComponent/ICameraComponent/ICameraComponent.h"

TrailParticleSystem::TrailParticleSystem(BaseScene* pOwnerScene, int maxParticles)
	: ParticleSystem(pOwnerScene, maxParticles)
{
}

TrailParticleSystem::~TrailParticleSystem()
{
}

void TrailParticleSystem::Draw()
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

void TrailParticleSystem::LoadAssets()
{
	m_model = KdAssets::Instance().m_modeldatas.GetData("Asset/Models/Particle/Sphere.gltf");
}

void TrailParticleSystem::InitializeParticle(Particle& p, const Math::Vector3& position, const Math::Vector3& baseDirection, float spread)
{
	p.pos = position;

	//軌跡としてその場に残る表現にするため初速はゼロにする
	p.velocity = Math::Vector3::Zero;

	p.color = { 0.2f, 1.0f, 0.4f, 0.8f };//緑色
	p.lifetime = KdRandom::GetFloat(1.0f, 1.5f);
	p.maxLifetime = p.lifetime;
	p.size = KdRandom::GetFloat(0.2f, 0.3f);
	p.initialSize = p.size;
	p.rot = Math::Vector3::Zero;
	p.angularVelocity = Math::Vector3::Zero;
}

void TrailParticleSystem::UpdateParticle(Particle& p, float deltatime)
{
	//寿命が減るにつれてサイズを小さくする
	float lifeRatio = std::max(0.0f, p.lifetime / p.maxLifetime);
	p.size = p.initialSize * lifeRatio;

	//物理演算は行わず、ゆっくりと沈下させる
	p.pos.y -= 0.5f * deltatime;
}