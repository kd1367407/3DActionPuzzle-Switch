#include "SwapBlockCommand.h"
#include"../Src/Application/GameLogic/StageModel/StageModel.h"
#include"../Src/Application/Scene/SceneManager.h"

SwapBlockCommand::SwapBlockCommand(std::shared_ptr<StageModel> model, unsigned int id1, unsigned int id2, bool isEditorRequest) :
	m_wpModel(model), m_id1(id1), m_id2(id2), m_isEditorRequest(isEditorRequest)
{
}

void SwapBlockCommand::Execute()
{
	if (auto spModel = m_wpModel.lock())
	{
		spModel->SwapBlocks(m_id1, m_id2, m_isEditorRequest);
	}
}

void SwapBlockCommand::Undo()
{
	//エディタモード(Create)の時のみUndoを許可
	if (SceneManager::Instance().GetCurrentMode() == SceneManager::SceneMode::Create)
	{
		//もう一度入れ替え処理を実行することで元の状態に戻す
		Execute();
	}
}