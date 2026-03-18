#pragma once
#include "../ParticleSystem.h"

class SparkParticleSystem : public ParticleSystem
{
public:
	SparkParticleSystem(BaseScene* pOwnerScene, int maxParticles = 400);
	~SparkParticleSystem() override;

	void Draw() override;

protected:
	void LoadAssets() override;
	void InitializeParticle(Particle& p, const Math::Vector3& pos, const Math::Vector3& baseDirection, float spread) override;
	void UpdateParticle(Particle& p, float deltatime) override;

private:
	float m_gravity = -0.5f;
};