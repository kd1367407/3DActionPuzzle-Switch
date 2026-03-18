#pragma once
#include"../Src/Framework/Component/GameObject.h"

class JsonHelper
{
public:
	//Jsonオブジェクトから指定されたキーのVector3を読み込む
	static void GetVector3(const nlohmann::json& jsonObj, const std::string& key, Math::Vector3& outVec, const Math::Vector3& defaultValue = Math::Vector3::Zero)
	{
		if (jsonObj.contains(key) && jsonObj[key].is_array() && jsonObj[key].size() == 3)
		{
			outVec = { jsonObj[key][0], jsonObj[key][1], jsonObj[key][2] };
		}
		else
		{
			outVec = defaultValue;
		}
	}

	//Jsonオブジェクトから指定されたキーの文字列を読み込む
	static std::string GetString(const nlohmann::json& jsonObj, const std::string& key, const std::string& defaultValue = "")
	{
		return jsonObj.value(key, defaultValue);
	}

	//Jsonオブジェクトから指定されたキーのfloatを読み込む
	static float GetFloat(const nlohmann::json& jsonObj, const std::string& key, const float defaultValue = 0.0f)
	{
		return jsonObj.value(key, defaultValue);
	}

	//Jsonオブジェクトから指定されたキーのintを読み込む
	static int GetInt(const nlohmann::json& jsonObj, const std::string& key, const int defaultValue = 0)
	{
		return jsonObj.value(key, defaultValue);
	}

	//Jsonオブジェクトから指定されたキーのboolを読み込む
	static bool GetBool(const nlohmann::json& jsonObj, const std::string& key, const bool defaultValue = false)
	{
		return jsonObj.value(key, defaultValue);
	}

	//文字列から数値への変換
	static GameObject::Tag GetGameObjectTag(const nlohmann::json& jsonObj, const std::string& key, GameObject::Tag defaultValue = GameObject::Tag::None)
	{
		//キーが存在しない、もしくは文字列でない場合はデフォルト値
		if (!jsonObj.contains(key) || !jsonObj[key].is_string())
		{
			return defaultValue;
		}

		std::string tagStr = JsonHelper::GetString(jsonObj, key, "");

		//変換
		if (tagStr == "Player") return GameObject::Tag::Player;
		if (tagStr == "Goal")   return GameObject::Tag::Goal;
		if (tagStr == "Block")  return GameObject::Tag::Block;
		if (tagStr == "Jump")   return GameObject::Tag::Jump;
		if (tagStr == "System") return GameObject::Tag::System;
		if (tagStr == "None")   return GameObject::Tag::None;

		return defaultValue;
	}

	static std::string SetGameObjectTag(GameObject::Tag tag)
	{
		switch (tag)
		{
		case GameObject::Tag::Player: return "Player";
		case GameObject::Tag::Goal:   return "Goal";
		case GameObject::Tag::Block:  return "Block";
		case GameObject::Tag::Jump:   return "Jump";
		case GameObject::Tag::System: return "System";
		default:                      return "None";
		}
	}
};