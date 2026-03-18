#pragma once
#include"Particle.h"

class BaseScene;

class ParticleSystem
{
public:
	ParticleSystem(BaseScene* pOwnerScene, int maxParticles = 200):
		m_pOwnerScene(pOwnerScene),m_maxParticles(maxParticles){ }
	virtual ~ParticleSystem() = default;

	void	Init();
	virtual void Update(float deltatime) final;
	virtual void Draw() = 0;

	void Emit(
		const Math::Vector3& position,
		int count,
		const Math::Vector3& baseDirection,
		float spread
	);

protected:
	virtual void LoadAssets() = 0;
	virtual void InitializeParticle(Particle& p, const Math::Vector3& pos, const Math::Vector3& dir, float spread) = 0;
	virtual void UpdateParticle(Particle& p, float deltatime) = 0;

	//共通のデータ
	std::vector<Particle> m_particles;
	std::shared_ptr<KdModelData> m_model;
	BaseScene* m_pOwnerScene = nullptr;
	int m_maxParticles;
};