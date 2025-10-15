#include "Enemy.h"
#include "MapChipField/MapChipField.h"
#include <algorithm>
#include <array>
#include <cmath> // イージングで cos を使う
#include <numbers>

using namespace KamataEngine::MathUtility;

static float EaseInOutSine(float x) { return -(std::cos(std::numbers::pi_v<float> * x) - 1.0f) / 2.0f; }

void Enemy::Initialize(Camera* camera, const Vector3& position) {

	camera_ = camera;
	camera_->Initialize();

	worldTransform_.Initialize();
	worldTransform_.translation_ = position; // 初期位置

	// 初期速度（とりあえず左向きに歩き始める）
	velocity_ = {-kWalkSpeed, 0.0f, 0.0f};

	// 初期は地面上想定なら必要に応じて onGround_ = true; にしてOK
	onGround_ = false;

	// 速度に応じて見た目の向きを即時決定（初期のみ即反映）
	FaceByVelocity();

	walkTimer_ = 0.0f;
}

void Enemy::Update() {

	// ==== 空中なら重力を加える ====
	if (!onGround_) {
		velocity_ += Vector3(0.0f, -kGravityAcceleration * deltaTime_, 0.0f);
		velocity_.y = (std::max)(velocity_.y, -kLimitFallSpeed);
	}

	LedgeTurnCheck();

	// ==== 横移動＋落下を含む移動量を作成 ====
	CollisionMapInfo info{};
	info.move = velocity_ * deltaTime_;

	if (mapChipField_) {
		// 上下左右の順で当たり判定
		CollisionMapCheck(info);

		// 判定結果を反映
		MoveByCollisionCheck(info);

		// 付随処理
		OnCellingCollision(info); // 天井に当たったら上昇速度をゼロ
		OnWallCollision(info);    // 壁に当たったら反転（※ここで回転開始要求を出す）
		SwitchOnGround(info);     // 接地/空中の切り替え（落ち始め・着地）
	} else {
		// マップ未設定なら従来どおりそのまま移動
		worldTransform_.translation_ += info.move;
	}

	// 万一、AI等で速度符号が外部から変わった場合にも対応
	RequestTurnByVelocity();

	// ==== 歩行アニメーション（既存） ====
	walkTimer_ += deltaTime_;
	float param = std::sin(2 * std::numbers::pi_v<float> * walkTimer_ / kWalkMotionTime);
	float degree = kWalkMotionAngleStart + kWalkMotionAngleEnd * (param + 1.0f) / 2.0f;
	worldTransform_.rotation_.x = degree * std::numbers::pi_v<float> / 180.0f;

	// ==== イージング回転 ====
	ModelRotate();

	UpdateAffineTransformMatrix();
}

void Enemy::Draw() { model_->Draw(worldTransform_, *camera_); }

// =======================
// アフィン変換行列の更新処理
// =======================
void Enemy::UpdateAffineTransformMatrix() {
	Matrix4x4 scaleMat = MakeScaleMatrix(worldTransform_.scale_);
	Matrix4x4 rotZMat = MakeRotateZMatrix(worldTransform_.rotation_.z);
	Matrix4x4 rotYMat = MakeRotateYMatrix(worldTransform_.rotation_.y);
	Matrix4x4 rotXMat = MakeRotateXMatrix(worldTransform_.rotation_.x);
	Matrix4x4 rotMat = rotZMat * rotYMat * rotXMat;
	Matrix4x4 transMat = MakeTranslateMatrix(worldTransform_.translation_);
	worldTransform_.matWorld_ = scaleMat * rotMat * transMat;
	worldTransform_.TransferMatrix();
}

void Enemy::OnPlayerCollision(Player* player) { player; }

Vector3 Enemy::GetWorldPosition() {
	Vector3 worldPos;
	worldPos.x = worldTransform_.translation_.x;
	worldPos.y = worldTransform_.translation_.y;
	worldPos.z = worldTransform_.translation_.z;
	return worldPos;
}

AABB Enemy::GetAABB() {
	Vector3 worldPos = GetWorldPosition();
	AABB aabb;
	aabb.min = Vector3(worldPos.x - kWidth / 2.0f, worldPos.y - kHeight / 2.0f, worldPos.z - kWidth / 2.0f);
	aabb.max = Vector3(worldPos.x + kWidth / 2.0f, worldPos.y + kHeight / 2.0f, worldPos.z + kWidth / 2.0f);
	return aabb;
}

// ============================
// 角の座標を計算
// ============================
Vector3 Enemy::CornerPosition(const Vector3& center, Corner corner) {
	Vector3 offsetTable[kNumCorner] = {
	    {+kWidth / 2.0f, -kHeight / 2.0f, 0.0f}, //  右下
	    {-kWidth / 2.0f, -kHeight / 2.0f, 0.0f}, //  左下
	    {+kWidth / 2.0f, +kHeight / 2.0f, 0.0f}, //  右上
	    {-kWidth / 2.0f, +kHeight / 2.0f, 0.0f}  //  左上
	};
	return center + offsetTable[static_cast<uint32_t>(corner)];
}

// ============================
// 衝突判定本体（今フレームの移動量を詰める）
// ============================
void Enemy::CollisionMapCheck(CollisionMapInfo& info) {

	// 事前に移動後の四隅を一括算出（Playerと同じ最適化）
	std::array<Vector3, kNumCorner> positionsNew;
	for (uint32_t i = 0; i < positionsNew.size(); ++i) {
		positionsNew[i] = CornerPosition(worldTransform_.translation_ + info.move, static_cast<Corner>(i));
	}

	// 上下左右の順で4回呼ぶ（Player準拠）
	const HitDir order[] = {HitDir::kUp, HitDir::kDown, HitDir::kRight, HitDir::kLeft};
	for (HitDir d : order) {
		CollisionOneSide(info, d, positionsNew);
	}
}

// ============================
// 方向別の衝突判定（上下左右）
// ============================
void Enemy::CollisionOneSide(CollisionMapInfo& info, HitDir dir, const std::array<Vector3, kNumCorner>& positionsNew) {

	// 方向別の早期 return 条件（Player準拠）
	if (dir == HitDir::kUp && info.move.y <= 0.0f)
		return; // 上昇時のみ天井判定
	if (dir == HitDir::kDown && info.move.y >= 0.0f)
		return; // 下降時のみ床判定
	if (dir == HitDir::kRight && info.move.x <= 0.0f)
		return; // 右移動時のみ右壁判定
	if (dir == HitDir::kLeft && info.move.x >= 0.0f)
		return; // 左移動時のみ左壁判定

	bool hit = false;
	MapChipField::IndexSet indexSet;
	MapChipType mapChipType, mapChipTypeNext;

	// 方向ごとに「見る角」「隣セル方向」を切り替える（Playerと同じ）
	auto testTwoPoints = [&](Corner a, Corner b, int dx, int dy) {
		// 1点目
		indexSet = mapChipField_->GetMapChipIndexSetByPosition(positionsNew[static_cast<uint32_t>(a)]);
		mapChipType = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex);
		mapChipTypeNext = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex + dx, indexSet.yIndex + dy);
		if (mapChipType == MapChipType::kBlock && mapChipTypeNext != MapChipType::kBlock)
			hit = true;

		// 2点目
		indexSet = mapChipField_->GetMapChipIndexSetByPosition(positionsNew[static_cast<uint32_t>(b)]);
		mapChipType = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex);
		mapChipTypeNext = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex + dx, indexSet.yIndex + dy);
		if (mapChipType == MapChipType::kBlock && mapChipTypeNext != MapChipType::kBlock)
			hit = true;
	};

	switch (dir) {
	case HitDir::kUp: {
		// 左上/右上、隣セルは +y（天井）
		testTwoPoints(kLeftTop, kRightTop, 0, +1);
		if (!hit)
			return;

		// 「移動後の上端」基準セルを取得して rect.bottom から詰める（Playerと同式）
		indexSet = mapChipField_->GetMapChipIndexSetByPosition(worldTransform_.translation_ + info.move + Vector3(0.0f, -kHeight / 2.0f + kBlank, 0.0f));
		MapChipField::IndexSet now = mapChipField_->GetMapChipIndexSetByPosition(worldTransform_.translation_ + Vector3(0.0f, kHeight / 2.0f + kBlank, 0.0f));
		if (now.yIndex != indexSet.yIndex) {
			MapChipField::Rect rect = mapChipField_->GetRectByIndex(indexSet.xIndex, indexSet.yIndex);
			float moveY = (rect.bottom - worldTransform_.translation_.y) - (kHeight / 2.0f + kBlank);
			info.move.y = (std::max)(0.0f, moveY); // 上方向は正に詰める
			info.cellingCollision = true;
		}
		break;
	}

	case HitDir::kDown: {
		// 左下/右下、隣セルは -y（床）
		testTwoPoints(kLeftBottom, kRightBottom, 0, -1);
		if (!hit)
			return;

		// 左下のセルを起点に rect.top から詰める（Playerと同式）
		indexSet = mapChipField_->GetMapChipIndexSetByPosition(positionsNew[kLeftBottom]);
		MapChipField::IndexSet now = mapChipField_->GetMapChipIndexSetByPosition(worldTransform_.translation_ - Vector3(0.0f, kHeight / 2.0f, 0.0f));
		if (now.yIndex != indexSet.yIndex) {
			MapChipField::Rect rect = mapChipField_->GetRectByIndex(indexSet.xIndex, indexSet.yIndex);
			float moveY = (rect.top - worldTransform_.translation_.y) + (kHeight / 2.0f + kBlank);
			info.move.y = (std::min)(0.0f, moveY); // 下方向は負に詰める
			info.floorCollision = true;
		}
		break;
	}

	case HitDir::kRight: {
		// 右下/右上、隣セルは x-1（ブロック左端に詰める）
		testTwoPoints(kRightBottom, kRightTop, -1, 0);
		if (!hit)
			return;

		indexSet = mapChipField_->GetMapChipIndexSetByPosition(worldTransform_.translation_ + info.move + Vector3(kWidth / 2.0f + kBlank, 0.0f, 0.0f));
		MapChipField::Rect rect = mapChipField_->GetRectByIndex(indexSet.xIndex, indexSet.yIndex);
		float moveX = (rect.left - worldTransform_.translation_.x) - (kWidth / 2.0f + kBlank);
		info.move.x = (std::max)(0.0f, moveX);
		info.wallCollision = true;
		break;
	}

	case HitDir::kLeft: {
		// 左下/左上、隣セルは x+1（ブロック右端に詰める）
		testTwoPoints(kLeftBottom, kLeftTop, +1, 0);
		if (!hit)
			return;

		indexSet = mapChipField_->GetMapChipIndexSetByPosition(worldTransform_.translation_ + info.move - Vector3(kWidth / 2.0f + kBlank, 0.0f, 0.0f));
		MapChipField::Rect rect = mapChipField_->GetRectByIndex(indexSet.xIndex, indexSet.yIndex);
		float moveX = (rect.right - worldTransform_.translation_.x) + (kWidth / 2.0f + kBlank);
		info.move.x = (std::min)(0.0f, moveX);
		info.wallCollision = true;
		break;
	}

	default:
		break;
	}
}

// ============================
// 衝突判定後の移動量を適用
// ============================
void Enemy::MoveByCollisionCheck(const CollisionMapInfo& info) {
	worldTransform_.translation_ += info.move; // 詰めた移動量を適用
}

// ============================
// 天井接触時：上昇速度をゼロ
// ============================
void Enemy::OnCellingCollision(CollisionMapInfo& info) {
	if (info.cellingCollision) {
		velocity_.y = 0.0f;
	}
}

// ============================
// 壁接触時の処理：進行方向を反転（＋回転開始要求）
// ============================
void Enemy::OnWallCollision(CollisionMapInfo& info) {
	if (!info.wallCollision)
		return;

	velocity_.x *= -1.0f;    // 次フレームから逆方向へ
	RequestTurnByVelocity();
}

// ============================
// 接地状態の切り替え処理
// ============================
void Enemy::SwitchOnGround(CollisionMapInfo& info) {

	if (onGround_) {
		// 上向き速度が出たら空中へ
		if (velocity_.y > 0.0f) {
			onGround_ = false;
		} else {
			// 足元にブロックが無ければ落下開始
			std::array<Vector3, kNumCorner> positionsNew;
			for (uint32_t i = 0; i < positionsNew.size(); ++i) {
				positionsNew[i] = CornerPosition(worldTransform_.translation_ + info.move, static_cast<Corner>(i));
			}

			bool hit = false;
			MapChipField::IndexSet indexSet;
			MapChipType mapChipType;

			// 左下
			indexSet = mapChipField_->GetMapChipIndexSetByPosition(positionsNew[kLeftBottom] + Vector3(0, -kBlank, 0));
			mapChipType = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex);
			if (mapChipType == MapChipType::kBlock) {
				hit = true;
			}

			// 右下
			indexSet = mapChipField_->GetMapChipIndexSetByPosition(positionsNew[kRightBottom] + Vector3(0, -kBlank, 0));
			mapChipType = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex);
			if (mapChipType == MapChipType::kBlock) {
				hit = true;
			}

			// 足元が空なら空中状態へ
			if (!hit) {
				onGround_ = false;
			}
		}

	} else {
		// 空中 → 床衝突で着地
		if (info.floorCollision) {
			onGround_ = true;
			velocity_.x *= (1.0f - kAttenuationLanding); // 着地時の減衰
			velocity_.y = 0.0f;                          // 落下停止
		}
	}
}

// ============================
// 足元の端が空なら反転（freefall=false のときのみ）
// ============================
void Enemy::LedgeTurnCheck() {
	// 条件: マップあり / freefall無効 / 接地中 / 水平に動いている
	if (!mapChipField_ || freefall_ || !onGround_ || velocity_.x == 0.0f) {
		return;
	}

	// 進行方向の“下側の角”を選ぶ
	const bool movingRight = (velocity_.x > 0.0f);
	const Corner footCorner = movingRight ? kRightBottom : kLeftBottom;

	// いまの位置基準の足元角を少しだけ下にずらして、そのセルの種類を調べる
	const Vector3 footPos = CornerPosition(worldTransform_.translation_, footCorner) + Vector3(0.0f, -kBlank, 0.0f);
	MapChipField::IndexSet idx = mapChipField_->GetMapChipIndexSetByPosition(footPos);
	const MapChipType below = mapChipField_->GetMapChipTypeByIndex(idx.xIndex, idx.yIndex);

	// 空（ブロックでない）なら進行方向を反転
	if (below != MapChipType::kBlock) {
		velocity_.x *= -1.0f;
		RequestTurnByVelocity(); // ← イージングで振り向き
	}
}

// ============================
// 速度の符号に合わせてY回転を“即時”決める（初期化専用）
// ============================
void Enemy::FaceByVelocity() {
	// lrDirection_ を現在速度から決定
	lrDirection_ = (velocity_.x >= 0.0f) ? LRDirection::kRight : LRDirection::kLeft;

	// 既存Enemyの向きに合わせて：右=3π/2、左=π/2
	float destinationRotationYTable[] = {
	    std::numbers::pi_v<float> * 3.0f / 2.0f, // 右
	    std::numbers::pi_v<float> / 2.0f,        // 左
	};
	worldTransform_.rotation_.y = destinationRotationYTable[static_cast<uint32_t>(lrDirection_)];
	turnTimer_ = 0.0f; // 初期は即時反映
}

// ============================
// 速度から“振り向き開始”を要求
// ============================
void Enemy::RequestTurnByVelocity() {
	LRDirection newDir = (velocity_.x >= 0.0f) ? LRDirection::kRight : LRDirection::kLeft;
	if (newDir != lrDirection_) {
		lrDirection_ = newDir;
		turnFirstRotation_ = worldTransform_.rotation_.y;
		turnTimer_ = kTimeTurn;
	}
}

// ============================
// イージング回転
// ============================
void Enemy::ModelRotate() {
	float destinationRotationYTable[] = {
	    std::numbers::pi_v<float> * 3.0f / 2.0f, // 右
	    std::numbers::pi_v<float> / 2.0f,        // 左
	};

	if (turnTimer_ > 0.0f) {
		turnTimer_ -= deltaTime_;
		float t = 1.0f - std::clamp(turnTimer_ / kTimeTurn, 0.0f, 1.0f);
		float easedT = EaseInOutSine(t);

		float start = turnFirstRotation_;
		float dest = destinationRotationYTable[static_cast<uint32_t>(lrDirection_)];

		float diff = dest - start;
		while (diff > std::numbers::pi_v<float>)
			diff -= 2.0f * std::numbers::pi_v<float>;
		while (diff < -std::numbers::pi_v<float>)
			diff += 2.0f * std::numbers::pi_v<float>;
		float destReverse = (diff >= 0.0f) ? dest - 2.0f * std::numbers::pi_v<float> : dest + 2.0f * std::numbers::pi_v<float>;

		worldTransform_.rotation_.y = lerp(start, destReverse, easedT);
	} else {
		// 終了時は正規の目的角にピタッと合わせる（±2πは付けない）
		worldTransform_.rotation_.y = destinationRotationYTable[static_cast<uint32_t>(lrDirection_)];
	}
}


// ============================
// 敵同士の衝突判定＆反転処理
// ============================
void Enemy::OnEnemyCollision(Enemy* other) {
	if (!other || other == this) {
		return;
	}

	// AABB で交差判定
	const AABB a = GetAABB();
	const AABB b = other->GetAABB();

	const bool intersect = !(a.max.x < b.min.x || a.min.x > b.max.x || a.max.y < b.min.y || a.min.y > b.max.y || a.max.z < b.min.z || a.min.z > b.max.z);
	if (!intersect) {
		return;
	}

	// 各軸方向の重なり量
	const float overlapX = (std::min)(a.max.x - b.min.x, b.max.x - a.min.x);
	const float overlapY = (std::min)(a.max.y - b.min.y, b.max.y - a.min.y);
	const float overlapZ = (std::min)(a.max.z - b.min.z, b.max.z - a.min.z);

	// 横からの衝突っぽい時だけ反転（縦重なりが大きい時は上下処理に任せる）
	if (overlapX <= overlapY && overlapX <= overlapZ) {
		// 互いを X 方向に半分ずつ押し戻す（少しだけ多めに離して連続反転を防ぐ）
		const float push = overlapX * 0.51f;
		const float dir = (worldTransform_.translation_.x <= other->worldTransform_.translation_.x) ? -1.0f : +1.0f;
		worldTransform_.translation_.x += dir * push;
		other->worldTransform_.translation_.x -= dir * push;

		// 双方の進行方向を反転
		velocity_.x *= -1.0f;
		other->velocity_.x *= -1.0f;

		// イージングで振り向き開始
		RequestTurnByVelocity();
		other->RequestTurnByVelocity();
	}
}
