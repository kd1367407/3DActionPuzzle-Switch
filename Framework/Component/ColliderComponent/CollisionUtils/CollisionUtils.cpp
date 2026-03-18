#include "CollisionUtils.h"
using namespace DirectX;

// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// /////
// スフィアの情報を逆行列化する
// ===== ===== ===== ===== ===== ===== ===== ===== ===== ===== ===== =====
// 当たり判定を行う際、メッシュ側をワールド変換するより、球側を逆変換してローカルで判定する方が高速
// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// /////
void InvertSphereInfo(DirectX::XMVECTOR& spherePosInv, DirectX::XMVECTOR& sphereScale, float& radiusSqr,
	const DirectX::XMMATRIX& matrix, const DirectX::BoundingSphere& sphere)
{
	// メッシュの逆行列で、球の中心座標を変換(メッシュのローカル座標系へ)
	DirectX::XMMATRIX invMat = XMMatrixInverse(0, matrix);
	spherePosInv = XMVector3TransformCoord(XMLoadFloat3(&sphere.Center), invMat);

	// 半径の二乗(距離比較の高速化用)
	radiusSqr = sphere.Radius * sphere.Radius;

	// 行列の各軸の拡大率(スケール)を取得しておく。楕円体として判定するため
	sphereScale.m128_f32[0] = DirectX::XMVector3Length(matrix.r[0]).m128_f32[0];
	sphereScale.m128_f32[1] = DirectX::XMVector3Length(matrix.r[1]).m128_f32[0];
	sphereScale.m128_f32[2] = DirectX::XMVector3Length(matrix.r[2]).m128_f32[0];
	sphereScale.m128_f32[3] = 0;
}

// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// /////
// スフィアとポリゴンの最近接点を元に接触しているかどうかを判定
// 接触していた場合、球を押し出し位置へ更新する
// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// /////
bool HitCheckAndPosUpdate(DirectX::XMVECTOR& finalPos, DirectX::XMVECTOR& finalHitPos, std::vector<Math::Vector3>& finalFace,
	const std::vector<Math::Vector3>& nearFace, const DirectX::XMVECTOR& nearPoint, const DirectX::XMVECTOR& objScale, float radiusSqr, float sphereRadius)
{
	// 最近接点→球の中心　ベクトルを求める
	DirectX::XMVECTOR vToCenter = finalPos - nearPoint;

	// オブジェクトのスケールを考慮して、球が正球として扱える座標系へ変換
	vToCenter *= objScale;

	// 最近接点が半径の2乗より遠い場合は、衝突していない
	if (DirectX::XMVector3LengthSq(vToCenter).m128_f32[0] > radiusSqr)
	{
		return false;
	}

	// 押し戻し計算
	// めり込んでいるぶんのベクトルを計算
	DirectX::XMVECTOR vPush = DirectX::XMVector3Normalize(vToCenter);

	vPush *= sphereRadius - DirectX::XMVector3Length(vToCenter).m128_f32[0];

	// 拡縮を考慮した座標系(ローカル)へ戻す
	vPush /= objScale;

	// 球の中心座標を更新(押し出す)
	finalPos += vPush;

	finalHitPos = nearPoint;

	// 面情報を更新
	finalFace = nearFace;

	return true;
}

// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// /////
// スフィアとの当たり判定結果をリザルト構造体にセットする
// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// /////
void SetSphereResult(CollisionMeshResult& result, bool isHit, const DirectX::XMVECTOR& hitPos,
	const DirectX::XMVECTOR& finalPos, std::vector<Math::Vector3>& finalFace, const DirectX::XMVECTOR& beginPos)
{
	result.m_hit = isHit;

	result.m_hitPos = hitPos;

	result.m_hitDir = DirectX::XMVectorSubtract(finalPos, beginPos);

	result.m_overlapDistance = DirectX::XMVector3Length(result.m_hitDir).m128_f32[0];

	result.m_hitDir = DirectX::XMVector3Normalize(result.m_hitDir);

	// HITした面の法線を計算する
	Math::Vector3 _normalV1 = finalFace[1] - finalFace[0];
	Math::Vector3 _normalV2 = finalFace[2] - finalFace[0];

	result.m_hitNDir = _normalV1.Cross(_normalV2);
	result.m_hitNDir = DirectX::XMVector3Normalize(result.m_hitNDir);
}

// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// /////
// 点 vs 面を形成する三角形との最近接点を求める
// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// /////
void KdPointToTriangle(DirectX::FXMVECTOR p, DirectX::FXMVECTOR a, DirectX::FXMVECTOR b, DirectX::FXMVECTOR c, DirectX::XMVECTOR& outPt)
{
	// ※参考:[書籍]「ゲームプログラミングのためのリアルタイム衝突判定」
	// 点pが三角形abcのどの領域(頂点近傍、辺近傍、面内部)にあるかを判定し、最近接点を算出する

	//--頂点領域調査--
	// pがaの外側の頂点領域の中にあるかどうかチェック
	XMVECTOR ab = b - a;
	XMVECTOR ac = c - a;
	XMVECTOR ap = p - a;

	float d1 = XMVector3Dot(ab, ap).m128_f32[0];//ab.Dot(ap);
	float d2 = XMVector3Dot(ac, ap).m128_f32[0];//ac.Dot(ap);

	if (d1 <= 0.0f && d2 <= 0.0f)
	{
		outPt = a;	// 重心座標(1,0,0)
		return;
	}

	// pがbの外側の頂点領域の中にあるかどうかチェック
	XMVECTOR bp = p - b;
	float d3 = XMVector3Dot(ab, bp).m128_f32[0];//ab.Dot(bp);
	float d4 = XMVector3Dot(ac, bp).m128_f32[0];//ac.Dot(bp);

	if (d3 >= 0.0f && d4 <= d3)
	{
		outPt = b;	// 重心座標(0,1,0)
		return;
	}

	// pがabの辺領域の中にあるかどうかチェックし、あればpのab上に対する射影を返す
	float vc = d1 * d4 - d3 * d2;

	if (vc <= 0.0f && d1 >= 0.0f && d3 <= 0.0f)
	{
		float v = d1 / (d1 - d3);
		outPt = a + ab * v;	// 重心座標(1-v,v,0)
		return;
	}

	// pがcの外側の頂点領域の中にあるかどうかチェック
	XMVECTOR cp = p - c;
	float d5 = XMVector3Dot(ab, cp).m128_f32[0];//ab.Dot(cp);
	float d6 = XMVector3Dot(ac, cp).m128_f32[0];;//ac.Dot(cp);

	if (d6 >= 0.0f && d5 <= d6)
	{
		outPt = c;	// 重心座標(0,0,1)
		return;
	}

	//--辺領域調査--
	// pがacの辺領域の中にあるかどうかチェックし、あればpのac上に対する射影を返す
	float vb = d5 * d2 - d1 * d6;

	if (vb <= 0.0f && d2 >= 0.0f && d6 <= 0.0f)
	{
		float w = d2 / (d2 - d6);
		outPt = a + ac * w;	// 重心座標(1-w,0,w)
		return;
	}

	// pがbcの辺領域の中にあるかどうかチェックし、あればpのbc上に対する射影を返す
	float va = d3 * d6 - d5 * d4;

	if (va <= 0.0f && (d4 - d3) >= 0.0f && (d5 - d6) >= 0.0f)
	{
		float w = (d4 - d3) / ((d4 - d3) + (d5 - d6));
		outPt = b + (c - b) * w;	// 重心座標(0,1-w,w)
		return;
	}

	//--面領域調査--
	// pは面領域の中にある。Qをその重心座標(u,v,w)を用いて計算
	float denom = 1.0f / (va + vb + vc);
	float v = vb * denom;
	float w = vc * denom;
	outPt = a + ab * v + ac * w;	// = u*a + v*b + w*c, u = va*demon = 1.0f - v - w
}

void SetIterativePushoutResult(CollisionMeshResult& result, bool isHit, const DirectX::XMVECTOR& initialSpherePos_local, const DirectX::XMVECTOR& finalSpherePos_local, const DirectX::XMMATRIX& matrix)
{
	result.m_hit = isHit;

	//ローカル座標系での押し出しベクトル計算
	DirectX::XMVECTOR pushVector_local = DirectX::XMVectorSubtract(finalSpherePos_local, initialSpherePos_local);

	//それをワールド座標系に変換(方向のみ)
	DirectX::XMVECTOR pushVector_world = DirectX::XMVector3TransformNormal(pushVector_local, matrix);

	//ワールド座標系での押し出し距離と方向を格納
	result.m_overlapDistance = DirectX::XMVector3Length(pushVector_world).m128_f32[0];
	result.m_hitDir = DirectX::XMVector3Normalize(pushVector_world);

	//面の法線=最終的な押し出し方向(このアルゴリズムにおいて)
	result.m_hitNDir = result.m_hitDir;

	//最終的な接触点は押し出された後の球の座標をワールド変換したもの
	result.m_hitPos = DirectX::XMVector3TransformCoord(finalSpherePos_local, matrix);
}