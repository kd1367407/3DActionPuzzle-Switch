#pragma once
#include"../ParticleSystem.h"

class JumpParticleSystem :public ParticleSystem
{
public:
	JumpParticleSystem(BaseScene* pOwnerScene, int maxParticles = 200);
	~JumpParticleSystem() override;

	void	Draw()override;
protected:
	void LoadAssets() override;
	void InitializeParticle(Particle& p, const Math::Vector3& pos, const Math::Vector3& baseDirection, float spread) override;
	void UpdateParticle(Particle& p, float deltatime) override;
private:
	float m_gravity = -0.1f;
	float m_damping = 0.05f;
};