#pragma once
#include"../GameType/GameType.h"
#include"../Src/Framework/Component/ColliderComponent/Shape.h"
#include"../Src/Framework/Component/RigidbodyComponent/RigidbodyComponent.h"
#include"../Src/Framework/Component/GameObject.h"

//各コンポーネント専用のデータ構造体を定義

struct MovingBlockData {
	Math::Vector3 startPos;
	Math::Vector3 endPos;
	float duration = 2.0f;
};

struct TransferBlockData {
	int transferID = 0;
};

struct JumpBlockData {
	Math::Vector3 direction;
	float force = 0.0f;
	float duration = 0.5f;
};

struct SlipperyBlockData
{
	float slipperyDrag;
};

struct RotatingBlockData {
	Math::Vector3 axis = { 0.0f, 1.0f, 0.0f };
	float amount = 90.0f;
	float speed = 5.0f;
};

struct SinkingBlockData {
	Math::Vector3 initialPos;
	float maxSinkDistance = 2.0f;
	float acceleration = 3.0f;
	float riseSpeed = 1.0f;
};

struct ScalingBlockData {
	Math::Vector3 axis = { 0,1,0 };
	float amount = 0.5f;
	float speed = 1.0f;
};

struct MagicCircleData {
	std::string modelPath;
	Math::Vector3 localPos;
	Math::Vector3 localRot;
	Math::Vector3 localScale;
	//軌道上の回転演出パラメータ
	float orbitRadius;
	float orbitSpeed;
	Math::Vector3 orbitAxisOffset;
	//選択状態に応じたアニメーション速度制御
	float normalSpeed = 0.0f;
	float selectedSpeed;
	float selectedScaleMultiplier;
	float scaleLerpSpeed;
};

struct EmitterData {
	std::string systemName;
	int count = 0;
	float frequency = 0.0f;
	Math::Vector3 baseDirection;
	float spread = 0.0f;
	std::vector<Math::Vector3> offsets;
};

struct DissolveData {
	float fadeDuration = 0.3f;
	Math::Vector3 edgeColor = { 1.0f, 1.0f, 1.0f };
	float edgeRange = 0.01f;
	float blockResolution = 4.0f;
};

struct ColliderData {
	Shape::Type shapeType = Shape::Type::Polygon;
	std::string modelPath;
	float radius = 0.5f;
	Math::Vector3 extents = { 0.5f, 0.5f, 0.5f };
	Math::Vector3 offset;
	bool isTrigger = false;
};

struct RigidbodyData {
	RigidbodyType type = RigidbodyType::Static;
};

struct TutorialData {
	std::string blockName;
	std::string text;
	std::string imagePath;
};

struct GlowInstanceData {
	std::string name = "Glow";
	Math::Vector3 offset = Math::Vector3::Zero;
	Math::Vector3 baseRotation = Math::Vector3::Zero;
	Math::Vector3 scale = Math::Vector3(1, 1, 1);
};

struct GlowPartData {
	std::string modelPath;
	Math::Vector3 color = { 0, 50, 0 };

	//発光パーツのアニメーションフラグ管理
	bool enableFloat = false;
	bool enableBlink = false;
	bool enableRotate = false;
	bool isDirectional = false;
	//アニメーション詳細パラメータ
	float floatSpeed = 5.0f;
	Math::Vector3 floatDirection = { 0,1,0 };
	float blinkSpeed = 5.0f;
	float rotateSpeed = 90.0f;
	Math::Vector3 rotateAxis = { 0,1,0 };

	std::vector<GlowInstanceData> instances;
};

struct GearData {
	std::string name = "Gear";
	Math::Vector3 offset = Math::Vector3::Zero;
	Math::Vector3 rotationAxis = { 0, 1, 0 };
	Math::Vector3 baseRotation = Math::Vector3::Zero;
	float startAngle = 0.0f;
	bool reverse = false;
	Math::Vector3 scale = { 1.0,1.0,1.0 };
};

struct GearRotateComponentData {
	std::string modelPath;
	float speed;
	std::vector<GearData> gears;
};

struct BouncerData {
	std::string modelPath;
	Math::Vector3 offset = Math::Vector3::Zero;
	float stroke = 0.3f;
};

//Modelが管理する純粋なブロックの状態データ
struct BlockState
{
	unsigned int entityId = 0;
	BlockType type = BlockType::None;
	GameObject::Tag tag = GameObject::Tag::None;
	Math::Vector3 pos;
	Math::Vector3 rot;
	Math::Vector3 scale;
	std::string renderModelPath;
	std::string archetypeName;
	bool isSwappable;

	//std::optionalを使用することで「その機能を持っているか」のフラグと「データそのもの」を同時に管理する。値が無効(std::nullopt)の場合、メモリ領域は確保されるがコンストラクタコストは発生しない
	std::optional<ColliderData> collider;
	std::optional<RigidbodyData> rigidbody;

	std::optional<MovingBlockData> moving;
	std::optional<TransferBlockData> transfer;
	std::optional<JumpBlockData> jump;
	std::optional<SlipperyBlockData> slippery;
	std::optional<RotatingBlockData> rotating;
	std::optional<SinkingBlockData> sinking;
	std::optional<ScalingBlockData> scaling;
	std::optional<TutorialData> tutorial;
	std::optional<MagicCircleData> magicCircle;
	std::optional<EmitterData> emitter;
	std::optional<DissolveData> dissolve;
	std::optional<GlowPartData> glow;
	std::optional<GearRotateComponentData> gearRotate;
	std::optional<BouncerData> bouncer;
};