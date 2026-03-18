#pragma once
#include"../Command.h"
#include"../Src/Application/GameData/BlockState/BlockState.h"

class StageModel;

class BlockStateChangeCommand : public ICommand
{
public:
	//複数オブジェクト用コンストラクタ
	BlockStateChangeCommand(
		std::shared_ptr<StageModel> spModel,
		const std::map<UINT, BlockState>& preStates,
		const std::map<UINT, BlockState>& newStates
	);

	//単体オブジェクト用コンストラクタ
	BlockStateChangeCommand(
		std::shared_ptr<StageModel> spModel,
		UINT objId,
		const BlockState& preState,
		const BlockState& newState);

	void Execute() override;
	void Undo() override;

private:
	std::weak_ptr<StageModel> m_wpModel;
	UINT m_objId = 0;

	//Undo/Redoを実現するために、変更前(Undo用)と変更後(Redo/Execute用)の状態を保持する
	std::map<UINT, BlockState> m_preStates;
	std::map<UINT, BlockState> m_newStates;
};