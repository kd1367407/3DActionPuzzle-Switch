#include "SparkParticleSystem.h"
#include"../../../Scene/BaseScene/BaseScene.h"
#include "../Src/Framework/Component/CameraComponent/ICameraComponent/ICameraComponent.h"

SparkParticleSystem::SparkParticleSystem(BaseScene* pOwnerScene, int maxParticles)
	: ParticleSystem(pOwnerScene, maxParticles)
{
}

SparkParticleSystem::~SparkParticleSystem()
{
}

void SparkParticleSystem::Draw()
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

	//描画設定。火花は自ら発光するため加算合成(Add)を使用
	KdShaderManager::Instance().ChangeBlendState(KdBlendState::Add);
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

void SparkParticleSystem::LoadAssets()
{
	m_model = KdAssets::Instance().m_modeldatas.GetData("Asset/Models/Particle/Sphere.gltf");
}

void SparkParticleSystem::InitializeParticle(Particle& p, const Math::Vector3& pos, const Math::Vector3& baseDirection, float spread)
{
	p.pos = pos;

	//基準方向(baseDirection)に対し、XZ平面上でランダムなブレを与えて拡散させる
	Math::Vector3 dir = baseDirection;
	dir.x += KdRandom::GetFloat(-0.5f, 0.5f);
	dir.z += KdRandom::GetFloat(-0.5f, 0.5f);
	dir.Normalize();

	//速度の設定(元コードになかったため追加)。ランダムな速度で飛び散らせる
	p.velocity = dir * KdRandom::GetFloat(3.0f, 6.0f);

	//火花らしい色(赤～黄色)をランダムに設定
	p.color = { 1.0f,KdRandom::GetFloat(0.5f,0.8f),0.2f,1.0f };

	p.lifetime = KdRandom::GetFloat(0.3f, 0.6f);
	p.maxLifetime = p.lifetime;

	p.size = KdRandom::GetFloat(0.05f, 0.1f);
	p.initialSize = p.size;
}

void SparkParticleSystem::UpdateParticle(Particle& p, float deltatime)
{
	//重力の適用
	p.velocity.y += m_gravity * deltatime;
	p.pos += p.velocity * deltatime;

	//寿命に応じてサイズを縮小(燃え尽きる表現)
	float ratio = p.lifetime / p.maxLifetime;
	p.size = p.initialSize * ratio;
}