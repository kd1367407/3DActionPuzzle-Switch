#include "CameraComponent.h"
#include"../GameObject.h"
#include"../TransformComponent/TransformComponent.h"
#include"../Src/Application/Scene/SceneManager.h"
#include"../Src/Application/System/PhysicsSystem.h"
#include"../Src/Application/main.h"
#include"../Src/Application/SettingsManager/SettingsManager.h"
#include"../Src/Application/JsonHelper/JsonHelper.h"

void CameraComponent::Awake()
{
	//カメラインスタンス生成
	m_spCamera = std::make_shared<KdCamera>();
	m_spCamera->SetProjectionMatrix(60.0f);
}

void CameraComponent::Start()
{
	//自身がアタッチされているGameObjectから必要なコンポーネントを取得
	m_transform = m_owner->GetComponent<TransformComponent>();
}

void CameraComponent::Update()
{
	if (!m_spCamera)return;

	//1.現在のバックバッファのサイズ変更を取得
	auto& backBufferInfo = KdDirect3D::Instance().GetBackBuffer()->GetInfo();

	//2.アスペクト比(縦横比)を計算
	if (backBufferInfo.Height == 0)return;

	float aspectRatio = (float)backBufferInfo.Width / (float)backBufferInfo.Height;

	//3.カメラの射影行列を再設定
	m_spCamera->SetProjectionMatrix(60.0f, 0.01f, 2000.0f, aspectRatio);
}
