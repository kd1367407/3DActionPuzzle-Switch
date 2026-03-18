#include "DustParticleSystem.h"
#include"../../../Scene/BaseScene/BaseScene.h"
#include"../Src/Framework/Component/CameraComponent/ICameraComponent/ICameraComponent.h"

DustParticleSystem::DustParticleSystem(BaseScene* pOwnerScene, int maxParticles)
	: ParticleSystem(pOwnerScene, maxParticles)
{
}

DustParticleSystem::~DustParticleSystem()
{
}

void DustParticleSystem::Draw()
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

			//寿命の割合(1.0->0.0)を算出し、フェードアウトさせる
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

void DustParticleSystem::LoadAssets()
{
	m_model = KdAssets::Instance().m_modeldatas.GetData("Asset/Models/Particle/Sphere.gltf");
}

void DustParticleSystem::InitializeParticle(Particle& p, const Math::Vector3& position, const Math::Vector3& baseDirection, float spread)
{
	p.pos = position;
	p.velocity = Math::Vector3::Zero;
	p.color = { 0.2f, 0.2f, 0.25f, 0.8f };
	p.lifetime = KdRandom::GetFloat(1.0f, 1.5f);
	p.maxLifetime = p.lifetime;

	p.size = KdRandom::GetFloat(0.2f, 0.3f);
	p.initialSize = p.size; //サイズ変化のために初期サイズを保存

	p.rot = Math::Vector3::Zero;
	p.angularVelocity = Math::Vector3::Zero;
}

void DustParticleSystem::UpdateParticle(Particle& p, float deltatime)
{
	//寿命の割合を計算
	float lifeRatio = std::max(0.0f, p.lifetime / p.maxLifetime);

	//寿命が減るにつれてサイズを小さくする(塵が消滅していく表現)
	p.size = p.initialSize * lifeRatio;

	//ゆっくりと下に沈下していく動き
	p.pos.y -= 0.5f * deltatime;
}