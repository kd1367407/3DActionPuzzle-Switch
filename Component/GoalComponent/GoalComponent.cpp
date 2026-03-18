#include "GoalComponent.h"
#include"../GameObject.h"
#include"../Src/Application/Scene/BaseScene/BaseScene.h"
#include"../TimerComponent/TimerComponent.h"
#include"../ICollisionReceiver.h"
#include"../Src/Application/Scene/SceneManager.h"
#include"../Src/Application/Scene/GameScene/GameManager/GameManager.h"

void GoalComponent::Awake()
{
	int a = 1;//実験
}

void GoalComponent::OnTriggerEnter(const CollisionInfo& info)
{
	//ぶつかってきた相手がplayerか
	if (info.otherObject->HasTag(GameObject::Tag::Player))
	{
		
		//現在のシーンからタイマーを取得
		if (auto scene = SceneManager::Instance().GetCurrentScene())
		{
			if (auto timerObj = scene->FindObject("Timer"))
			{
				if (auto timerComp = timerObj->GetComponent<TimerComponent>())
				{
					// GameManagerに最終的なタイムを記録
					GameManager::Instance().SetFinalTime(timerComp->GetElapsedTime());
				}
			}
		}

		//ResultSceneへの切り替え予約
		SceneManager::Instance().ChangeScene(SceneManager::SceneType::Result);
	}
}
