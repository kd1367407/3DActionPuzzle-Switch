#include "BaseScene.h"
#include"../Src/Framework/Component/CameraComponent/CameraComponent.h"
#include"../Src/Framework/Component/GameObject.h"
#include"../Src/Framework/Component/TransformComponent/TransformComponent.h"
#include"../Src/Framework/Component/RenderComponent/RenderComponent.h"
#include"../../main.h"
#include"../../System/PhysicsSystem.h"
#include"../Src/Framework/Component/RigidbodyComponent/RigidbodyComponent.h"

void BaseScene::PreUpdate()
{
	//寿命切れのオブジェクトをリストから除外
	for (auto it = m_objList.begin(); it != m_objList.end();)
	{
		if ((*it)->IsExpired())
		{
			it = m_objList.erase(it);
		}
		else
		{
			++it;
		}
	}

	//物理演算用の力をリセット。毎フレーム0に戻してから、Updateで再び力を加算していく方式
	for (const auto& obj : m_objList)
	{
		if (auto rigid = obj->GetComponent<RigidbodyComponent>())
		{
			rigid->m_force = Math::Vector3::Zero;
		}
	}

	for (const auto& obj : m_objList)
	{
		obj->PreUpdate();
	}
}

void BaseScene::Update()
{
	//シーン固有の更新処理
	SceneUpdate();

	for (const auto& obj : m_objList)
	{
		obj->Update();
	}
}

void BaseScene::PostUpdate()
{
	//物理シミュレーションの安定化処理
	float deltaTime = Application::Instance().GetDeltaTime();
	if (deltaTime > 0.0f)
	{
		//理想の物理演算の間隔(60FPS)
		const float FIXED_TIME_STEP = 1.0f / 60.0f;

		//今回のフレームで"何回分の60FPS計算"が必要か計算
		int steps = static_cast<int>(std::floor(deltaTime / FIXED_TIME_STEP));

		//最低1回は回す
		if (steps < 1)steps = 1;

		//計算落ち防止：もし処理落ちしてステップ数が過大になっても、最大5回で打ち切る(スローモーションにはなるがフリーズは防ぐ)
		if (steps > 5)steps = 5;

		//deltatimeを均等に分割して小刻みに物理演算を回すことで、すり抜けなどを防ぐ
		float subDeltatime = deltaTime / steps;

		for (int i = 0; i < steps; i++)
		{
			PhysicsSystem::Instance().Update(subDeltatime);
		}
	}

	for (const auto& obj : m_objList)
	{
		obj->PostUpdate();
	}
}

void BaseScene::PreDraw()
{
	//アクティブカメラの行列情報をシェーダー(GPU)に転送する
	if (auto spActiveCamera = m_wpActiveCamera.lock())
	{
		if (const auto& kdCam = spActiveCamera->GetCamera())
		{
			kdCam->SetToShader();
		}
	}
}

void BaseScene::Draw()
{
	//3Dオブジェクトを描画する直前に、光と霧の情報をシェーダーに送る
	KdShaderManager::Instance().WorkAmbientController().Draw();

	// ===== ===== ===== ===== ===== ===== ===== ===== ===== ===== ===== =====
	//【影の生成】
	//光を遮るオブジェクトを描画し、影用の深度マップを作成する
	KdShaderManager::Instance().m_StandardShader.BeginGenerateDepthMapFromLight();
	{
		for (auto& obj : m_objList)
		{
			obj->GenerateDepthMapFromLight();
		}
	}
	KdShaderManager::Instance().m_StandardShader.EndGenerateDepthMapFromLight();

	// ===== ===== ===== ===== ===== ===== ===== ===== ===== ===== ===== =====
	//【陰影なし描画】
	//ライティング計算を行わないオブジェクト(UI的な3D物体や地面のグリッドなど)を描画
	KdShaderManager::Instance().m_StandardShader.BeginUnLit();
	{
		for (auto& obj : m_objList)
		{
			obj->DrawUnLit();
		}
	}
	KdShaderManager::Instance().m_StandardShader.EndUnLit();

	// ===== ===== ===== ===== ===== ===== ===== ===== ===== ===== ===== =====
	//【通常描画】
	//光の影響を受ける標準的なオブジェクトを描画
	KdShaderManager::Instance().m_StandardShader.BeginLit();
	{

		for (auto& obj : m_objList)
		{
			obj->DrawLit();
		}
	}
	KdShaderManager::Instance().m_StandardShader.EndLit();

	// ===== ===== ===== ===== ===== ===== ===== ===== ===== ===== ===== =====
	//【発光描画】
	//Bloom(光溢れ)エフェクトの対象となる、高輝度なオブジェクトを描画
	KdShaderManager::Instance().m_postProcessShader.BeginBright();
	{
		for (auto& obj : m_objList)
		{
			obj->DrawBright();
		}
	}
	KdShaderManager::Instance().m_postProcessShader.EndBright();
}

void BaseScene::PostDraw()
{
	for (auto& obj : m_objList)
	{
		obj->PostDraw();
	}
}

void BaseScene::DrawSprite()
{
	//2D描画(UIなど)
	KdShaderManager::Instance().m_spriteShader.Begin();
	for (auto& obj : m_objList)
	{
		obj->DrawSprite();
	}
	KdShaderManager::Instance().m_spriteShader.End();
}

void BaseScene::AddObject(const std::shared_ptr<GameObject>& obj)
{
	if (obj)
	{
		m_objList.push_back(obj);
	}
}

void BaseScene::SetActiveCamera(const std::shared_ptr<ICameraComponent>& camera)
{
	if (camera)
	{
		m_wpActiveCamera = camera;
	}
}

std::shared_ptr<ICameraComponent> BaseScene::GetActiveCamera()
{
	return m_wpActiveCamera.lock();
}

std::shared_ptr<GameObject> BaseScene::FindObject(std::string objName)const
{
	auto it = std::find_if(m_objList.begin(), m_objList.end(), [&](const std::shared_ptr<GameObject>& obj)
		{
			return obj && obj->GetName() == objName;
		}
	);

	if (it != m_objList.end())
	{
		return *it;
	}

	return nullptr;
}