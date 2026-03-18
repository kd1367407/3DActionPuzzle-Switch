#pragma once
#include "../ParticleSystem.h"

class ImplosionParticleSystem : public ParticleSystem
{
public:
	ImplosionParticleSystem(BaseScene* pOwnerScene, int maxParticles = 400);
	~ImplosionParticleSystem() override;

	void Draw() override;

protected:
	void LoadAssets() override;
	void InitializeParticle(Particle& p, const Math::Vector3& pos, const Math::Vector3& baseDirection, float spread) override;
	void UpdateParticle(Particle& p, float deltatime) override;

private:

};