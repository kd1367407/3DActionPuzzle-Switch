#include "DeleteObjectCommand.h"
#include"../Src/Application/GameViewModel.h"
#include"../Src/Application/main.h"

DeleteObjectCommand::DeleteObjectCommand(std::shared_ptr<GameViewModel> viewModel, std::vector<UINT>& objectIds, const std::vector<BlockState>& deletedObjStates) :
	m_wpViewModel(viewModel), m_objectIds(objectIds), m_deletedObjectStates(deletedObjStates)
{
}

void DeleteObjectCommand::Execute()
{
	if (auto spViewModel = m_wpViewModel.lock())
	{
		for (UINT id : m_objectIds)
		{
			spViewModel->FinalizeObjectDeletion(id);
		}
	}
}

void DeleteObjectCommand::Undo()
{
	if (auto spViewModel = m_wpViewModel.lock())
	{
		//保存しておいた状態データからオブジェクトを復元
		for (const auto& state : m_deletedObjectStates)
		{
			spViewModel->CreateObjectFromState(state);
		}

		//復元された転送ブロックなどのペア情報を再構築
		spViewModel->PairTransferBlocks();
	}
}