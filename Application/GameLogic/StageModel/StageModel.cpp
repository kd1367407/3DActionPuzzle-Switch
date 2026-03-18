#include "StageModel.h"
#include"../IStageObserver/IStageObserver.h"
#include"../Src/Framework/JsonConversion/JsonConversion.h"

void StageModel::Init()
{
	m_blockStates.clear();
}

bool StageModel::SwapBlocks(unsigned int id1, unsigned int id2, bool passRules)
{
	auto it1 = m_blockStates.find(id1);
	auto it2 = m_blockStates.find(id2);

	if (it1 == m_blockStates.end() || it2 == m_blockStates.end())return false;

	//入れ替え可能フラグで判定
	if (!passRules && (!it1->second.isSwappable || !it2->second.isSwappable))
	{
		return false;
	}

	//位置のみ入れ替える
	std::swap(it1->second.pos, it2->second.pos);

	//もし動くブロックなら、新しい位置に合わせて軌道を再計算する
	if (it1->second.moving.has_value())
	{
		//中身にアクセスする
		auto& data = it1->second.moving.value();

		//移動ベクトル(軌道)を計算
		Math::Vector3 trajectory = data.endPos - data.startPos;

		//入れ替え後の座標を新しい開始地点とし、そこから元の移動ベクトル分だけ動くように終了地点を補正する
		data.startPos = it1->second.pos;
		data.endPos = it1->second.pos + trajectory;
	}

	if (it2->second.moving.has_value())
	{
		auto& data = it2->second.moving.value();

		//移動ベクトル(軌道)を計算
		Math::Vector3 trajectory = data.endPos - data.startPos;

		//入れ替え後の座標に合わせて補正
		data.startPos = it2->second.pos;
		data.endPos = it2->second.pos + trajectory;
	}

	//Modelが変化したと通知
	NotifyObservers(id1);
	NotifyObservers(id2);

	return true;
}

const BlockState* StageModel::GetBlockState(unsigned int id) const
{
	auto it = m_blockStates.find(id);
	if (it != m_blockStates.end())
	{
		return &it->second;
	}
	return nullptr;
}

BlockState* StageModel::GetBlockState_Nonconst(UINT id)
{
	auto it = m_blockStates.find(id);
	if (it != m_blockStates.end())
	{
		return &it->second;
	}
	return nullptr;
}

void StageModel::UpdateBlockState(UINT id, const BlockState& newState)
{
	auto it = m_blockStates.find(id);
	if (it != m_blockStates.end())
	{
		it->second = newState;
		NotifyObservers(id);
	}
}

void StageModel::SyncBlockPos(UINT id, const Math::Vector3& currentPos)
{
	auto it = m_blockStates.find(id);
	if (it != m_blockStates.end())
	{
		//内部データのみ同期
		it->second.pos = currentPos;
	}
}

void StageModel::NotifyObservers(unsigned int updatedObjectId)
{
	//登録されている全ての観察者に通知する
	for (auto it = m_observers.begin(); it != m_observers.end();)
	{
		//弱参照(WeakPtr)をロックして生存確認。生きていれば通知し、死んでいればリストから削除する
		if (auto spObs = it->lock())
		{
			spObs->OnStageStateChanged(updatedObjectId);
			++it;
		}
		else
		{
			//寿命切れのObserverをリストから削除
			it = m_observers.erase(it);
		}
	}
}