#include "StageListManager.h"
#include"../JsonHelper/JsonHelper.h"

void StageListManager::UpdateStageLabel(const std::string& filePath, const std::string& newLabel)
{
	//ステージ一覧の配列を走査して、該当するファイルパスのエントリを探す
	if (m_stageListData.contains("stages") && m_stageListData["stages"].is_array())
	{
		for (auto& stageInfo : m_stageListData["stages"])
		{
			std::string path = JsonHelper::GetString(stageInfo, "path");
			if (path == filePath)
			{
				//ラベルを書き換えて保存
				stageInfo["label"] = newLabel;
				Save();
				break;
			}
		}
	}
}

void StageListManager::AddStageEntry(const std::string& newLabel, const std::string& newFilePath)
{
	if (!m_stageListData.contains("stages"))
	{
		m_stageListData["stages"] = nlohmann::json::array();
	}

	//新しいステージ情報をJSONオブジェクトとして作成
	nlohmann::json newEntry;
	newEntry["label"] = newLabel;
	newEntry["path"] = newFilePath;

	//配列の末尾に追加して保存
	m_stageListData["stages"].push_back(newEntry);
	Save();
}

std::string StageListManager::GenerateNewStagePath()
{
	int stageNumber = 0;
	std::string newFilePath;

	//未使用のファイル名が見つかるまで番号を増やしながらループ
	while (true)
	{
		//"Stage" + 2桁の数字(0埋め) + ".json" を生成(例: Stage05.json)
		std::string filePath = std::format("Stage{:02}.json", stageNumber);
		newFilePath = "Asset/Data/Stages/" + filePath;

		//既にリストに存在するかチェック
		if (!StagePathExists(newFilePath))
		{
			break;//このパスは使われていないので決定
		}
		stageNumber++;
	}
	return newFilePath;
}

StageListManager::StageListManager()
{
	Load();
}

void StageListManager::Load()
{
	std::ifstream ifs(m_filePath);
	if (ifs.is_open())
	{
		ifs >> m_stageListData;
	}
	else
	{
		//ファイルが無ければ新規作成
		m_stageListData = nlohmann::json::object();
		m_stageListData["stages"] = nlohmann::json::array();
	}
}

void StageListManager::Save()
{
	std::ofstream ofs(m_filePath);

	if (ofs)
	{
		//インデント幅4で整形して保存
		ofs << std::setw(4) << m_stageListData << std::endl;
	}
}

bool StageListManager::StagePathExists(const std::string& filePath)
{
	//重複チェック
	if (m_stageListData.contains("stages") && m_stageListData["stages"].is_array())
	{
		for (const auto& stageInfo : m_stageListData["stages"])
		{
			std::string path = JsonHelper::GetString(stageInfo, "path");
			if (path == filePath)
			{
				return true;//既に存在する
			}
		}
	}

	return false;//存在しない
}