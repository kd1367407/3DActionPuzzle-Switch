#include "ShockwaveParticleSystem.h"
#include"../../../Scene/BaseScene/BaseScene.h"
#include"../Src/Framework/Component/CameraComponent/ICameraComponent/ICameraComponent.h"

ShockwaveParticleSystem::ShockwaveParticleSystem(BaseScene* pOwnerScene, int maxParticles)
	: ParticleSystem(pOwnerScene, maxParticles)
{
}

ShockwaveParticleSystem::~ShockwaveParticleSystem()
{
}

void ShockwaveParticleSystem::Draw()
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

	//描画設定。衝撃波なので加算合成(Add)で光を強調する
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

void ShockwaveParticleSystem::LoadAssets()
{
	m_model = KdAssets::Instance().m_modeldatas.GetData("Asset/Models/Particle/Sphere.gltf");
}

void ShockwaveParticleSystem::InitializeParticle(Particle& p, const Math::Vector3& pos, const Math::Vector3& baseDirection, float spread)
{
	p.pos = pos;

	//0度～360度のランダムな角度を決定し、XZ平面(水平方向)に広がる円形の衝撃波を作成
	float angle = KdRandom::GetFloat(0.0f, 360.0f);
	float rad = DirectX::XMConvertToRadians(angle);

	//三角関数を使って角度から進行方向ベクトル(X,Z成分)を算出
	Math::Vector3 dir;
	dir.x = sin(rad);
	dir.y = 0.0f;
	dir.z = cos(rad);
	dir.Normalize();

	p.velocity = dir * KdRandom::GetFloat(5.0f, 8.0f);

	p.color = { 1.0f,1.0f,0.5f,1.0f };

	p.lifetime = KdRandom::GetFloat(0.3f, 0.5f);
	p.maxLifetime = p.lifetime;

	p.size = KdRandom::GetFloat(0.2f, 0.4f);
	p.initialSize = p.size;
}

void ShockwaveParticleSystem::UpdateParticle(Particle& p, float deltatime)
{
	//空気抵抗の適用。広がりすぎないように徐々に減速させる
	p.velocity *= 0.95;
	p.pos += p.velocity * deltatime;

	//寿命が尽きるにつれてサイズを小さくし、衝撃波が消えていく様子を表現
	float ratio = p.lifetime / p.maxLifetime;
	p.size = p.initialSize * ratio;
}