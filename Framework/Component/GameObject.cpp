#include "GameObject.h"
#include"Component.h"
#include"../Src/Application/GameData/BlockState/BlockState.h"
#include"../Src/Framework/GameObject/ArchetypeManager.h"
#include"BlockDataComponent/BlockDataComponent.h"
#include"IdComponent/IdComponent.h"
#include"TransformComponent/TransformComponent.h"
#include"RenderComponent/RenderComponent.h"
#include"ColliderComponent/ColliderComponent.h"
#include"MovingBlockComponent/MovingBlockComponent.h"
#include"TransferBlockComponent/TransferBlockComponent.h"
#include"JumpBlockComponent/JumpBlockComponent.h"
#include"RotatingBlockComponent/RotatingBlockComponent.h"
#include"SinkingBlockComponent/SinkingBlockComponent.h"
#include"SlipperyComponent/SlipperyComponent.h"
#include"ScalingBlockComponent/ScalingBlockComponent.h"
#include"MagicCircleComponent/MagicCircleComponent.h"
#include"ParticleEmitterComponent/ParticleEmitterComponent.h"
#include"DissolveComponent/DissolveComponent.h"
#include"GlowPartComponent/GlowPartComponent.h"
#include"../Component/GearRotateComponent/GearRotateComponent.h"
#include"../Component/BouncerComponent/BouncerComponent.h"

void GameObject::Init()
{
	//コンポーネントのUpdate順序を優先度に従ってソート(例: Transformは最初、Physicsは最後など)
	std::sort(m_component.begin(), m_component.end(), [](const auto& a, const auto& b) {
		return a->GetUpdatePriority() < b->GetUpdatePriority();
		});

	for (auto& comp : m_component)
	{
		comp->Awake();
	}

	for (auto& comp : m_component)
	{
		comp->Start();
	}
}

void GameObject::PreUpdate()
{
	for (auto& comp : m_component)
	{
		comp->PreUpdate();
	}
}

void GameObject::Update()
{
	for (auto& comp : m_component)
	{
		comp->Update();
	}
}

void GameObject::PostUpdate()
{
	for (auto& comp : m_component)
	{
		comp->PostUpdate();
	}
}

void GameObject::GenerateDepthMapFromLight()
{
	for (auto& comp : m_component)
	{
		comp->GenerateDepthMapFromLight();
	}
}

void GameObject::PreDraw()
{
	for (auto& comp : m_component)
	{
		comp->PreDraw();
	}
}

void GameObject::DrawLit()
{
	for (auto& comp : m_component)
	{
		comp->DrawLit();
	}
}

void GameObject::DrawUnLit()
{
	for (auto& comp : m_component)
	{
		comp->DrawUnLit();
	}
}

void GameObject::DrawBright()
{
	for (auto& comp : m_component)
	{
		comp->DrawBright();
	}
}

void GameObject::DrawSprite()
{
	for (auto& comp : m_component)
	{
		comp->DrawSprite();
	}
}

void GameObject::PostDraw()
{
	for (auto& comp : m_component)
	{
		comp->PostDraw();
	}
}

void GameObject::DrawDebug()
{
	for (auto& comp : m_component)
	{
		comp->DrawDebug();
	}
	if (!m_pDebugWire)return;

	m_pDebugWire->Draw();
}

void GameObject::AddComponent(const std::shared_ptr<Component>& component)
{
	if (component)
	{
		// 1. 所有者を設定する
		component->m_owner = this;
		// 2. リストに追加する
		m_component.push_back(component);
	}
}

nlohmann::json GameObject::ToJson() const
{
	nlohmann::json entityData;

	//--基本情報--
	std::string objectName = GetName();
	std::string archetypeName = GetArchetypeName();

	entityData["archetype"] = archetypeName;
	entityData["name"] = objectName;

	//--各コンポーネントデータ--
	auto& compData = entityData["components"];
	for (auto comp : m_component)
	{
		nlohmann::json compJson = comp->ToJson();
		if (!compJson.empty())
		{
			std::string compName = comp->GetComponentName();
			compData[compName] = compJson;
		}
	}
	return entityData;
}

std::string GameObject::GetArchetypeName() const
{
	std::string name = GetName();
	size_t underbarPos = name.find_last_of('_');
	if (underbarPos != std::string::npos)
	{
		return name.substr(0, underbarPos);
	}

	return name;
}

//現在のGameObjectの状態をBlockState構造体として保存(シリアライズ)
BlockState GameObject::CreateState() const
{
	BlockState outState;

	//--基本情報--
	outState.archetypeName = GetArchetypeName();
	if (auto idComp = GetComponent<IdComponent>())
	{
		outState.entityId = idComp->GetId();
	}

	const Archetype* archetype = ArchetypeManager::Instance().GetArchetype(outState.archetypeName);
	if (archetype)
	{
		outState.isSwappable = archetype->GetSwapp();
	}

	//--Transform--
	if (auto comp = GetComponent<TransformComponent>())
	{
		outState.pos = comp->GetPos();
		outState.rot = comp->GetRot();
		outState.scale = comp->GetScale();
	}

	//--BlockData--
	if (auto comp = GetComponent<BlockDataComponent>())
	{
		outState.type = comp->GetType();
	}

	//--Render--
	if (auto comp = GetComponent<RenderComponent>())
	{
		outState.renderModelPath = comp->GetModelPath();
	}

	//以下はコンポーネントを持っている場合のみ生成して代入
	for (const auto& baseComp : m_component)
	{
		//--Rigidbody--
		if (auto comp = std::dynamic_pointer_cast<RigidbodyComponent>(baseComp))
		{
			RigidbodyData data;
			data.type = comp->m_type;
			outState.rigidbody = data;
		}

		//--Collider--
		if (auto comp = std::dynamic_pointer_cast<ColliderComponent>(baseComp))
		{
			Shape* shape = comp->GetShape();
			if (shape)
			{
				ColliderData data;
				data.shapeType = shape->GetType();
				data.offset = shape->m_offset;

				//各シェイプの情報取得
				if (auto s = dynamic_cast<SphereShape*>(shape))
				{
					data.radius = s->m_radius;
				}
				else if (auto b = dynamic_cast<BoxShape*>(shape))
				{
					data.extents = b->m_extents;
				}
				else if (auto m = dynamic_cast<MeshShape*>(shape))
				{
					if (m->m_spModel) data.modelPath = m->m_spModel->GetFilePath();
				}
				else if (auto p = dynamic_cast<PolygonShape*>(shape))
				{
					if (p->m_spModel) data.modelPath = p->m_spModel->GetFilePath();
				}

				outState.collider = data;
			}
		}

		//--Moving--
		if (auto comp = std::dynamic_pointer_cast<MovingBlockComponent>(baseComp))
		{
			MovingBlockData data;
			data.startPos = comp->GetStartPos();
			data.endPos = comp->GetEndPos();
			data.duration = comp->GetDuration();
			outState.moving = data;
		}

		//--Transfer--
		if (auto comp = std::dynamic_pointer_cast<TransferBlockComponent>(baseComp))
		{
			TransferBlockData data;
			data.transferID = comp->GetTransferID();
			outState.transfer = data;
		}

		//--Jump--
		if (auto comp = std::dynamic_pointer_cast<JumpBlockComponent>(baseComp))
		{
			JumpBlockData data;
			data.direction = comp->GetJumpDirection();
			data.force = comp->GetJumpForce();
			data.duration = comp->GetChargeDuration();
			outState.jump = data;
		}

		//--Rotating--
		if (auto comp = std::dynamic_pointer_cast<RotatingBlockComponent>(baseComp))
		{
			RotatingBlockData data;
			data.axis = comp->GetRotatingAxis();
			data.amount = comp->GetRotatingAmount();
			data.speed = comp->GetRotatingSpeed();
			outState.rotating = data;
		}

		//--Sinking--
		if (auto comp = std::dynamic_pointer_cast<SinkingBlockComponent>(baseComp))
		{
			SinkingBlockData data;
			data.initialPos = comp->GetInitialPos();
			data.maxSinkDistance = comp->GetMaxSinkDistance();
			data.acceleration = comp->GetAcceleration();
			data.riseSpeed = comp->GetRiseSpeed();
			outState.sinking = data;
		}

		//--Slippery--
		if (auto comp = std::dynamic_pointer_cast<SlipperyComponent>(baseComp))
		{
			SlipperyBlockData data;
			data.slipperyDrag = comp->GetDragCoefficient();
			outState.slippery = data;
		}

		//--Scaling--
		if (auto comp = std::dynamic_pointer_cast<ScalingBlockComponent>(baseComp))
		{
			ScalingBlockData data;
			data.axis = comp->GetScaleAxis();
			data.amount = comp->GetscaleAmount();
			data.speed = comp->GetscaleSpeed();
			outState.scaling = data;
		}

		//--MagicCircle--
		if (auto comp = std::dynamic_pointer_cast<MagicCircleComponent>(baseComp))
		{
			MagicCircleData data;
			data.modelPath = comp->GetModelPath();
			data.localPos = comp->GetLocalPos();
			data.localRot = comp->GetLocalRot();
			data.localScale = comp->GetLocalScale();
			data.orbitRadius = comp->GetOrbitRadius();
			data.orbitSpeed = comp->GetOrbitSpeed();;
			data.orbitAxisOffset = comp->GetOrbitAxisOffset();
			data.normalSpeed = comp->GetNormalSpeed();
			data.selectedSpeed = comp->GetSelectedSpeed();
			data.selectedScaleMultiplier = comp->GetScaleMultiplier();
			data.scaleLerpSpeed = comp->GetScaleLerpSpeed();
			outState.magicCircle = data;
		}

		//--ParticleEmitter--
		if (auto comp = std::dynamic_pointer_cast<ParticleEmitterComponent>(baseComp))
		{
			EmitterData data;
			data.systemName = comp->GetSystemName();
			data.count = comp->GetEmitCount();
			data.frequency = comp->GetEmitFrequency();
			data.baseDirection = comp->GetBaseDirection();
			data.spread = comp->GetSpread();
			data.offsets = comp->GetOffsets();
			outState.emitter = data;
		}

		//--Dissolve--
		if (auto comp = std::dynamic_pointer_cast<DissolveComponent>(baseComp))
		{
			DissolveData data;
			data.fadeDuration = comp->GetFadeDuration();
			data.edgeColor = comp->GetEdgeColor();
			data.edgeRange = comp->GetEdgeRange();
			data.blockResolution = comp->GetBlockResolution();
			outState.dissolve = data;
		}

		//--Glow--
		if (auto comp = std::dynamic_pointer_cast<GlowPartComponent>(baseComp))
		{
			GlowPartData data;
			data.modelPath = comp->GetModelPath();
			data.color = comp->GetGlowColor();
			data.enableFloat = comp->GetEnableFloat();
			data.enableBlink = comp->GetEnableBlink();
			data.enableRotate = comp->GetEnableRotate();
			data.isDirectional = comp->GetIsDirectional();
			data.floatSpeed = comp->GetFloatSpeed();
			data.floatDirection = comp->GetFloatDirection();
			data.blinkSpeed = comp->GetBlinkSpeed();
			data.rotateSpeed = comp->GetRotateSpeed();
			data.rotateAxis = comp->GetRotateAxis();

			data.instances = comp->GetGlowInstances();

			outState.glow = data;
		}

		//--Gear--
		if (auto comp = std::dynamic_pointer_cast<GearRotateComponent>(baseComp))
		{
			GearRotateComponentData data;
			data.modelPath = comp->GetModelPath();
			data.speed = comp->GetRotationSpeed();

			data.gears = comp->GetGears();

			outState.gearRotate = data;
		}

		//--Bouncer--
		if (auto comp = std::dynamic_pointer_cast<BouncerComponent>(baseComp)) {
			BouncerData data;
			data.modelPath = comp->GetModelPath();
			data.offset = comp->GetOffset();
			data.stroke = comp->GetStroke();
			outState.bouncer = data;
		}
	}

	return outState;
}

//BlockState構造体の内容をGameObjectに適用(復元)
void GameObject::ApplyState(const BlockState& state)
{
	if (auto comp = GetComponent<TransformComponent>())
	{
		comp->SetPos(state.pos);
		comp->SetRot(state.rot);
		comp->SetScale(state.scale);
	}

	if (auto comp = GetComponent<BlockDataComponent>())
	{
		comp->SetType(state.type);
	}

	if (auto comp = GetComponent<RenderComponent>())
	{
		auto model = KdAssets::Instance().m_modeldatas.GetData(state.renderModelPath);
		comp->SetModel(model);
	}

	//以下はコンポーネントを持っていれば適用
	//--Rigidbody--
	if (state.rigidbody.has_value())
	{
		if (auto comp = GetComponent<RigidbodyComponent>())
		{
			const auto& data = state.rigidbody.value();
			comp->m_type = data.type;
			comp->SetVelocity(Math::Vector3::Zero);//状態適用時は速度リセット(吹き飛ばないように)
		}
	}

	//--Collider--
	if (state.collider.has_value())
	{
		if (auto comp = GetComponent<ColliderComponent>())
		{
			const auto& data = state.collider.value();

			switch (data.shapeType)
			{
			case Shape::Type::Sphere:
			{
				comp->SetShapeAsShpere(data.radius, data.offset);
				break;
			}
			case Shape::Type::Box:
			{
				if (!data.modelPath.empty())
				{
					auto colModel = KdAssets::Instance().m_modeldatas.GetData(data.modelPath);
					comp->SetShapeAsBoxFromModel(colModel);
				}
				break;
			}
			case Shape::Type::Mesh:
			{
				if (!data.modelPath.empty())
				{
					auto colModel = KdAssets::Instance().m_modeldatas.GetData(data.modelPath);
					comp->SetShapeAsMesh(colModel);
				}
				break;
			}
			case Shape::Type::Polygon:
			{
				if (!data.modelPath.empty())
				{
					auto colModel = KdAssets::Instance().m_modeldatas.GetData(data.modelPath);
					comp->SetShapeAsPolygon(colModel);
				}
				break;
			}
			}
		}
	}

	//--Moving--
	if (state.moving.has_value())
	{
		if (auto comp = GetComponent<MovingBlockComponent>())
		{
			const auto& data = state.moving.value();

			Math::Vector3 startPos = data.startPos;
			Math::Vector3 endPos = data.endPos;
			comp->SetActive(true);
			comp->SetStartPos(startPos);
			comp->SetEndPos(endPos);
			comp->SetDuration(data.duration);
			comp->ResetProgress();
		}
	}
	else
	{
		if (auto comp = GetComponent<MovingBlockComponent>())
		{
			comp->SetActive(false);
		}
	}

	//--Transfer--
	if (state.transfer.has_value())
	{
		if (auto comp = GetComponent<TransferBlockComponent>())
		{
			const auto& data = state.transfer.value();

			comp->SetTransferID(data.transferID);
		}
	}

	//--Jump--
	if (state.jump.has_value())
	{
		if (auto comp = GetComponent<JumpBlockComponent>())
		{
			const auto& data = state.jump.value();

			comp->SetJumpDirection(data.direction);
			comp->SetJumpForce(data.force);
			comp->SetChargeDuration(data.duration);
		}
	}

	//--Rotating--
	if (state.rotating.has_value())
	{
		if (auto comp = GetComponent<RotatingBlockComponent>())
		{
			const auto& data = state.rotating.value();

			comp->SetRotatingAxis(data.axis);
			comp->SetRotatingAmount(data.amount);
			comp->SetRotatingSpeed(data.speed);
		}
	}

	//--Sinking--
	if (state.sinking.has_value())
	{
		if (auto comp = GetComponent<SinkingBlockComponent>())
		{
			const auto& data = state.sinking.value();

			comp->SetInitialPos(data.initialPos);
			comp->SetMaxSinkDistance(data.maxSinkDistance);
			comp->SetAcceleration(data.acceleration);
			comp->SetRiseSpeed(data.riseSpeed);
		}
	}

	//--Slippery--
	if (state.slippery.has_value())
	{
		if (auto comp = GetComponent<SlipperyComponent>())
		{
			const auto& data = state.slippery.value();

			comp->SetDragCoefficient(data.slipperyDrag);
		}
	}

	//--Scaling--
	if (state.scaling.has_value())
	{
		if (auto comp = GetComponent<ScalingBlockComponent>())
		{
			const auto& data = state.scaling.value();

			comp->SetScaleAxis(data.axis);
			comp->SetScaleAmount(data.amount);
			comp->SetScaleSpeed(data.speed);
		}
	}

	//--MagicCircle--
	if (state.magicCircle.has_value())
	{
		if (auto comp = GetComponent<MagicCircleComponent>())
		{
			const auto& data = state.magicCircle.value();

			comp->SetLocalPos(data.localPos);
			comp->SetLocalRot(data.localRot);
			comp->SetLocalScale(data.localScale);
			comp->SetOrbitRadius(data.orbitRadius);
			comp->SetOrbitSpeed(data.orbitSpeed);
			comp->SetOrbitAxisOffset(data.orbitAxisOffset);
			comp->SetNormalSpeed(data.normalSpeed);
			comp->SetSelectedSpeed(data.selectedSpeed);
			comp->SetScaleMultiplier(data.selectedScaleMultiplier);
			comp->SetScaleLerpSpeed(data.scaleLerpSpeed);
		}
	}

	//--ParticleEmitter--
	if (state.emitter.has_value())
	{
		if (auto comp = GetComponent<ParticleEmitterComponent>())
		{
			const auto& data = state.emitter.value();

			comp->SetSystemName(data.systemName);
			comp->SetEmitCount(data.count);
			comp->SetEmitFrequency(data.frequency);
			comp->SetBaseDirection(data.baseDirection);
			comp->SetSpread(data.spread);
			comp->SetOffsets(data.offsets);
		}
	}

	//--Dissolce--
	if (state.dissolve.has_value())
	{
		if (auto comp = GetComponent<DissolveComponent>())
		{
			const auto& data = state.dissolve.value();

			comp->SetFadeDuration(data.fadeDuration);
			comp->SetEdgeColor(data.edgeColor);
			comp->SetEdgeRange(data.edgeRange);
			comp->SetBlockResolution(data.blockResolution);
		}
	}

	//--Glow--
	if (state.glow.has_value())
	{
		if (auto comp = GetComponent<GlowPartComponent>())
		{
			const auto& data = state.glow.value();

			comp->SetModel(data.modelPath);
			comp->SetGlowColor(data.color);
			comp->SetEnableFloat(data.enableFloat);
			comp->SetEnableBlink(data.enableBlink);
			comp->SetEnableRotate(data.enableRotate);
			comp->SetIsDirectional(data.isDirectional);
			comp->SetFloatSpeed(data.floatSpeed);
			comp->SetFloatDirection(data.floatDirection);
			comp->SetBlinkSpeed(data.blinkSpeed);
			comp->SetRotateSpeed(data.rotateSpeed);
			comp->SetRotateAxis(data.rotateAxis);

			comp->SetGlowInstances(data.instances);
		}
	}

	//--Gear--
	if (state.gearRotate.has_value())
	{
		if (auto comp = GetComponent<GearRotateComponent>())
		{
			const auto& data = state.gearRotate.value();

			comp->SetModel(data.modelPath);
			comp->SetRotationSpeed(data.speed);
			comp->SetGears(data.gears);
		}
	}

	//--Bouncer--
	if (state.bouncer.has_value()) {
		if (auto comp = GetComponent<BouncerComponent>()) {
			const auto& data = state.bouncer.value();
			comp->SetModel(data.modelPath);
			comp->SetOffset(data.offset);
			comp->SetStroke(data.stroke);
		}
	}
}