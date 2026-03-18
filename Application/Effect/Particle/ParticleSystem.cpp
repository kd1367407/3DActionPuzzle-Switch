#include "ParticleSystem.h"
#include"../Src/Framework/Component/GameObject.h"
#include"../../Scene/BaseScene/BaseScene.h"
#include"../Src/Framework/Component/TransformComponent/TransformComponent.h"
#include"../Src/Framework/Component/RenderComponent/RenderComponent.h"


void ParticleSystem::Init()
{
	//マスターモデルをロード
	LoadAssets();
	if (!m_model)assert("パーティクル用のマスターメッシュが見つかりません");

	//メモリ確保
	m_particles.resize(m_maxParticles);

	//オブジェクトプール作成。最大数分だけGameObjectを事前に生成し、非表示状態で待機させておく
	for (int i = 0; i < m_maxParticles; ++i)
	{
		auto& p = m_particles[i];
		p.gameObject = std::make_shared<GameObject>();
		p.gameObject->SetName("Particle_" + std::to_string(i));
		p.gameObject->SetTag(GameObject::Tag::System);
		p.gameObject->AddComponent<TransformComponent>();
		auto renderComp = p.gameObject->AddComponent<RenderComponent>();
		renderComp->SetModel(m_model);

		//初期状態では描画しない
		renderComp->SetEnable(false);
		p.lifetime = 0.0f;

		m_pOwnerScene->AddObject(p.gameObject);
	}
}

void ParticleSystem::Update(float deltatime)
{
	for (auto& p : m_particles)
	{
		//寿命が切れているものは更新しない
		if (p.lifetime <= 0.0f)continue;

		//寿命を減算
		p.lifetime -= deltatime;

		//寿命尽きた瞬間の処理
		if (p.lifetime <= 0.0f)
		{
			//描画をオフにして待機状態に戻す
			p.gameObject->GetComponent<RenderComponent>()->SetEnable(false);
			continue;
		}

		//各パーティクル固有の物理演算(移動など)
		UpdateParticle(p, deltatime);

		//計算結果をGameObjectのTransformに反映
		auto transform = p.gameObject->GetComponent<TransformComponent>();
		if (transform)
		{
			transform->SetPos(p.pos);

			//オイラー角(ラジアン)を度数法に変換してセット
			Math::Vector3 rotDeg = {
				DirectX::XMConvertToDegrees(p.rot.x),
				DirectX::XMConvertToDegrees(p.rot.y),
				DirectX::XMConvertToDegrees(p.rot.z)
			};
			transform->SetRot(rotDeg);

			transform->SetScale({ p.size, p.size, p.size });
		}
	}
}

void ParticleSystem::Emit(const Math::Vector3& position, int count, const Math::Vector3& baseDirection, float spread)
{
	for (int i = 0; i < count; ++i)
	{
		//プールから現在使用されていない(寿命切れの)パーティクルを検索
		Particle* p = nullptr;
		for (auto& particle : m_particles)
		{
			if (particle.lifetime <= 0.0f)
			{
				p = &particle;
				break;
			}
		}

		//空きがなければ生成を中止
		if (!p)break;

		//使用可能にするため描画を有効化
		p->gameObject->GetComponent<RenderComponent>()->SetEnable(true);

		//パラメータを初期化して発射
		InitializeParticle(*p, position, baseDirection, spread);
	}
}