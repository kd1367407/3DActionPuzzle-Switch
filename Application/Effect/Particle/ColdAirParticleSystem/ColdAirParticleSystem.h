#pragma once
#include "../ParticleSystem.h"

class ColdAirParticleSystem : public ParticleSystem
{
public:
	ColdAirParticleSystem(BaseScene* pOwnerScene, int maxParticles = 200);
	~ColdAirParticleSystem() override;

	void Draw() override;

protected:
	void LoadAssets() override;
	void InitializeParticle(Particle& p, const Math::Vector3& pos, const Math::Vector3& baseDirection, float spread) override;
	void UpdateParticle(Particle& p, float deltatime) override;

private:
	float m_gravity = -0.3f; //重力加速度
	float m_damping = 0.5f;  //空気抵抗係数
};