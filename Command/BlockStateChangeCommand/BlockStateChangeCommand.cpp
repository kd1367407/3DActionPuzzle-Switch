#include "BlockStateChangeCommand.h"
#include"../Src/Application/GameLogic/StageModel/StageModel.h"

BlockStateChangeCommand::BlockStateChangeCommand(std::shared_ptr<StageModel> spModel, const std::map<UINT, BlockState>& preStates, const std::map<UINT, BlockState>& newStates) :
	m_wpModel(spModel), m_preStates(preStates), m_newStates(newStates)
{
}

BlockStateChangeCommand::BlockStateChangeCommand(std::shared_ptr<StageModel> spModel, UINT objId, const BlockState& preState, const BlockState& newState) :
	m_wpModel(spModel), m_objId(objId)
{
	m_preStates[objId] = preState;
	m_newStates[objId] = newState;
}

void BlockStateChangeCommand::Execute()
{
	//変更後の新しい状態で上書きする（Redoまたは初回の実行）
	if (auto spModel = m_wpModel.lock())
	{
		for (const auto& [id, state] : m_newStates)
		{
			spModel->UpdateBlockState(id, state);
		}
	}
}

void BlockStateChangeCommand::Undo()
{
	//変更前の古い状態で上書きして元に戻す（Undo）
	if (auto spModel = m_wpModel.lock())
	{
		for (const auto& [id, state] : m_preStates)
		{
			spModel->UpdateBlockState(id, state);
		}
	}
}