#include "SparkleParticleSystem.h"
#include"../../../Scene/BaseScene/BaseScene.h"
#include "../Src/Framework/Component/CameraComponent/ICameraComponent/ICameraComponent.h"

SparkleParticleSystem::SparkleParticleSystem(BaseScene* pOwnerScene, int maxParticles)
	: ParticleSystem(pOwnerScene, maxParticles)
{
}

SparkleParticleSystem::~SparkleParticleSystem()
{
}

void SparkleParticleSystem::Draw()
{
	auto iCamera = m_pOwnerScene->GetActiveCamera();
	if (!iCamera) return;
	const auto& camera = iCamera->GetCamera();
	if (!camera) return;

	//ビルボード行列の計算。カメラのビュー行列から回転成分のみを取り出し、逆行列にすることで常にカメラの方を向く回転行列を作成
	Math::Matrix viewMat = camera->GetCameraViewMatrix();
	Math::Matrix billMat = viewMat;
	billMat.Translation(Math::Vector3::Zero);
	billMat.Invert();

	//描画設定。加算合成(Add)できらめきを強調する
	KdShaderManager::Instance().ChangeBlendState(KdBlendState::Add);
	KdShaderManager::Instance().ChangeRasterizerState(KdRasterizerState::CullNone);

	KdShaderManager::Instance().m_postProcessShader.BeginBright();
	{
		for (auto& p : m_particles)
		{
			if (p.lifetime <= 0.0f)continue;

			//行列合成。Z軸回転(面内回転)→拡大縮小→ビルボード(カメラ対面)→平行移動の順で行うことで、カメラを向いたままクルクル回る表現になる
			Math::Matrix rotMat = Math::Matrix::CreateRotationZ(p.rot.z);
			Math::Matrix scaleMat = Math::Matrix::CreateScale(p.size);
			Math::Matrix transMat = Math::Matrix::CreateTranslation(p.pos);
			Math::Matrix finalMat = rotMat * scaleMat * billMat * transMat;

			//アルファ値計算
			float lifeRatio = p.lifetime / p.maxLifetime;
			Math::Color drawColor = p.color;
			drawColor.A(lifeRatio);

			//描画
			KdShaderManager::Instance().m_StandardShader.DrawModel(*m_model, finalMat, drawColor);
		}
	}
	KdShaderManager::Instance().m_postProcessShader.EndBright();

	KdShaderManager::Instance().UndoBlendState();
	KdShaderManager::Instance().UndoRasterizerState();
}

void SparkleParticleSystem::LoadAssets()
{
	m_model = KdAssets::Instance().m_modeldatas.GetData("Asset/Models/Particle/Sphere.gltf");
}

void SparkleParticleSystem::InitializeParticle(Particle& p, const Math::Vector3& pos, const Math::Vector3& baseDirection, float spread)
{
	p.pos = pos;

	//Y軸を0.0以上に制限することで、地面より上方向へのランダムな拡散ベクトルを作成
	Math::Vector3 dir;
	dir.x = KdRandom::GetFloat(-1.0, 1.0f);
	dir.y = KdRandom::GetFloat(0.0, 1.0f);
	dir.z = KdRandom::GetFloat(-1.0, 1.0f);
	dir.Normalize();
	p.velocity = dir * KdRandom::GetFloat(0.5f, 2.0f);

	p.color = { 0.5f,1.0f,1.0f,1.0f };

	p.lifetime = KdRandom::GetFloat(1.0f, 2.0f);
	p.maxLifetime = p.lifetime;

	p.size = KdRandom::GetFloat(0.1f, 0.3f);
	p.initialSize = p.size;

	p.rot.z = KdRandom::GetFloat(0.0f, 360.0f);
}

void SparkleParticleSystem::UpdateParticle(Particle& p, float deltatime)
{
	//空気抵抗による減速処理
	p.velocity *= 0.95f;
	p.pos += p.velocity * deltatime;

	//寿命の割合を0～1～0のサイン波に変換し、発生してから徐々に大きくなり、また小さくなって消える(明滅)挙動を作る
	float ratio = p.lifetime / p.maxLifetime;
	p.size = p.initialSize * sin(ratio * 3.14f);

	//Z軸(視線方向軸)回転を加算してキラキラ回る表現
	p.rot.z += 90.0f * deltatime;
}