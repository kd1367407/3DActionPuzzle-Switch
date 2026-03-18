#pragma once
#include"../ParticleSystem.h"

class UpdraftParticleSystem :public ParticleSystem
{
public:
	UpdraftParticleSystem(BaseScene* pOwnerScene, int maxParticles = 200);
	~UpdraftParticleSystem() override;

	void	Draw()override;
protected:
	void LoadAssets() override;
	void InitializeParticle(Particle& p, const Math::Vector3& pos, const Math::Vector3& baseDirection, float spread) override;
	void UpdateParticle(Particle& p, float deltatime) override;
private:
	float m_gravity = -0.3f;
	float m_damping = 0.5f;
};