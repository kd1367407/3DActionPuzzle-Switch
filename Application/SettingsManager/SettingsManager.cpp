#include "SettingsManager.h"
#include"../main.h"

SettingsManager& SettingsManager::Instance()
{
	static SettingsManager instance;
	return instance;
}

void SettingsManager::Init()
{
	LoadJson("Asset/Data/Settings/GameSettings.json", m_gameSettings);
	LoadJson("Asset/Data/Settings/Input.json", m_inputSettings);
}

void SettingsManager::SaveGameSetting()
{
	SaveJson("Asset/Data/Settings/GameSettings.json", m_gameSettings);
}

void SettingsManager::LoadJson(const std::string& filepath, nlohmann::json& outJson)
{
	std::ifstream ifs(filepath);

	if (ifs.is_open())
	{
		try
		{
			//ファイルストリームからJSONオブジェクトへ直接パースして読み込む
			ifs >> outJson;
		}
		catch (nlohmann::json::parse_error& e)
		{
			//JSONの構文エラー(カンマ忘れ等)があった場合の例外処理
			std::string errorMsg = "JSON parse error at " + filepath + ": " + e.what();
			Application::Instance().AddLog(errorMsg.c_str());
		}
	}
	else
	{
		std::string errorMsg = "Failed to open JSON file: " + filepath;
		Application::Instance().AddLog(errorMsg.c_str());
	}
}

void SettingsManager::SaveJson(const std::string& filepath, const nlohmann::json& json)
{
	std::ofstream ofs(filepath);
	if (ofs.is_open())
	{
		//std::setw(4)を使用することで、インデント幅4で見やすく整形してファイルに書き込む
		ofs << std::setw(4) << json << std::endl;
	}
	else
	{
		std::string errorMsg = "Failed to open JSON file for writing: " + filepath;
		Application::Instance().AddLog(errorMsg.c_str());
	}
}