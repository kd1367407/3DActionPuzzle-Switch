#include "Archetype.h"
#include"../Component/GameObject.h"
#include"../Component/Component.h"
#include"../Component/TransformComponent/TransformComponent.h"
#include"../Component/PlayerStatsComponent/PlayerStatsComponent.h"

std::shared_ptr<GameObject> Archetype::Instantiate(const nlohmann::json& entityData) const
{
	//--1. 骨格(GameObject)の生成--
	auto newObject = std::make_shared<GameObject>();

	//登録されている全コンポーネントを生成して追加
	for (const auto& creator : m_componentCreators)
	{
		newObject->AddComponent(creator());
	}
	newObject->SetName(m_defaultName);

	newObject->SetTag(m_tag);

	//--2. アーキタイプとしてのデフォルト値を適用--
	//(Archetypes.jsonで定義された値)
	for (const auto& comp : newObject->GetComponents())
	{
		comp->Configure(m_componentData);
	}

	//--3. ステージ個別の設定データで上書き--
	//(StageXX.jsonなどで定義された配置情報など)
	if (entityData.contains("components"))
	{
		for (const auto& comp : newObject->GetComponents())
		{
			comp->Configure(entityData.at("components"));
		}
	}

	//名前の上書き
	if (entityData.contains("name"))
	{
		newObject->SetName(entityData.at("name").get<std::string>());
	}

	if (entityData.contains("tag"))
	{
		newObject->SetTag(entityData.at("tag").get<GameObject::Tag>());
	}

	//初期化
	newObject->Init();

	return newObject;
}