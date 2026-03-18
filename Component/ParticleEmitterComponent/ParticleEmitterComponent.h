#pragma once
#include"../Component.h"

class TransformComponent;
class GameScene;
class GameViewModel;

class ParticleEmitterComponent :public Component
{
public:
	void Configure(const nlohmann::json& data) override;
	void Start() override;
	void Update() override;
	nlohmann::json ToJson() const override;
	void OnInspect() override;

	void RequestParamChangeCommand();
	void SetViewModel(std::shared_ptr<GameViewModel> vm);

	void SetActive(bool active) { m_isActive = active; }
	bool IsActive() const { return m_isActive; }

	//--BlockState連携用--
	const	 std::string& GetSystemName()const { return m_systemName; }
	int GetEmitCount()const { return m_emitCount; }
	float GetEmitFrequency() const { return m_emitFrequency; }
	const Math::Vector3& GetBaseDirection() const { return m_baseDir; }
	float GetSpread() const { return m_spread; }
	const std::vector<Math::Vector3>& GetOffsets() const { return m_offsets; }
	void SetSystemName(const std::string& name) { m_systemName = name; }
	void SetEmitCount(int count) { m_emitCount = count; }
	void SetEmitFrequency(float freq) { m_emitFrequency = freq; }
	void SetBaseDirection(const Math::Vector3& dir) { m_baseDir = dir; }
	void SetSpread(float spread) { m_spread = spread; }
	void SetOffsets(const std::vector<Math::Vector3>& offsets) { m_offsets = offsets; }

	const char* GetComponentName() const override { return "ParticleEmitterComponent"; }

private:
	//--jsonから読み込む値--
	std::string m_systemName = "ColdAir";
	int	 m_emitCount = 3;//一回に何個出すか
	float	m_emitFrequency = 0.1f;//何秒毎に出すか
	Math::Vector3 m_baseDir = { 0.0f,1.0f,0.0f };//放出方向
	float	m_spread = 0.3f;//広がり
	std::vector<Math::Vector3> m_offsets; //オブジェクトの中心からのoffset

	//--実行時データ--
	float m_timer = 0.0f;//頻度計測タイマー	
	bool m_isActive = true;

	//--キャッシュするポインタ--
	std::shared_ptr<TransformComponent> m_transform;
	std::weak_ptr<GameViewModel> m_wpViewModel;
	GameScene* m_pGameScene = nullptr;
};