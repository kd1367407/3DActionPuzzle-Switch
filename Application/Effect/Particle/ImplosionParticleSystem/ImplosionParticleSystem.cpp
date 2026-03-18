#include "ImplosionParticleSystem.h"
#include"../../../Scene/BaseScene/BaseScene.h"
#include"../Src/Framework/Component/CameraComponent/ICameraComponent/ICameraComponent.h"

ImplosionParticleSystem::ImplosionParticleSystem(BaseScene* pOwnerScene, int maxParticles)
	: ParticleSystem(pOwnerScene, maxParticles)
{
}

ImplosionParticleSystem::~ImplosionParticleSystem()
{
}

void ImplosionParticleSystem::Draw()
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

	//描画設定。加算合成(Add)で光が重なって強くなる表現にする
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

			//寿命の割合を算出してフェードアウト
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

void ImplosionParticleSystem::LoadAssets()
{
	m_model = KdAssets::Instance().m_modeldatas.GetData("Asset/Models/Particle/Sphere.gltf");
}

void ImplosionParticleSystem::InitializeParticle(Particle& p, const Math::Vector3& pos, const Math::Vector3& baseDirection, float spread)
{
	//全方位へのランダムな方向ベクトルを作成
	Math::Vector3 randomDir;
	randomDir.x = KdRandom::GetFloat(-1.0f, 1.0f);
	randomDir.y = KdRandom::GetFloat(-1.0f, 1.0f);
	randomDir.z = KdRandom::GetFloat(-1.0f, 1.0f);
	randomDir.Normalize();

	//中心からの距離をランダムに決定
	float dist = KdRandom::GetFloat(1.0f, 1.5f);
	Math::Vector3 offset = randomDir * dist;

	//パーティクルの発生位置を中心から離れた外側に設定
	p.pos = pos + offset;

	//中心に到達するまでの時間を0.4秒と想定して速度を計算(距離÷時間=速度)
	float speed = dist / 0.4f;

	//中心に向かうベクトルにするためマイナスを掛けて反転させる(吸い込み表現)
	p.velocity = -randomDir * speed;

	p.lifetime = 0.5f;
	p.maxLifetime = p.lifetime;

	p.size = KdRandom::GetFloat(0.5f, 1.0f);
	p.initialSize = p.size;
}

void ImplosionParticleSystem::UpdateParticle(Particle& p, float deltatime)
{
	p.pos += p.velocity * deltatime;
}