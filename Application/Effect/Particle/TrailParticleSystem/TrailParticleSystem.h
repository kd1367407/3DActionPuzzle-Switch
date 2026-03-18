#pragma once
#include"../ParticleSystem.h"

class TrailParticleSystem:public ParticleSystem
{
public:
	TrailParticleSystem(BaseScene* pOwnerScene, int maxParticles = 200);
	~TrailParticleSystem() override;

	void Draw()override;

protected:
	void LoadAssets() override;

	void InitializeParticle(Particle& p, const Math::Vector3& position, const Math::Vector3& baseDirection, float spread) override;

	void UpdateParticle(Particle& p, float deltatime) override;

private:

};