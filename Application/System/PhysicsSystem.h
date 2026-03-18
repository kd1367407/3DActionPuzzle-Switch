#pragma once
#include"../Src/Framework/Component/ColliderComponent/ColliderComponent.h"//Layerのため

class GameObject;
class RigidbodyComponent;
class ColliderComponent;
class TransformComponent;

//レイキャストの結果を格納する構造体
struct RayResult
{
	std::weak_ptr<GameObject> m_hitObject;//ヒットしたGameObject
	UINT m_hitObjectId = 0;//ヒットしたGameObjectのID
	Math::Vector3 m_hitPos;//衝突点
	Math::Vector3 m_hitNormal;//衝突点の法線
	float m_distance = 0.0f;//レイの始点からの距離
};

//レイキャストの入力情報
struct RayInfo
{
	Math::Vector3 m_start;//始点
	Math::Vector3 m_dir;//方向(正規化前提)
	float m_maxDistance = 1000.0f;//最大距離
};

//グリッド座標を表すキー
struct GridKey
{
	int x, y, z;

	bool operator==(const GridKey& other) const {
		return x == other.x && y == other.y && z == other.z;
	}
};

struct GridKeyHash
{
	std::size_t operator()(const GridKey& k) const {
		//シンプルなハッシュ結合 (XORとビットシフト)
		return std::hash<int>()(k.x) ^ (std::hash<int>()(k.y) << 1) ^ (std::hash<int>()(k.z) << 2);
	}
};

//物理演算システム
class PhysicsSystem
{
public:
	static PhysicsSystem& Instance();
	~PhysicsSystem() { m_colliders.clear(); }

	void RegisterCollider(const std::shared_ptr<ColliderComponent>& collider);
	void RegisterRigidbody(const std::shared_ptr<RigidbodyComponent>& rigid);

	//物理演算と衝突解決
	void Update(float deltatime);

	//レイキャスト実行
	bool Raycast(const RayInfo& rayInfo, RayResult& outResult, uint32_t layerMask = LayerAll, const GameObject* ignoreObject = nullptr);
	bool RaycastToDebug(const RayInfo& rayInfo, RayResult& outResult, uint32_t layerMask = LayerAll, const GameObject* ignoreObject = nullptr);

	//デバッグ描画
	void DrawDebug(class KdDebugWireFrame& wire);

private:
	PhysicsSystem();

	PhysicsSystem(const PhysicsSystem&) = delete;
	PhysicsSystem& operator=(const PhysicsSystem&) = delete;

	//衝突解決ヘルパー
	void ResolveCollision(const std::shared_ptr<ColliderComponent>& colA, const std::shared_ptr<ColliderComponent>& colB);

	//押し出し処理
	void Pushout(GameObject* dynamicObj, RigidbodyComponent* rigid, Shape* dynamicShape, GameObject* staticObj, Shape* staticShape);

	static bool CheckSpherePolygonIntersection(const DirectX::BoundingSphere& spahe, PolygonShape* polygon, const Math::Matrix& mat);

	//登録された全コライダーのリスト
	std::vector<std::weak_ptr<ColliderComponent>> m_colliders;

	std::vector<std::weak_ptr<RigidbodyComponent>> m_rigidbodies;

	//レイデバッグ
	RayInfo m_debugRay;
	bool m_shouldDrawDebugRay = false;

	//衝突判定関数のシグネチャ
	using CollisionCheakFunc = bool(*)(const std::shared_ptr<ColliderComponent>&, const std::shared_ptr<ColliderComponent>&);

	//関数テーブル
	static const int ShapeTypeCount = 4;//Sphere, Box, Mesh, Polygon
	CollisionCheakFunc m_collisionMatrix[ShapeTypeCount][ShapeTypeCount];

	//テーブル初期化
	void InitializeCollisionMatrix();

	static bool CheckSphereBox(const std::shared_ptr<ColliderComponent>& colA, const std::shared_ptr<ColliderComponent>& colB);
	static bool CheakBoxSphere(const std::shared_ptr<ColliderComponent>& colA, const std::shared_ptr<ColliderComponent>& colB);
	static bool CheckSphereMesh(const std::shared_ptr<ColliderComponent>& colA, const std::shared_ptr<ColliderComponent>& colB);
	static bool CheckMeshSphere(const std::shared_ptr<ColliderComponent>& colA, const std::shared_ptr<ColliderComponent>& colB);
	static bool CheckSpherePolygon(const std::shared_ptr<ColliderComponent>& colA, const std::shared_ptr<ColliderComponent>& colB);
	static bool CheckPolygonSphere(const std::shared_ptr<ColliderComponent>& colA, const std::shared_ptr<ColliderComponent>& colB);

	const float	m_cellSize = 5.0f;

	//グリッドデータ： キー(座標) -> そのセルにいるコライダーのリスト
	std::unordered_map<GridKey, std::vector<std::shared_ptr<ColliderComponent>>, GridKeyHash> m_grid;

	//重複判定防止用
	std::unordered_set<std::string> m_checkedPairs;

	//グリッドにコライダー登録
	void AssignGrid(const std::shared_ptr<ColliderComponent>& collider);

	//コライダーのAABBを取得するヘルパー
	void GetColliderAABB(const std::shared_ptr<ColliderComponent>& collider, Math::Vector3& outMin, Math::Vector3& outMax);
};