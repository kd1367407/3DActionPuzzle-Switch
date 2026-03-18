#include "CreateObjectCommand.h"
#include"../Src/Application/GameViewModel.h"

CreateObjectCommand::CreateObjectCommand(const std::shared_ptr<GameViewModel>& viewModel, const std::vector<UINT>& createIds) :
	m_wpViewModel(viewModel), m_createdIDs(createIds)
{
}

void CreateObjectCommand::Execute()
{
	//このコマンドはオブジェクト生成直後に作成されるため、
	//初回の生成処理(Execute)はViewModel側で既に完了しており、ここでは何もしない
}

void CreateObjectCommand::Undo()
{
	if (auto spViewModel = m_wpViewModel.lock())
	{
		//「オブジェクトを生成した」操作の取り消し＝「生成されたオブジェクトの削除」
		//保持していたIDリストを使って対象オブジェクトを特定し、削除する
		for (UINT id : m_createdIDs)
		{
			spViewModel->FinalizeObjectDeletion(id);
		}
	}
}