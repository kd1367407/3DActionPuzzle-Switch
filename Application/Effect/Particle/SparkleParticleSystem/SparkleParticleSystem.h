#pragma once
#include "../ParticleSystem.h"

class SparkleParticleSystem : public ParticleSystem
{
public:
	SparkleParticleSystem(BaseScene* pOwnerScene, int maxParticles = 200);
	~SparkleParticleSystem() override;

	void Draw() override;

protected:
	void LoadAssets() override;
	void InitializeParticle(Particle& p, const Math::Vector3& pos, const Math::Vector3& baseDirection, float spread) override;
	void UpdateParticle(Particle& p, float deltatime) override;

private:
	
};