#include "GroupSwapCommand.h"
#include"../Src/Application/GameLogic/StageModel/StageModel.h"

GroupSwapCommand::GroupSwapCommand(std::shared_ptr<StageModel> model, const std::vector<UINT>& groupAIds, const std::vector<UINT>& groupBIds, const Math::Vector3& deltaAtoB) :
	m_wpModel(model), m_groupAIds(groupAIds), m_groupBIds(groupBIds), m_delta(deltaAtoB)
{
}

void GroupSwapCommand::Execute()
{
	MoveObject(m_groupAIds, m_delta);
	MoveObject(m_groupBIds, -m_delta);
}

void GroupSwapCommand::Undo()
{
	MoveObject(m_groupAIds, -m_delta);
	MoveObject(m_groupBIds, m_delta);
}

void GroupSwapCommand::MoveObject(const std::vector<UINT>& ids, const Math::Vector3& offset)
{
	if (auto spModel = m_wpModel.lock())
	{
		for (UINT id : ids)
		{
			if (BlockState* state = spModel->GetBlockState_Nonconst(id))
			{
				//移動前の軌道ベクトルを計算して保持
				Math::Vector3 trajectory = Math::Vector3::Zero;
				if (state->moving.has_value())
				{
					trajectory = state->moving->endPos - state->moving->startPos;
				}

				//座標更新
				state->pos += offset;

				//動く床の場合、現在位置の変更に合わせて始点と終点もずらす
				if (state->moving.has_value())
				{
					state->moving->startPos = state->pos;
					state->moving->endPos = state->moving->startPos + trajectory;
				}

				//沈む床の場合、初期位置パラメータも更新しないと挙動がおかしくなるため更新
				if (state->sinking.has_value())
				{
					state->sinking->initialPos = state->pos;
				}

				spModel->NotifyObservers(id);
			}
		}
	}
}