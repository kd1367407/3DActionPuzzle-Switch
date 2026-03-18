#include "GameManager.h"
#include"../Src/Framework/Component/GameObject.h"
#include"../Src/Framework/Component/CheckpointComponent/CheckpointComponent.h"

void GameManager::UpdateAfterNewStageSave(const std::string& newFilePath)
{
	m_currentStagePath = newFilePath;
	m_loadMode = LoadMode::Edit;
}

void GameManager::SetActiveCheckpoint(const std::shared_ptr<GameObject>& newCheckpointObject)
{
	if (!newCheckpointObject)return;

	auto newCheckpointComp = newCheckpointObject->GetComponent<CheckpointComponent>();
	if (!newCheckpointComp)return;

	if (auto oldActive = m_activeCheckpoint.lock())
	{
		if (oldActive.get() != newCheckpointComp.get())
		{
			oldActive->Deactivate();
		}
	}
	m_activeCheckpoint = newCheckpointComp;
}
