#include "InputHandler.h"
#include"../InputReceiver/IInputReceiver.h"
#include"../Src/Application/Scene/SceneManager.h"//シーンオブジェクトにアクセスするため
#include"../Src/Application/Scene/BaseScene/BaseScene.h"
#include"../Src/Application/System/PhysicsSystem.h"
#include"../Src/Application/GameViewModel.h"
#include"../Src/Application/Scene/SceneManager.h"
#include"../../Component/CameraComponent/CameraComponent.h"
#include"../Src/Application/main.h"
#include"../Src/Framework/Component/RenderComponent/RenderComponent.h"
#include"../Src/Framework/Component/IdComponent/IdComponent.h"
#include"../Src/Application/GameData/BlockState/BlockState.h"

void InputHandler::Update()
{
	//レイ生成に必要なカメラ情報を取得
	auto activeCameraComp = SceneManager::Instance().GetCurrentScene()->GetActiveCamera();
	if (!activeCameraComp)return;
	auto kdCamera = activeCameraComp->GetCamera();
	if (!kdCamera)return;

	//マウスカーソルのスクリーン座標を取得し、クライアント領域(ウィンドウ内)の座標に変換
	POINT mousePos;
	GetCursorPos(&mousePos);
	ScreenToClient(Application::Instance().GetWindowHandle(), &mousePos);

	//カメラ位置からマウスカーソルの方向へ伸びるレイ(光線)を作成
	RayInfo ray;
	kdCamera->GenerateRayInfoFromClientPos(mousePos, ray.m_start, ray.m_dir, ray.m_maxDistance);

	//--ホバー処理--
	std::shared_ptr<GameObject> currentHoveredObj = nullptr;

	//物理システムにレイキャストを依頼し、ブロックレイヤーとの衝突を判定
	RayResult result;
	if (PhysicsSystem::Instance().Raycast(ray, result, CollisionLayer::LayerBlock))
	{
		//何らかのブロックにヒットした場合
		if (auto hitObj = result.m_hitObject.lock())
		{
			//ViewModelを通じて、そのブロックが「入れ替え可能(Swappable)」か確認
			if (auto viewModel = m_wpViewModel.lock())
			{
				if (auto idComp = hitObj->GetComponent<IdComponent>())
				{
					UINT id = idComp->GetId();
					BlockState state = viewModel->GetBlockState(id);

					if (state.isSwappable)
					{
						currentHoveredObj = hitObj;
					}
				}
			}
		}
	}
	auto preHoveredObj = m_wpHoverdObj.lock();

	//前フレームとホバー対象が変化した場合の処理
	if (currentHoveredObj != preHoveredObj)
	{
		//前のオブジェクトのホバー解除処理
		if (preHoveredObj)
		{
			if (auto renderComp = preHoveredObj->GetComponent<RenderComponent>())
			{
				//既にクリックされて「選択状態」になっている場合は解除しない
				if (renderComp->GetHighlightState() == RenderComponent::HighlightState::Hoverd)
				{
					renderComp->SetHighlightState(RenderComponent::HighlightState::None);
				}
			}
		}

		//新しいオブジェクトのホバー開始処理
		if (currentHoveredObj)
		{
			if (auto renderComp = currentHoveredObj->GetComponent<RenderComponent>())
			{
				//未選択状態の場合のみホバー表示にする
				if (renderComp->GetHighlightState() == RenderComponent::HighlightState::None)
				{
					renderComp->SetHighlightState(RenderComponent::HighlightState::Hoverd);
				}
			}
		}
	}
	//今回のホバー対象を記録
	m_wpHoverdObj = currentHoveredObj;


	//左クリックによる選択確定
	if (KdInputManager::Instance().IsPress("Select"))
	{
		if (auto viewModel = m_wpViewModel.lock())
		{
			if (currentHoveredObj)
			{
				viewModel->PlayPlayerAction();
				viewModel->OnBlockSelected(currentHoveredObj);
			}
		}
	}

	//右クリックによる選択解除
	if (KdInputManager::Instance().IsPress("CameraRotate"))
	{
		if (auto viewModel = m_wpViewModel.lock())
		{
			if (currentHoveredObj)
			{
				viewModel->BlockUnselected(currentHoveredObj);
			}
		}
	}
}