#pragma once
#include"../Src/Application/GameData/BlockState/BlockState.h"
#include"../Src/Application/JsonHelper/JsonHelper.h"

//nlohmann::jsonがVector3を認識できるようにするための変換定義
namespace DirectX::SimpleMath {

	inline void to_json(nlohmann::json& j, const Vector3& v) {
		j = { v.x, v.y, v.z };
	}

	inline void from_json(const nlohmann::json& j, Vector3& v) {
		if (j.is_array() && j.size() == 3) {
			j.at(0).get_to(v.x);
			j.at(1).get_to(v.y);
			j.at(2).get_to(v.z);
		}
	}

}

//--各種enumと文字列の相互変換マクロ--
//BlockType
NLOHMANN_JSON_SERIALIZE_ENUM(BlockType, {
	{BlockType::None, "None"},
	{BlockType::Movable, "Movable"},
	{BlockType::Wall, "Wall"},
	{BlockType::Goal, "Goal"},
	{BlockType::Moving, "Moving"},
	{BlockType::Transfer, "Transfer"},
	{BlockType::Slippery, "Slippery"},
	{BlockType::Rotating, "Rotating"},
	{BlockType::Sinking, "Sinking"},
	{BlockType::Scaling, "Scaling"},
	{BlockType::Checkpoint, "Checkpoint"},
	{BlockType::TutorialTrigger, "TutorialTrigger"}
	})

	//ShapeType
	NLOHMANN_JSON_SERIALIZE_ENUM(Shape::Type, {
		{Shape::Type::Sphere, "sphere"},
		{Shape::Type::Box, "box"},
		{Shape::Type::Mesh, "mesh"},
		{Shape::Type::Polygon, "polygon"}
		})

	//RigidbodyType
	NLOHMANN_JSON_SERIALIZE_ENUM(RigidbodyType, {
		{RigidbodyType::Dynamic, "dynamic"},
		{RigidbodyType::Kinematic, "kinematic"},
		{RigidbodyType::Static, "static"}
		})

	//--BlockState構造体をJsonオブジェクトへ変換(シリアライズ)--
	inline void to_json(nlohmann::json& j, const BlockState& p)
{
	j = nlohmann::json{
		{"archetypeName",p.archetypeName},
		{"entityId",p.entityId},
		{"isSwappable", p.isSwappable},
		{"tag", JsonHelper::SetGameObjectTag(p.tag)}
	};

	//コンポーネントごとのデータを格納するオブジェクトを作成
	auto& components = j["components"];

	//基本コンポーネント
	components["TransformComponent"] = {
		{"position", p.pos},
		{"rotation", p.rot},
		{"scale",    p.scale}
	};

	components["BlockDataComponent"] = {
		{"type", p.type}
	};

	components["RenderComponent"] = {
		{"model", p.renderModelPath}
	};

	//以下はOptionalな値を持っている場合のみJsonに出力してファイルサイズを削減
	if (p.collider.has_value())
	{
		const auto& data = p.collider.value();
		components["ColliderComponent"] = {
			{"shape", data.shapeType},
			{"model", data.modelPath},
			{"radius", data.radius},
			{"extents", data.extents},
			{"offset", data.offset},
			{"isTrigger", data.isTrigger}
		};
	}

	if (p.rigidbody.has_value())
	{
		const auto& data = p.rigidbody.value();

		components["RigidbodyComponent"] = {
			{"type", data.type}
		};
	}

	if (p.moving.has_value())
	{
		const auto& data = p.moving.value();

		components["MovingBlockComponent"] = {
			{"startPos", data.startPos},
			{"endPos",   data.endPos},
			{"duration", data.duration},
			{"active",   true}//データがあるならactiveと見なす
		};
	}

	if (p.transfer.has_value())
	{
		const auto& data = p.transfer.value();

		components["TransferBlockComponent"] = {
			{"transferID", data.transferID}
		};
	}

	if (p.jump.has_value())
	{
		const auto& data = p.jump.value();

		components["JumpBlockComponent"] = {
			{"jumpDirection",data.direction},
			{"jumpForce",data.force},
			{"jumpDuration",data.duration}
		};
	}

	if (p.rotating.has_value())
	{
		const auto& data = p.rotating.value();

		components["RotatingBlockComponent"] = {
			{"rotation_axis", data.axis},
			{"rotation_amount", data.amount},
			{"rotation_speed", data.speed}
		};
	}

	if (p.sinking.has_value())
	{
		const auto& data = p.sinking.value();

		components["SinkingBlockComponent"] = {
			{"initial_pos", data.initialPos},
			{"max_sink_distance", data.maxSinkDistance},
			{"acceleration", data.acceleration},
			{"rise_speed", data.riseSpeed}
		};
	}

	if (p.slippery.has_value())
	{
		const auto& data = p.slippery.value();

		components["SlipperyComponent"] = {
			{"drag_coefficient", data.slipperyDrag}
		};
	}

	if (p.scaling.has_value())
	{
		const auto& data = p.scaling.value();

		components["ScalingBlockComponent"] = {
			{"scale_axis",data.axis},
			{"scale_amount",data.amount},
			{"scale_speed",data.speed}
		};
	}

	if (p.tutorial.has_value())
	{
		const auto& data = p.tutorial.value();

		components["TutorialTriggerComponent"] = {
			{"BlockName", data.blockName},
			{"Text", data.text},
			{"ImagePath", data.imagePath}
		};
	}

	if (p.magicCircle.has_value())
	{
		const auto& data = p.magicCircle.value();

		components["MagicCircleComponent"] = {
			{"model", data.modelPath},
			{"localPos", data.localPos},
			{"localRot", data.localRot},
			{"localScale",data.localScale},
			{"orbitRadius",data.orbitRadius},
			{"orbitSpeed",data.orbitSpeed},
			{"orbitAxisOffset",data.orbitAxisOffset},
			{"normalSpeed",data.normalSpeed},
			{"selectedSpeed",data.selectedSpeed},
			{"selectedScaleMultiplier",data.selectedScaleMultiplier},
			{"scaleLerpSpeed",data.scaleLerpSpeed}
		};
	}

	if (p.emitter.has_value())
	{
		const auto& data = p.emitter.value();

		components["ParticleEmitterComponent"] = {
			{"system_name", data.systemName},
			{"emit_count", data.count},
			{"emit_frequency", data.frequency},
			{"base_direction", data.baseDirection},
			{"spread", data.spread},
			{"offsets", data.offsets}
		};
	}

	if (p.dissolve.has_value())
	{
		const auto& data = p.dissolve.value();

		components["DissolveComponent"] = {
			{"fadeDuration", data.fadeDuration},
			{"edgeColor", data.edgeColor},
			{"edgeRange", data.edgeRange},
			{"blockResolution", data.blockResolution}
		};
	}

	if (p.glow.has_value())
	{
		const auto& data = p.glow.value();
		nlohmann::json compJson;

		compJson["model"] = data.modelPath;
		compJson["glowColor"] = data.color;

		compJson["enableFloat"] = data.enableFloat;
		compJson["enableBlink"] = data.enableBlink;
		compJson["enableRotate"] = data.enableRotate;
		compJson["isDirectional"] = data.isDirectional;
		compJson["floatSpeed"] = data.floatSpeed;
		compJson["floatDirection"] = data.floatDirection;
		compJson["blinkSpeed"] = data.blinkSpeed;
		compJson["rotateSpeed"] = data.rotateSpeed;
		compJson["rotateAxis"] = data.rotateAxis;

		nlohmann::json instArry = nlohmann::json::array();

		for (const auto& inst : data.instances)
		{
			nlohmann::json i;
			i["name"] = inst.name;
			i["offset"] = inst.offset;
			i["baseRot"] = inst.baseRotation;
			i["scale"] = inst.scale;
			instArry.push_back(i);
		}
		compJson["instances"] = instArry;

		components["GlowPartComponent"] = compJson;
	}

	if (p.gearRotate.has_value())
	{
		const auto& data = p.gearRotate.value();

		nlohmann::json compJson;
		compJson["model"] = data.modelPath;
		compJson["speed"] = data.speed;

		nlohmann::json gearsJson = nlohmann::json::array();
		for (const auto& gear : data.gears)
		{
			nlohmann::json g;
			g["name"] = gear.name;
			g["offset"] = gear.offset;
			g["axis"] = gear.rotationAxis;
			g["baseRot"] = gear.baseRotation;
			g["startAngle"] = gear.startAngle;
			g["reverse"] = gear.reverse;
			g["scale"] = gear.scale;
			gearsJson.push_back(g);
		}
		compJson["gears"] = gearsJson;

		components["GearRotateComponent"] = compJson;
	}

	if (p.bouncer.has_value()) {
		const auto& data = p.bouncer.value();
		components["BouncerComponent"] = {
			{"model", data.modelPath},
			{"offset", data.offset},
			{"stroke", data.stroke}
		};
	}
}

//--JsonオブジェクトをBlockState構造体へ変換(デシリアライズ)--
inline void from_json(const nlohmann::json& j, BlockState& p)
{
	p.archetypeName = j.value("archetypeName", "");
	p.entityId = j.value("entityId", 0);
	p.isSwappable = j.value("swappable", false);
	p.tag = JsonHelper::GetGameObjectTag(j, "tag", GameObject::Tag::None);

	//componentsの欄があるかチェックし、各コンポーネントデータを読み込む
	if (j.contains("components"))
	{
		const auto& components = j.at("components");

		if (components.contains("TransformComponent"))
		{
			const auto& data = components.at("TransformComponent");
			p.pos = data.value("position", Math::Vector3::Zero);
			p.rot = data.value("rotation", Math::Vector3::Zero);
			p.scale = data.value("scale", Math::Vector3::One);
		}

		if (components.contains("BlockDataComponent"))
		{
			const auto& data = components.at("BlockDataComponent");
			p.type = data.value("type", BlockType::None);
		}

		if (components.contains("RenderComponent"))
		{
			p.renderModelPath = components.at("RenderComponent").value("model", "");
		}

		if (components.contains("ColliderComponent"))
		{
			const auto& data = components.at("ColliderComponent");
			ColliderData cData;
			cData.shapeType = data.value("shape", Shape::Type::Mesh);
			cData.modelPath = data.value("model", "");
			cData.radius = data.value("radius", 0.5f);
			cData.extents = data.value("extents", Math::Vector3(0.5f, 0.5f, 0.5f));
			cData.offset = data.value("offset", Math::Vector3::Zero);
			cData.isTrigger = data.value("isTrigger", false);
			p.collider = cData;
		}

		if (components.contains("RigidbodyComponent"))
		{
			const auto& data = components.at("RigidbodyComponent");
			RigidbodyData rData;
			rData.type = data.value("type", RigidbodyType::Static);
			p.rigidbody = rData;
		}

		if (components.contains("MovingBlockComponent"))
		{
			const auto& data = components.at("MovingBlockComponent");
			MovingBlockData mData;
			mData.startPos = data.value("startPos", Math::Vector3::Zero);
			mData.endPos = data.value("endPos", Math::Vector3::Zero);
			mData.duration = data.value("duration", 2.0f);
			p.moving = mData;
		}

		if (components.contains("TransferBlockComponent"))
		{
			TransferBlockData tData;
			tData.transferID = components.at("TransferBlockComponent").value("transferID", 0);
			p.transfer = tData;
		}

		if (components.contains("SlipperyComponent"))
		{
			const auto& data = components.at("SlipperyComponent");
			SlipperyBlockData sliData;
			sliData.slipperyDrag = data.value("drag_coefficient", 1.0f);
			p.slippery = sliData;
		}

		if (components.contains("JumpBlockComponent"))
		{
			const auto& data = components.at("JumpBlockComponent");
			JumpBlockData jData;
			jData.direction = data.value("jumpDirection", Math::Vector3(0, 1, 0));
			jData.force = data.value("jumpForce", 0.0f);
			jData.duration = data.value("jumpDuration", 0.5);
			p.jump = jData;
		}

		if (components.contains("RotatingBlockComponent"))
		{
			const auto& data = components.at("RotatingBlockComponent");
			RotatingBlockData rData;
			rData.axis = data.value("rotation_axis", Math::Vector3(0, 1, 0));
			rData.amount = data.value("rotation_amount", 0.0f);
			rData.speed = data.value("rotation_speed", 0.0f);
			p.rotating = rData;
		}

		if (components.contains("SinkingBlockComponent"))
		{
			const auto& data = components.at("SinkingBlockComponent");
			SinkingBlockData sData;
			sData.initialPos = data.value("initial_pos", Math::Vector3::Zero);
			sData.maxSinkDistance = data.value("max_sink_distance", 0.0f);
			sData.acceleration = data.value("acceleration", 0.0f);
			sData.riseSpeed = data.value("rise_speed", 0.0f);
			p.sinking = sData;
		}

		if (components.contains("ScalingBlockComponent"))
		{
			const auto& data = components.at("ScalingBlockComponent");
			ScalingBlockData sData;
			sData.axis = data.value("scale_axis", Math::Vector3{ 0.0,1.0,0.0 });
			sData.amount = data.value("scale_amount", 0.5);
			sData.speed = data.value("scale_speed", 2.0);
			p.scaling = sData;
		}

		if (components.contains("TutorialTriggerComponent"))
		{
			const auto& data = components.at("TutorialTriggerComponent");
			TutorialData tData;
			tData.blockName = data.value("BlockName", "");
			tData.text = data.value("Text", "");
			tData.imagePath = data.value("ImagePath", "");
			p.tutorial = tData;
		}

		if (components.contains("MagicCircleComponent"))
		{
			const auto& data = components.at("MagicCircleComponent");
			MagicCircleData mData;
			mData.localPos = data.value("localPos", Math::Vector3::Zero);
			mData.localRot = data.value("localRot", Math::Vector3::Zero);
			mData.localScale = data.value("localScale", Math::Vector3::One);
			mData.orbitRadius = data.value("orbitRadius", 1.5f);
			mData.orbitSpeed = data.value("orbitSpeed", 1.0f);
			mData.orbitAxisOffset = data.value("orbitAxisOffset", Math::Vector3::Zero);
			mData.normalSpeed = data.value("normalSpeed", 90.0f);
			mData.selectedSpeed = data.value("selectedSpeed", 360.0f);
			mData.selectedScaleMultiplier = data.value("selectedScaleMultiplier", 1.5f);
			mData.scaleLerpSpeed = data.value("scaleLerpSpeed", 10.0f);
			p.magicCircle = mData;
		}

		if (components.contains("ParticleEmitterComponent"))
		{
			const auto& data = components.at("ParticleEmitterComponent");
			EmitterData eData;
			eData.systemName = data.value("system_name", "ColdAir");
			eData.count = data.value("emit_count", 3);
			eData.frequency = data.value("emit_frequency", 0.1f);
			eData.baseDirection = data.value("base_direction", Math::Vector3(0, 1, 0));
			eData.spread = data.value("spread", 0.3f);
			if (data.contains("offsets") && data.at("offsets").is_array())
			{
				eData.offsets = data.at("offsets").get<std::vector<Math::Vector3>>();
			}
			else if (data.contains("offset"))
			{
				eData.offsets.push_back(data.value("offset", Math::Vector3::Zero));
			}

			if (eData.offsets.empty())
			{
				eData.offsets.push_back(Math::Vector3::Zero);
			}
			p.emitter = eData;
		}

		if (components.contains("DissolveComponent"))
		{
			const auto& data = components.at("DissolveComponent");
			DissolveData dData;
			dData.fadeDuration = data.value("fadeDuration", 0.3);
			dData.edgeColor = data.value("edgeColor", Math::Vector3(1.0, 1.0, 1.0));
			dData.edgeRange = data.value("edgeRange", 0.01);
			dData.blockResolution = data.value("blockResolution", 4.0);
			p.dissolve = dData;
		}

		if (components.contains("GlowPartComponent"))
		{
			const auto& data = components.at("GlowPartComponent");
			GlowPartData gCompData;

			gCompData.modelPath = data.value("model", "");
			gCompData.color = data.value("glowColor", Math::Vector3(0, 0, 0));
			gCompData.enableFloat = data.value("enableFloat", false);
			gCompData.enableBlink = data.value("enableBlink", false);
			gCompData.enableRotate = data.value("enableRotate", false);
			gCompData.isDirectional = data.value("isDirectional", false);
			gCompData.floatSpeed = data.value("floatSpeed", 5.0);
			gCompData.floatDirection = data.value("floatDirection", Math::Vector3(0, 1, 0));
			gCompData.blinkSpeed = data.value("blinkSpeed", 5.0);
			gCompData.rotateSpeed = data.value("rotateSpeed", 90);
			gCompData.rotateAxis = data.value("rotateAxis", Math::Vector3(0, 1, 0));

			if (data.contains("instances") && data["instances"].is_array())
			{
				for (const auto& i : data["instances"])
				{
					GlowInstanceData inst;
					inst.name = i.value("name", "Glow");
					inst.offset = i.value("offset", Math::Vector3::Zero);
					inst.baseRotation = i.value("baseRot", Math::Vector3::Zero);
					inst.scale = i.value("scale", Math::Vector3(1, 1, 1));
					gCompData.instances.push_back(inst);
				}
			}
			p.glow = gCompData;
		}

		if (components.contains("GearRotateComponent"))
		{
			const auto& data = components.at("GearRotateComponent");
			GearRotateComponentData gCompData;

			gCompData.modelPath = data.value("model", "");
			gCompData.speed = data.value("speed", 90.0);

			if (data.contains("gears") && data["gears"].is_array())
			{
				for (const auto& gearJson : data["gears"])
				{
					GearData gear;

					gear.name = gearJson.value("name", "Gear");
					gear.offset = gearJson.value("offset", Math::Vector3::Zero);
					gear.rotationAxis = gearJson.value("axis", Math::Vector3(0, 1, 0));
					gear.baseRotation = gearJson.value("baseRot", Math::Vector3::Zero);

					gear.startAngle = gearJson.value("startAngle", 0.0f);
					gear.reverse = gearJson.value("reverse", false);
					gear.scale = gearJson.value("scale", Math::Vector3::One);

					gCompData.gears.push_back(gear);
				}
			}
			p.gearRotate = gCompData;
		}

		if (components.contains("BouncerComponent")) {
			const auto& data = components.at("BouncerComponent");
			BouncerData bData;
			bData.modelPath = data.value("model", "");
			bData.offset = data.value("offset", Math::Vector3::Zero);
			bData.stroke = data.value("stroke", 0.3f);
			p.bouncer = bData;
		}
	}
}