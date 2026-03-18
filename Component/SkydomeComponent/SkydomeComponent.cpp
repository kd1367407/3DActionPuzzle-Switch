#include "SkydomeComponent.h"
#include"../GameObject.h"
#include"../TransformComponent/TransformComponent.h"
#include"../Src/Application/Scene/BaseScene/BaseScene.h"
#include"../Src/Application/Scene/SceneManager.h"
#include"../CameraComponent/ICameraComponent/ICameraComponent.h"

void SkydomeComponent::Awake()
{
	m_spModel = KdAssets::Instance().m_modeldatas.GetData("Asset/Models/Skydome/Skydome1.gltf");
	m_gridTex1 = KdAssets::Instance().m_textures.GetData("Asset/Textures/grid2.png");
	m_gridTex2 = KdAssets::Instance().m_textures.GetData("Asset/Textures/grid3.png");
}

void SkydomeComponent::Start()
{
	m_transform = m_owner->GetComponent<TransformComponent>();
}

void SkydomeComponent::Update()
{
	float deltaTime = Application::Instance().GetDeltaTime();

	//2枚のテクスチャをそれぞれ縦と横にスクロールさせてサイバー空間っぽい演出にする
	m_uvOffset1.y += 0.02f * deltaTime;
	m_uvOffset2.x += 0.02f * deltaTime;

	//ループ処理
	if (m_uvOffset1.y > 1.0f) m_uvOffset1.y -= 1.0f;
	if (m_uvOffset2.y > 1.0f) m_uvOffset2.y -= 1.0f;
}

void SkydomeComponent::PostUpdate()
{
	//カメラの位置を取得し、スカイドームを追従させる
	//(スカイドームが近づいてこないように見せるため)
	if (auto camera = SceneManager::Instance().GetCurrentScene()->GetActiveCamera())
	{
		Math::Vector3 camPos = camera->GetCamera()->GetCameraMatrix().Translation();

		if (m_transform)
		{
			camPos.y -= 20.0f;//少し下にずらして配置
			m_transform->SetPos(camPos);
		}
	}
}

void SkydomeComponent::DrawUnLit()
{
	if (!m_spModel || !m_transform) return;

	//--スカイドーム用の描画設定--
	//zバッファへの書き込みOFF(最奥に描画するため、深度情報を更新しない)
	KdShaderManager::Instance().ChangeDepthStencilState(KdDepthStencilState::ZWriteDisable);
	//裏面カリンングをOFF(球の内側から見るため)
	KdShaderManager::Instance().ChangeRasterizerState(KdRasterizerState::CullNone);

	//グリッド描画用のシェーダーパラメータ設定
	KdShaderManager::Instance().m_StandardShader.SetUVTiling({ 5.0f, 5.0f });

	KdShaderManager::Instance().m_StandardShader.SetGridUVOffset(m_uvOffset2, m_uvOffset1);

	KdShaderManager::Instance().m_StandardShader.SetGridTexture(*m_gridTex1, *m_gridTex2);

	KdShaderManager::Instance().m_StandardShader.SetGridEnable(true);

	//描画(陰影なし)
	KdShaderManager::Instance().m_StandardShader.DrawModel(*m_spModel, m_transform->GetMatrix());

	KdShaderManager::Instance().m_StandardShader.SetGridEnable(false);

	//設定を元に戻す
	KdShaderManager::Instance().UndoRasterizerState();
	KdShaderManager::Instance().UndoDepthStencilState();
}