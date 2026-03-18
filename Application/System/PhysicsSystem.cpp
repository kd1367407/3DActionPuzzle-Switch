#include "PhysicsSystem.h"
#include"../Src/Framework/Component/GameObject.h"//Raycastで使用
#include"../Src/Framework/Component/ColliderComponent/ColliderComponent.h"
#include"../Src/Framework/Component/RigidbodyComponent/RigidbodyComponent.h"
#include"../Src/Framework/Component/TransformComponent/TransformComponent.h"
#include"../Src/Framework/Component/ColliderComponent/Shape.h"
#include"../Src/Framework/Component/TimerComponent/TimerComponent.h"
#include"../main.h"
#include"../Src/Framework/Component/ICollisionReceiver.h"
#include"../Src/Framework/Component/GravityComponent/GravityComponent.h"
#include"../Src/Framework/Math/KdCollision.h"
#include"../Src/Framework/Component/ColliderComponent/CollisionUtils/CollisionUtils.h"
#include"../Src/Framework/Component/ITriggerReceiver.h"

//ポリゴンと球の押し出し判定を行うヘルパー関数
static bool CalculatePolygonSpherePushout(
	const PolygonShape* polygon,
	const DirectX::BoundingSphere& sphere,
	const DirectX::XMMATRIX& matrix,
	CollisionMeshResult* pResult)
{
	//ブロードフェイズ:AABB(軸平行境界ボックス)による簡易判定で計算負荷を減らす
	DirectX::BoundingBox aabb;
	polygon->GetBoundingBox().Transform(aabb, matrix);
	if (!aabb.Intersects(sphere)) { return false; }

	//ナローフェイズ:詳細なメッシュ判定
	bool isHit = false;
	const auto& vertices = polygon->GetVertices();
	const auto& faces = polygon->GetFaces();

	//判定に必要な変数を準備
	DirectX::XMVECTOR finalHitPos = {};//最終的な接触点(ローカル座標)
	DirectX::XMVECTOR finalPos = {};//押し出された後の球の中心(ローカル座標)
	DirectX::XMVECTOR objScale = {};//オブジェクトのスケール
	std::vector<Math::Vector3> finalFace;//最終的な面
	float radiusSqr = 0.0f;//球の半径の2乗

	//球の情報をローカル座標系に変換
	InvertSphereInfo(finalPos, objScale, radiusSqr, matrix, sphere);

	const DirectX::XMVECTOR initialPos = finalPos;

	//全ての面と反復的に当たり判定
	for (const auto& face : faces)
	{
		DirectX::XMVECTOR nearPoint;
		const UINT* idx = face.Idx;

		//現在の球の中心(finalPos)と現在の三角形との最近接点を求める
		KdPointToTriangle(finalPos, vertices[idx[0]], vertices[idx[1]], vertices[idx[2]], nearPoint);

		std::vector<Math::Vector3> currentFaceVerts = { vertices[idx[0]], vertices[idx[1]], vertices[idx[2]] };

		//接触判定と球の中心位置(finalPos)の更新
		isHit |= HitCheckAndPosUpdate(finalPos, finalHitPos, finalFace, currentFaceVerts, nearPoint, objScale, radiusSqr, sphere.Radius);
	}

	//結果の格納
	if (pResult && isHit)
	{
		SetIterativePushoutResult(*pResult, isHit, initialPos, finalPos, matrix);
	}

	return isHit;
}

PhysicsSystem::PhysicsSystem()
{
	InitializeCollisionMatrix();
}

PhysicsSystem& PhysicsSystem::Instance()
{
	static PhysicsSystem instance;
	return instance;
}

void PhysicsSystem::RegisterCollider(const std::shared_ptr<ColliderComponent>& collider)
{
	m_colliders.push_back(collider);
}

void PhysicsSystem::RegisterRigidbody(const std::shared_ptr<RigidbodyComponent>& rigid)
{
	m_rigidbodies.push_back(rigid);
}

void PhysicsSystem::Update(float deltatime)
{
	//rigidbodyを持つオブジェクトを物理法則に従って動かす
	for (auto& weakRigid : m_rigidbodies)
	{
		if (auto spRigid = weakRigid.lock())
		{
			auto transform = spRigid->GetTransform();
			if (!transform)continue;

			//ジャンプリクエストがあれば速度を強制的に設定
			if (spRigid->m_isJumpRequested)
			{
				spRigid->m_velocity = spRigid->m_jumpVelocity;
				spRigid->m_isJumpRequested = false;
			}

			//動く床の移動量を計算して適用
			Math::Vector3 platformMovement = Math::Vector3::Zero;
			if (auto ground = spRigid->m_groundTransform.lock())
			{
				platformMovement = ground->GetPos() - spRigid->m_groundLastPos;
				spRigid->m_groundLastPos = ground->GetPos();
			}

			//自身の物理演算(Dynamicのみ)
			if (spRigid->m_type == RigidbodyType::Dynamic)
			{
				//m_force(重力や入力など、外から加えられた力)をベースにする
				Math::Vector3 currentStepForce = spRigid->m_force;

				//空気抵抗を加える
				const float defaultDragCoefficient = 1.5f;
				float dragCoefficient = defaultDragCoefficient;

				if (spRigid->m_isOnSlipperySurface)
				{
					dragCoefficient = spRigid->m_surfaceDragValue;
				}

				Math::Vector3 horizontalVelocity = spRigid->m_velocity;
				horizontalVelocity.y = 0; //Y軸(垂直方向)の速度を無視して水平抵抗のみ計算

				//今回のフレームの力に空気抵抗を加算(速度と逆向きの力を加える)
				currentStepForce += (-horizontalVelocity * dragCoefficient);

				//物理法則に従って速度と位置を更新(F=ma -> a=F/m)
				//Math::Vector3 acceleration = spRigid->m_force / spRigid->m_mass;
				Math::Vector3 acceleration = currentStepForce / spRigid->m_mass;
				spRigid->m_velocity += acceleration * deltatime;
			}

			//最終的な移動を実行(速度+床の移動+補正移動)
			transform->Move(spRigid->m_velocity * deltatime + platformMovement + spRigid->m_additionalMovement);

			//次のフレームのためにリセット
			spRigid->m_additionalMovement = Math::Vector3::Zero;
		}
	}

	//グリッド分割の衝突判定
	m_grid.clear();
	m_checkedPairs.clear();

	//1:生存しているコライダーをグリッドに登録
	auto it = m_colliders.begin();
	while (it!=m_colliders.end())
	{
		auto spCol = it->lock();

		if (spCol)
		{
			//グリッド登録(どのセルにいるか計算)
			AssignGrid(spCol);
			++it;
		}
		else
		{
			it = m_colliders.erase(it);
		}
	}

	//2:グリッドごとの衝突判定
	for (auto& pair : m_grid)
	{
		//そのセルにいるコライダーのリスト
		auto& cellColliders = pair.second;

		//オブジェクトが１つ以下なら衝突しないのでスキップ
		if (cellColliders.size() < 2)continue;

		//このセル内のオブジェクト同士で総当たり
		for (size_t i = 0; i < cellColliders.size(); ++i)
		{
			for (size_t j = i + 1; j < cellColliders.size(); ++j)
			{
				auto colA = cellColliders[i];
				auto colB = cellColliders[j];

				auto rbA = colA->GetOwner()->GetComponent<RigidbodyComponent>();
				auto rbB = colB->GetOwner()->GetComponent<RigidbodyComponent>();

				bool isStaticA = (!rbA || rbA->m_type == RigidbodyType::Static);
				bool isStaticB = (!rbB || rbB->m_type == RigidbodyType::Static);

				// 両方とも動かないオブジェクトなら衝突判定しない
				if (isStaticA && isStaticB) continue;

				//重複(オブジェクトA、Bがセル１、２の両方にまたがっている場合)判定
				//IDコンポーネントがあればよかったけど、今回はポインタのアドレスを利用
				uintptr_t ptrA = reinterpret_cast<uintptr_t>(colA.get());
				uintptr_t ptrB = reinterpret_cast<uintptr_t>(colB.get());

				//IDの大小を揃えてキーを作成
				if (ptrA > ptrB)std::swap(ptrA, ptrB);
				std::string pairKey = std::to_string(ptrA) + "-" + std::to_string(ptrB);

				//既にこのフレームで判定済みならスキップ
				if (m_checkedPairs.count(pairKey))continue;

				//判定済みリストに登録
				m_checkedPairs.insert(pairKey);

				//実際の衝突解決を実行
				ResolveCollision(colA, colB);
			}
		}
	}
}

bool PhysicsSystem::Raycast(const RayInfo& rayInfo, RayResult& outResult, uint32_t layerMask, const GameObject* ignoreObject)
{
	bool hit = false;
	float closestDist = FLT_MAX;
	RayResult finalResult;

	//登録されている全てのコライダーと判定
	for (auto& weakCollider : m_colliders)
	{
		if (auto spCollider = weakCollider.lock())
		{
			//レイヤーマスク判定
			if (!(spCollider->GetLayer() & layerMask))
			{
				continue;
			}

			auto owner = spCollider->GetOwner();

			//無視するオブジェクトと一致したらスキップ
			if (owner == ignoreObject)
			{
				continue;
			}

			auto transform = spCollider->GetTransform();
			Shape* shape = spCollider->GetShape();
			if (!transform || !shape)continue;

			float hitDist = -1.0f;
			Math::Vector3 hitPos{}, hitNolmal{};
			bool bThisShapeHit = false;//この形状でヒットしたかどうか

			//形状ごとの当たり判定
			switch (shape->GetType())
			{
			case Shape::Type::Sphere:
			{
				auto* sphereShape = static_cast<SphereShape*>(shape);
				DirectX::BoundingSphere sphere(transform->GetPos() + sphereShape->m_offset, sphereShape->m_radius);
				if (sphere.Intersects(rayInfo.m_start, rayInfo.m_dir, hitDist))
				{
					bThisShapeHit = true;
					hitPos = rayInfo.m_start + rayInfo.m_dir * hitDist;
					hitNolmal = hitPos - sphere.Center;
					hitNolmal.Normalize();
				}
				break;
			}
			case Shape::Type::Box:
			{
				auto* boxShape = static_cast<BoxShape*>(shape);
				DirectX::BoundingOrientedBox obb;
				obb.Center = transform->GetPos() + boxShape->m_offset;
				obb.Extents = boxShape->m_extents;
				Math::Quaternion q;
				q.CreateFromYawPitchRoll(
					DirectX::XMConvertToRadians(transform->GetRot().y),
					DirectX::XMConvertToRadians(transform->GetRot().x),
					DirectX::XMConvertToRadians(transform->GetRot().z)
				);
				obb.Orientation = q;
				if (obb.Intersects(rayInfo.m_start, rayInfo.m_dir, hitDist))
				{
					bThisShapeHit = true;
					hitPos = rayInfo.m_start + rayInfo.m_dir * hitDist;
					//簡易的法線
					hitNolmal = -rayInfo.m_dir;
				}
				break;
			}
			case Shape::Type::Mesh:
			{
				auto* meshShape = static_cast<MeshShape*>(shape);
				auto model = meshShape->m_spModel;

				if (model && !model->GetCollisionMeshNodeIndices().empty())
				{
					//当たり判定ように指定された全てのメッシュに対してチェック
					for (int nodeIndex : model->GetCollisionMeshNodeIndices())
					{
						//メッシュのポインタが有効かチェック
						if (const auto& mesh = model->GetMesh(nodeIndex))
						{
							CollisionMeshResult result;
							//ループで取得した正しいメッシュ(*mesh)を使って判定
							if (MeshIntersect(*mesh, rayInfo.m_start, rayInfo.m_dir, rayInfo.m_maxDistance, transform->GetMatrix(), &result))
							{
								//複数のメッシュにヒットした場合、最も近いものを採用(レイの最大距離 - 交点までの距離 で計算されるため、交点が近いほど、m_overlapDistanceは大きくなる)
								if (result.m_overlapDistance > hitDist || hitDist < 0)
								{
									bThisShapeHit = true;
									hitDist = rayInfo.m_maxDistance - result.m_overlapDistance;
									hitPos = result.m_hitPos;
									hitNolmal = result.m_hitNDir;
								}
							}
						}
					}
				}
				break;
			}
			case Shape::Type::Polygon:
			{
				//polygonShapeとして形状を取得
				auto* polygonShape = static_cast<PolygonShape*>(shape);
				if (!polygonShape)break;

				//レイをローカル座標系に変換(ポリゴンデータがローカル座標系のため)
				DirectX::XMVECTOR rayPosInv, rayDirInv;
				float rayRangeInv = 0.0f, scaleInv = 0.0f;
				InvertRayInfo(rayPosInv, rayDirInv, rayRangeInv, scaleInv, transform->GetMatrix(), rayInfo.m_start, rayInfo.m_dir, rayInfo.m_maxDistance);

				//頂点リストを取得
				const auto& positions = polygonShape->GetVertices();
				const auto& faces = polygonShape->GetFaces();
				float localClosestDist = FLT_MAX;
				size_t hitFaceIndex = -1;
				bool bHitInLoop = false;

				//全ての三角形とループで判定
				for (size_t faceid = 0; faceid < faces.size(); ++faceid)
				{
					const KdMeshFace& face = faces[faceid];
					const UINT* idx = face.Idx;

					float dist = FLT_MAX;
					if (DirectX::TriangleTests::Intersects(rayPosInv, rayDirInv, positions[idx[0]], positions[idx[1]], positions[idx[2]], dist))
					{
						if (dist > rayRangeInv)continue;//レイの範囲外なら無視

						if (dist < localClosestDist)
						{
							bHitInLoop = true;
							localClosestDist = dist;
							hitFaceIndex = faceid;
						}
					}
				}

				//ループ内で一度でも当たっていれば最終的な結果を計算(SetRayResultを参考)
				if (bHitInLoop)
				{
					CollisionMeshResult tempResult;
					tempResult.m_hit = true;

					//距離をワールド座標系に戻して計算(localClosestDistはスケールを考慮)
					tempResult.m_hitPos = rayInfo.m_start + rayInfo.m_dir * (localClosestDist / scaleInv);
					tempResult.m_overlapDistance = rayInfo.m_maxDistance - (localClosestDist / scaleInv);
					if (hitFaceIndex != -1)
					{
						const KdMeshFace& hitFace = faces[hitFaceIndex];
						const UINT* idx = hitFace.Idx;

						const Math::Vector3& v0 = positions[idx[0]];
						const Math::Vector3& v1 = positions[idx[1]];
						const Math::Vector3& v2 = positions[idx[2]];

						//外積を求めて法線の方向ベクトルを決定
						Math::Vector3 n = (v1 - v0).Cross(v2 - v0);
						//法線(ローカル座標系)にする
						n.Normalize();

						//ワールド座標系に戻す
						tempResult.m_hitNDir = DirectX::XMVector3TransformNormal(n, transform->GetMatrix());
					}

					if (tempResult.m_overlapDistance > hitDist || hitDist < 0)
					{
						bThisShapeHit = true;
						hitDist = rayInfo.m_maxDistance - tempResult.m_overlapDistance;
						hitPos = tempResult.m_hitPos;
						hitNolmal = tempResult.m_hitNDir;
					}
				}
				break;
			}
			}
			//より近いオブジェクトが見つかったかチェック
			if (bThisShapeHit && hitDist < closestDist)
			{
				hit = true;
				closestDist = hitDist;
				finalResult.m_hitObject = owner->shared_from_this();
				finalResult.m_distance = hitDist;
				finalResult.m_hitPos = hitPos;
				finalResult.m_hitNormal = hitNolmal;
			}
		}
	}
	if (hit)
	{
		outResult = finalResult;
	}
	return hit;
}

bool PhysicsSystem::RaycastToDebug(const RayInfo& rayInfo, RayResult& outResult, uint32_t layerMask, const GameObject* ignoreObject)
{
	m_debugRay = rayInfo;
	m_shouldDrawDebugRay = true;

	return Raycast(rayInfo, outResult, layerMask, ignoreObject);
}

void PhysicsSystem::DrawDebug(KdDebugWireFrame& wire)
{
	//登録されている全てのコライダーの形状を描画
	for (const auto& weakCol : m_colliders)
	{
		if (auto spCol = weakCol.lock())
		{
			auto transform = spCol->GetTransform();
			Shape* shape = spCol->GetShape();
			if (!transform || !shape)continue;

			//形状のオフセットを考慮した中心座標
			Math::Vector3 centerPos = transform->GetPos() + shape->m_offset;

			switch (shape->GetType())
			{
			case Shape::Type::Sphere:
			{
				auto* sphere = static_cast<SphereShape*>(shape);
				wire.AddDebugSphere(centerPos, sphere->m_radius, kGreenColor);
				break;
			}
			case Shape::Type::Box:
			{
				auto* box = static_cast<BoxShape*>(shape);
				wire.AddDebugBox(transform->GetMatrix(), box->m_extents, box->m_offset, true, kRedColor);
				break;
			}
			}
		}
	}

	if (m_shouldDrawDebugRay)
	{
		wire.AddDebugLine(m_debugRay.m_start, m_debugRay.m_dir, m_debugRay.m_maxDistance, kRedColor);
	}
}

void PhysicsSystem::ResolveCollision(const std::shared_ptr<ColliderComponent>& colA, const std::shared_ptr<ColliderComponent>& colB)
{
	auto ownerA = colA->GetOwner();
	auto ownerB = colB->GetOwner();

	Shape* shapeA = colA->GetShape();
	Shape* shapeB = colB->GetShape();
	if (!shapeA || !shapeB)return;

	//実際に衝突しているかどうかを判定
	bool isColliding = false;
	auto transformA = ownerA->GetComponent<TransformComponent>();
	auto transformB = ownerB->GetComponent<TransformComponent>();
	if (!transformA || !transformB)return;

	//テーブルから関数取得
	auto func = m_collisionMatrix[(int)shapeA->GetType()][(int)shapeB->GetType()];

	if (!func || !func(colA, colB))return;

	//どちらかがトリガーだった場合
	if (colA->isTrigger() || colB->isTrigger())
	{
		CollisionInfo infoA, infoB;
		infoA.otherObject = ownerB->shared_from_this();
		infoB.otherObject = ownerA->shared_from_this();

		//簡易的法線
		infoA.contactNormal = transformB->GetPos() - transformA->GetPos();
		infoB.contactNormal = -infoA.contactNormal;

		//通知
		for (const auto& comp : ownerA->GetComponents())
		{
			if (auto receiver = std::dynamic_pointer_cast<ITriggerReceiver>(comp))
			{
				receiver->OnTriggerEnter(infoA);
			}
		}
		for (const auto& comp : ownerB->GetComponents())
		{
			if (auto receiver = std::dynamic_pointer_cast<ITriggerReceiver>(comp))
			{
				receiver->OnTriggerEnter(infoB);
			}
		}

		//トリガーなので押し出しはしない
		return;
	}

	//衝突が確認出来たら各処理をする
	auto rigidA = ownerA->GetComponent<RigidbodyComponent>();
	auto rigidB = ownerB->GetComponent<RigidbodyComponent>();

	//どっちも動かないなら衝突解決は不要
	if (!rigidA && !rigidB)return;

	//ボディタイプで役割を判別
	bool isADynamic = rigidA && rigidA->m_type == RigidbodyType::Dynamic;
	bool isBDynamic = rigidB && rigidB->m_type == RigidbodyType::Dynamic;
	//bool isAStaticOrKinematic = !rigidA || rigidA->m_type == RigidbodyType::Kinematic;
	//bool isBStaticOrKinematic = !rigidB || rigidB->m_type == RigidbodyType::Kinematic;

	//Aが動的、Bが静的,または運動学的な場合
	if (isADynamic && !isBDynamic)
	{
		Pushout(ownerA, rigidA.get(), shapeA, ownerB, shapeB);
	}
	//Bが動的、Aが静的、静的,または運動学的な場合
	else if (!isADynamic && isBDynamic)
	{
		Pushout(ownerB, rigidB.get(), shapeB, ownerA, shapeA);
	}
	//両方動的
	else if (isADynamic && isBDynamic)
	{
		return;
	}
}

void PhysicsSystem::Pushout(GameObject* dynamicObj, RigidbodyComponent* rigid, Shape* dynamicShape, GameObject* staticObj, Shape* staticShape)
{
	auto dynamicTransform = rigid->GetTransform();
	auto staticTransform = staticObj->GetComponent<TransformComponent>();
	if (!dynamicTransform || !staticTransform)return;

	Math::Vector3 pushDir;//衝突した面の法線
	bool bPushOut = false;

	//球 vs メッシュ
	if (dynamicShape->GetType() == Shape::Type::Sphere && staticShape->GetType() == Shape::Type::Mesh)
	{
		auto* sphereShape = static_cast<SphereShape*>(dynamicShape);
		auto* meshShape = static_cast<MeshShape*>(staticShape);

		DirectX::BoundingSphere sphere;
		sphere.Center = dynamicTransform->GetPos() + sphereShape->m_offset;
		sphere.Radius = sphereShape->m_radius;

		CollisionMeshResult result;
		if (MeshIntersect(*meshShape->m_spModel->GetMesh(0), sphere, staticTransform->GetMatrix(), &result))
		{
			bPushOut = true;
			pushDir = result.m_hitDir;
			float pushDist = result.m_overlapDistance;
			Math::Vector3 push = pushDir * pushDist;
			dynamicTransform->Move(push);

			auto& vel = rigid->m_velocity;

			if (DirectX::XMVectorGetY(result.m_hitNDir) > 0.7f)//法線が真上に近い(地面)
			{
				if (vel.y < 0)
				{
					vel.y = 0;//バウンドさせない
				}
			}
			else//法線が横や下を向いている(壁や天井)
			{
				vel = DirectX::XMVector3Reflect(vel, result.m_hitNDir);
				vel *= 0.8f;
			}
		}
	}
	//球 vs 箱
	else if (dynamicShape->GetType() == Shape::Type::Sphere && staticShape->GetType() == Shape::Type::Box)
	{
		auto* sphereShape = static_cast<SphereShape*>(dynamicShape);
		auto* boxShape = static_cast<BoxShape*>(staticShape);

		DirectX::BoundingSphere sphere;
		sphere.Center = dynamicTransform->GetPos() + sphereShape->m_offset;
		sphere.Radius = sphereShape->m_radius;

		DirectX::BoundingOrientedBox obb;
		obb.Center = staticTransform->GetPos() + boxShape->m_offset;
		obb.Extents = boxShape->m_extents;

		Math::Quaternion q;
		q.CreateFromYawPitchRoll(
			DirectX::XMConvertToRadians(staticTransform->GetRot().y),
			DirectX::XMConvertToRadians(staticTransform->GetRot().x),
			DirectX::XMConvertToRadians(staticTransform->GetRot().z)
		);
		obb.Orientation = q;

		if (sphere.Intersects(obb))
		{
			bPushOut = true;
			//1:球の中心をローカル座標系に変換
			Math::Vector3 sphereCenter(sphere.Center);
			Math::Vector3 obbCenter(obb.Center);
			Math::Vector3 sphereCenterInBoxSpace = sphereCenter - obbCenter;
			sphereCenterInBoxSpace = XMVector3InverseRotate(sphereCenterInBoxSpace, q);

			//2:箱の表面上の最近接点をローカル座標系で求める
			Math::Vector3 closestPointInBoxSpace;
			closestPointInBoxSpace.x = std::clamp(sphereCenterInBoxSpace.x, -obb.Extents.x, obb.Extents.x);
			closestPointInBoxSpace.y = std::clamp(sphereCenterInBoxSpace.y, -obb.Extents.y, obb.Extents.y);
			closestPointInBoxSpace.z = std::clamp(sphereCenterInBoxSpace.z, -obb.Extents.z, obb.Extents.z);

			//3:最近接点をワールド座標系に戻す
			Math::Vector3 closestPointWorld = XMVector3Rotate(closestPointInBoxSpace, q);
			closestPointWorld += obb.Center;

			//4:押し出しベクトルと量を計算
			pushDir = Math::Vector3(sphere.Center) - closestPointWorld;
			float dist = pushDir.Length();
			float overlap = sphere.Radius - dist;

			if (overlap > 0)
			{
				pushDir.Normalize();
				Math::Vector3 pushVector = pushDir * overlap;

				//5:動的オブジェクトのみ押し出す
				dynamicTransform->Move(pushVector);
			}
		}
	}
	//球vsポリゴン
	else if (dynamicShape->GetType() == Shape::Type::Sphere && staticShape->GetType() == Shape::Type::Polygon)
	{
		auto* sphereShape = static_cast<SphereShape*>(dynamicShape);
		auto* polygonShape = static_cast<PolygonShape*>(staticShape);

		DirectX::BoundingSphere sphere;
		sphere.Center = dynamicTransform->GetPos() + sphereShape->m_offset;
		sphere.Radius = sphereShape->m_radius;

		CollisionMeshResult result;
		if (CalculatePolygonSpherePushout(polygonShape, sphere, staticTransform->GetMatrix(), &result))
		{
			bPushOut = true;
			pushDir = result.m_hitDir;
			float pushDist = result.m_overlapDistance;

			dynamicTransform->Move(pushDir * pushDist);

			auto& vel = rigid->m_velocity;
			if (rigid->m_isOnSlipperySurface)
			{
				//滑っているときは速度を一切いじらない(慣性で滑らせるため)
			}
			else if (DirectX::XMVectorGetY(result.m_hitNDir) > 0.7f)
			{
				if (vel.y < 0) { vel.y = 0; }
				/*vel = DirectX::XMVector3Reflect(vel, result.m_hitNDir);
				vel *= 0.8f;*/
			}
			else
			{
				vel = DirectX::XMVector3Reflect(vel, result.m_hitNDir);
				vel *= 0.8f;
			}
		}
	}

	//--通知処理--
	if (bPushOut)
	{
		CollisionInfo dynamicInfo, staticInfo;
		dynamicInfo.otherObject = staticObj->shared_from_this();
		dynamicInfo.contactNormal = pushDir;

		staticInfo.otherObject = dynamicObj->shared_from_this();
		staticInfo.contactNormal = pushDir;

		//動的オブジェクトに通知
		for (const auto& comp : dynamicObj->GetComponents()) {
			if (auto receiver = std::dynamic_pointer_cast<ICollisionReceiver>(comp)) {
				receiver->OnCollision(dynamicInfo);
			}
		}

		//静的オブジェクトに通知
		for (const auto& comp : staticObj->GetComponents()) {
			if (auto receiver = std::dynamic_pointer_cast<ICollisionReceiver>(comp)) {
				receiver->OnCollision(staticInfo);
			}
		}
	}
}

bool PhysicsSystem::CheckSpherePolygonIntersection(const DirectX::BoundingSphere& spahe, PolygonShape* polygon, const Math::Matrix& mat)
{
	//頂点リスト取得
	const auto& positions = polygon->GetVertices();
	const auto& faces = polygon->GetFaces();

	//球情報をローカル座標系に変換
	DirectX::XMVECTOR spherePosInv = {};
	DirectX::XMVECTOR objScale = {};
	float radiusSqr = 0.0f;
	InvertSphereInfo(spherePosInv, objScale, radiusSqr, mat, spahe);

	//全ての面と判定
	for (const KdMeshFace& face : faces)
	{
		DirectX::XMVECTOR nearPos;

		const Math::Vector3& v0 = positions[face.Idx[0]];
		const Math::Vector3& v1 = positions[face.Idx[1]];
		const Math::Vector3& v2 = positions[face.Idx[2]];

		//点と三角形の最近接点を計算
		KdPointToTriangle(spherePosInv, v0, v1, v2, nearPos);

		DirectX::XMVECTOR vecToCenter = DirectX::XMVectorSubtract(spherePosInv, nearPos);

		//拡縮を考慮した距離の2乗で比較
		vecToCenter = DirectX::XMVectorMultiply(vecToCenter, objScale);

		//.m128_f32[0]はXMVECTOR変数を4つのfloat値が入った配列として扱い、[0]番目の値を取り出している
		if (DirectX::XMVector3LengthSq(vecToCenter).m128_f32[0] <= radiusSqr)
		{
			return true;
		}
	}
	return false;
}

void PhysicsSystem::InitializeCollisionMatrix()
{
	//テーブル初期化
	for (int i = 0; i < ShapeTypeCount; ++i)
	{
		for (int j = 0; j < ShapeTypeCount; j++)
		{
			m_collisionMatrix[i][j] = nullptr;
		}
	}

	//組み合わせ登録
	m_collisionMatrix[(int)Shape::Type::Sphere][(int)Shape::Type::Box] = &PhysicsSystem::CheckSphereBox;
	m_collisionMatrix[(int)Shape::Type::Box][(int)Shape::Type::Sphere] = &PhysicsSystem::CheakBoxSphere;
	m_collisionMatrix[(int)Shape::Type::Sphere][(int)Shape::Type::Mesh] = &PhysicsSystem::CheckSphereMesh;
	m_collisionMatrix[(int)Shape::Type::Mesh][(int)Shape::Type::Sphere] = &PhysicsSystem::CheckMeshSphere;
	m_collisionMatrix[(int)Shape::Type::Sphere][(int)Shape::Type::Polygon] = &PhysicsSystem::CheckSpherePolygon;
	m_collisionMatrix[(int)Shape::Type::Polygon][(int)Shape::Type::Sphere] = &PhysicsSystem::CheckPolygonSphere;
}

bool PhysicsSystem::CheckSphereBox(const std::shared_ptr<ColliderComponent>& colA, const std::shared_ptr<ColliderComponent>& colB)
{
	auto sphereShape = static_cast<SphereShape*>(colA->GetShape());
	auto boxShape = static_cast<BoxShape*>(colB->GetShape());
	auto transA = colA->GetTransform();
	auto transB = colB->GetTransform();
	
	DirectX::BoundingSphere sphere(transA->GetPos() + sphereShape->m_offset, sphereShape->m_radius);
	DirectX::BoundingOrientedBox obb;
	obb.Center = transB->GetPos() + boxShape->m_offset;
	obb.Extents = boxShape->m_extents;
	Math::Quaternion q;
	q.CreateFromYawPitchRoll(
		DirectX::XMConvertToRadians(transB->GetRot().y),
		DirectX::XMConvertToRadians(transB->GetRot().x),
		DirectX::XMConvertToRadians(transB->GetRot().z)
	);
	obb.Orientation = q;

	return sphere.Intersects(obb);
}

bool PhysicsSystem::CheakBoxSphere(const std::shared_ptr<ColliderComponent>& colA, const std::shared_ptr<ColliderComponent>& colB)
{
	auto sphereShape = static_cast<SphereShape*>(colB->GetShape());
	auto boxShape = static_cast<BoxShape*>(colA->GetShape());
	auto transA = colA->GetTransform();
	auto transB = colB->GetTransform();

	DirectX::BoundingSphere sphere(transB->GetPos() + sphereShape->m_offset, sphereShape->m_radius);
	DirectX::BoundingOrientedBox obb;
	obb.Center = transA->GetPos() + boxShape->m_offset;
	obb.Extents = boxShape->m_extents;
	Math::Quaternion q;
	q.CreateFromYawPitchRoll(
		DirectX::XMConvertToRadians(transA->GetRot().y),
		DirectX::XMConvertToRadians(transA->GetRot().x),
		DirectX::XMConvertToRadians(transA->GetRot().z)
	);
	obb.Orientation = q;

	return sphere.Intersects(obb);
}

bool PhysicsSystem::CheckSphereMesh(const std::shared_ptr<ColliderComponent>& colA, const std::shared_ptr<ColliderComponent>& colB)
{
	auto sphereShape = static_cast<SphereShape*>(colA->GetShape());
	auto meshShape = static_cast<MeshShape*>(colB->GetShape());
	auto transA = colA->GetTransform();
	auto transB = colB->GetTransform();

	DirectX::BoundingSphere sphere(transA->GetPos() + sphereShape->m_offset, sphereShape->m_radius);
	CollisionMeshResult result; // Pushoutに渡さないので結果は捨てる
	
	return MeshIntersect(*meshShape->m_spModel->GetMesh(0), sphere, transB->GetMatrix(), &result);
}

bool PhysicsSystem::CheckMeshSphere(const std::shared_ptr<ColliderComponent>& colA, const std::shared_ptr<ColliderComponent>& colB)
{
	auto sphereShape = static_cast<SphereShape*>(colB->GetShape());
	auto meshShape = static_cast<MeshShape*>(colA->GetShape());
	auto transA = colA->GetTransform();
	auto transB = colB->GetTransform();

	DirectX::BoundingSphere sphere(transB->GetPos() + sphereShape->m_offset, sphereShape->m_radius);
	CollisionMeshResult result;
	
	return MeshIntersect(*meshShape->m_spModel->GetMesh(0), sphere, transA->GetMatrix(), &result);
}

bool PhysicsSystem::CheckSpherePolygon(const std::shared_ptr<ColliderComponent>& colA, const std::shared_ptr<ColliderComponent>& colB)
{
	auto sphereShape = static_cast<SphereShape*>(colA->GetShape());
	auto polygonShape = static_cast<PolygonShape*>(colB->GetShape());
	auto transA = colA->GetTransform();
	auto transB = colB->GetTransform();

	DirectX::BoundingSphere sphere(transA->GetPos() + sphereShape->m_offset, sphereShape->m_radius);
	
	return PhysicsSystem::CheckSpherePolygonIntersection(sphere, polygonShape, transB->GetMatrix());
}

bool PhysicsSystem::CheckPolygonSphere(const std::shared_ptr<ColliderComponent>& colA, const std::shared_ptr<ColliderComponent>& colB)
{
	auto sphereShape = static_cast<SphereShape*>(colB->GetShape());
	auto polygonShape = static_cast<PolygonShape*>(colA->GetShape());
	auto transA = colA->GetTransform();
	auto transB = colB->GetTransform();

	DirectX::BoundingSphere sphere(transB->GetPos() + sphereShape->m_offset, sphereShape->m_radius);
	
	return PhysicsSystem::CheckSpherePolygonIntersection(sphere, polygonShape, transA->GetMatrix());
}

//1つのオブジェクト（コライダー）を受け取って、そのオブジェクトが重なっている全てのセルに自分（のポインタ）を登録
void PhysicsSystem::AssignGrid(const std::shared_ptr<ColliderComponent>& collider)
{
	Math::Vector3 minAABB, maxAABB;
	GetColliderAABB(collider, minAABB, maxAABB);

	//このAABBが重なっているグリッドのインデックス範囲を計算
	int startX = static_cast<int>(std::floor(minAABB.x / m_cellSize));
	int endX = static_cast<int>(std::floor(maxAABB.x / m_cellSize));
	int startY = static_cast<int>(std::floor(minAABB.y / m_cellSize));
	int endY = static_cast<int>(std::floor(maxAABB.y / m_cellSize));
	int startZ = static_cast<int>(std::floor(minAABB.z / m_cellSize));
	int endZ = static_cast<int>(std::floor(maxAABB.z / m_cellSize));

	//範囲内の全てのセルにこのコライダーを登録
	for (int x = startX; x <= endX; ++x)
	{
		for (int y = startY; y <= endY; ++y)
		{
			for (int z = startZ; z <= endZ; ++z)
			{
				m_grid[{x, y, z}].push_back(collider);
			}
		}
	}
}

void PhysicsSystem::GetColliderAABB(const std::shared_ptr<ColliderComponent>& collider, Math::Vector3& outMin, Math::Vector3& outMax)
{
	auto transform = collider->GetTransform();
	Shape* shape = collider->GetShape();
	if (!transform || !shape) return;

	//Shapeが持っているローカルAABBを取得
	const DirectX::BoundingBox& localAABB = shape->GetBoundingBox();

	//ワールド行列を使って、ワールド空間でのAABBに変換
	DirectX::BoundingBox worldAABB;
	localAABB.Transform(worldAABB, transform->GetMatrix());

	//変換後のAABBから最小・最大を取得
	outMin = Math::Vector3(worldAABB.Center) - Math::Vector3(worldAABB.Extents);
	outMax = Math::Vector3(worldAABB.Center) + Math::Vector3(worldAABB.Extents);
}
