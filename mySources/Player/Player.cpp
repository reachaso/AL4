#define NOMIMAX
#include "Player.h"
#include "MapChipField/MapChipField.h"
#include "cassert"
#include <algorithm>

using namespace KamataEngine::MathUtility;

void Player::Initialize(Camera* camera, const Vector3& position) {

	camera_ = camera;

	worldTransform_.Initialize();
	worldTransform_.translation_ = position;
	worldTransform_.rotation_.y = std::numbers::pi_v<float> / 2.0f; // 初期状態でZ軸を向くようにY軸を90度回転
}

void Player::Update() {

	// =======================
	// 移動処理
	// =======================

	collisionMapInfo_ = {}; // 衝突判定情報を初期化

	collisionMapInfo_.move = velocity_ * deltaTime_; // 移動量を設定

	Move(); // 移動処理

	CollisionMapCheck(collisionMapInfo_); // 移動量を加味して衝突判定を行う

	MoveByCollisionCheck(collisionMapInfo_); // 衝突判定後の移動量を適用

	OnCellingCollision(collisionMapInfo_); // 天井と接触している場合

	//OnFloorCollision(collisionMapInfo); // 床と接触している場合

	OnWallCollision(collisionMapInfo_); // 壁と接触している場合

	Jump(collisionMapInfo_); // ジャンプ処理

	SwitchOnGround(collisionMapInfo_); // 接地状態の切り替え処理

	ModelRotate(); // 回転処理

	UpdateAffineTransformMatrix(); // アフィン変換行列の更新
}

// =======================
// 描画処理
// =======================

void Player::Draw() { model_->Draw(worldTransform_, *camera_); }


Vector3 Player::GetWorldPosition() {
	
	Vector3 worldPos;

	worldPos.x = worldTransform_.translation_.x;
	worldPos.y = worldTransform_.translation_.y;
	worldPos.z = worldTransform_.translation_.z;
	
	return worldPos;

}

AABB Player::GetAABB() {

	Vector3 worldPos = GetWorldPosition();

	AABB aabb;

	// キャラクターの半径分マイナスした座標を最小値
	aabb.min = Vector3(worldPos.x - kWidth / 2.0f, worldPos.y - kHeight / 2.0f, worldPos.z - kWidth / 2.0f);
	// キャラクターの半径分プラスした座標を最大値
	aabb.max = Vector3(worldPos.x + kWidth / 2.0f, worldPos.y + kHeight / 2.0f, worldPos.z + kWidth / 2.0f);

	return aabb; 
}

// =======================
// アフィン変換行列の更新処理
// =======================

void Player::UpdateAffineTransformMatrix() {
	// スケール行列
	Matrix4x4 scaleMat = MakeScaleMatrix(worldTransform_.scale_);
	// 回転行列
	Matrix4x4 rotZMat = MakeRotateZMatrix(worldTransform_.rotation_.z);
	Matrix4x4 rotYMat = MakeRotateYMatrix(worldTransform_.rotation_.y);
	Matrix4x4 rotXMat = MakeRotateXMatrix(worldTransform_.rotation_.x);
	Matrix4x4 rotMat = rotZMat * rotYMat * rotXMat;
	// 平行移動行列
	Matrix4x4 transMat = MakeTranslateMatrix(worldTransform_.translation_);
	// アフィン変換行列の計算
	worldTransform_.matWorld_ = scaleMat * rotMat * transMat;

	// 行列を転送
	worldTransform_.TransferMatrix();
}

// =======================
// 移動処理
// =======================

void Player::Move() {

    // 移動入力  
    if (Input::GetInstance()->PushKey(DIK_RIGHT) || Input::GetInstance()->PushKey(DIK_D) || Input::GetInstance()->PushKey(DIK_LEFT) || Input::GetInstance()->PushKey(DIK_A)) {  
        Vector3 acceleration = {};  

        if (Input::GetInstance()->PushKey(DIK_RIGHT) || Input::GetInstance()->PushKey(DIK_D)) {  

            if (lrDirection_ != LRDirection::kRight) {  
                lrDirection_ = LRDirection::kRight;  
                turnFirstRotation_ = worldTransform_.rotation_.y;  
                turnTimer_ = kTimeTurn;  
            }  

            if (velocity_.x < 0.0f) {  
                velocity_.x *= (1.0f - kAttenuation);  
            }  

            acceleration.x += kAcceleration;  
        } else if (Input::GetInstance()->PushKey(DIK_LEFT) || Input::GetInstance()->PushKey(DIK_A)) {  

            if (lrDirection_ != LRDirection::kLeft) {  
                lrDirection_ = LRDirection::kLeft;  
                turnFirstRotation_ = worldTransform_.rotation_.y;  
                turnTimer_ = kTimeTurn;  
            }  

            if (velocity_.x > 0.0f) {  
                velocity_.x *= (1.0f - kAttenuation);  
            }  

            acceleration.x -= kAcceleration;  
        }  

        velocity_ += acceleration * deltaTime_;  
        velocity_.x = std::clamp(velocity_.x, -kLimitRunSpeed, kLimitRunSpeed);  

    } else {  
        velocity_.x *= (1.0f - kAttenuation);  
    }  

	// 落下処理
	if (!onGround_) {
		// 通常の落下
		velocity_ += Vector3(0.0f, -kGravityAcceleration * deltaTime_, 0.0f);
		velocity_.y = (std::max)(velocity_.y, -kLimitFallSpeed);

		return; // 空中はここで終了
	}
}

void Player::Jump(CollisionMapInfo& info) {
	// ジャンプ入力
	if (Input::GetInstance()->TriggerKey(DIK_SPACE) || Input::GetInstance()->TriggerKey(DIK_UP)) {

		// 壁に接触していて、かつ空中であれば壁ジャンプ
		if (info.wallCollision && !onGround_) {
			if (wallJump_) {
				return; // 既に壁ジャンプしていたら何もしない
			}
			velocity_.y = 0.0f; // Y速度をリセット
			velocity_.x = 0.0f; // 壁ジャンプ時にX速度をリセット
			velocity_ += Vector3((lrDirection_ == LRDirection::kRight) ? -kJumpX : kJumpX, kJumpAcceleration, 0.0f);

			wallJump_ = true;    // 壁ジャンプ使用中フラグを立てる
			//doubleJump_ = false; // 壁ジャンプをしたら2段ジャンプの権利を復活させる

		}
		// それ以外の場合のジャンプ処理
		else {
			// 通常ジャンプ (地上)
			if (onGround_) {
				velocity_ += Vector3(0.0f, kJumpAcceleration, 0.0f);
			}
			// 2段ジャンプ (空中)
			else if (!doubleJump_) {
				velocity_.y = 0.0f; // Y速度をリセットしてからジャンプ
				velocity_ += Vector3(0.0f, kJumpAcceleration, 0.0f);
				doubleJump_ = true; // 2段ジャンプを使用済みにする
			}
		}

		// ジャンプが実行された場合（速度が上向きになった場合）
		if (velocity_.y > 0.0f) {
			onGround_ = false;
			landing_ = false;
		}
	}
}

// =======================
// 回転処理
// =======================

void Player::ModelRotate() {

	if (turnTimer_ > 0.0f) { // 回転中の処理
		turnTimer_ -= deltaTime_;

		float destinationRotationYTable[] = {
		    std::numbers::pi_v<float> / 2.0f,        // 右方向
		    std::numbers::pi_v<float> * 3.0f / 2.0f, // 左方向
		};

		float destinationRotationY = destinationRotationYTable[static_cast<uint32_t>(lrDirection_)];
		float t = 1.0f - std::clamp(turnTimer_ / kTimeTurn, 0.0f, 1.0f);
		float easedT = easeInOutSine(t);
		worldTransform_.rotation_.y = lerp(turnFirstRotation_, destinationRotationY, easedT);
	} else {
		float destinationRotationYTable[] = {
		    std::numbers::pi_v<float> / 2.0f,        // 右方向
		    std::numbers::pi_v<float> * 3.0f / 2.0f, // 左方向
		};
		worldTransform_.rotation_.y = destinationRotationYTable[static_cast<uint32_t>(lrDirection_)];
	}
}

// ============================
// 衝突判定を行う
// ============================

void Player::CollisionMapCheck(CollisionMapInfo& info) {

	// 事前に移動後の四隅を一括算出（元コードの繰り返しを削減）
	std::array<Vector3, kNumCorner> positionsNew;
	for (uint32_t i = 0; i < positionsNew.size(); ++i) {
		positionsNew[i] = CornerPosition(worldTransform_.translation_ + info.move, static_cast<Corner>(i));
	} // 元の各関数で毎回やっていた処理をここで一度だけ実施 :contentReference[oaicite:10]{index=10}

	// 上下左右の順で4回呼ぶ
	const HitDir order[] = {HitDir::kUp, HitDir::kDown, HitDir::kRight, HitDir::kLeft};
	for (HitDir d : order) {
		CollisionOneSide(info, d, positionsNew);
	}
}

// ============================
// 角の座標を計算
// ============================

Vector3 Player::CornerPosition(const Vector3& center, Corner corner) {

	Vector3 offsetTable[kNumCorner] = {
	    {+kWidth / 2.0f, -kHeight / 2.0f, 0.0f}, //  右下
	    {-kWidth / 2.0f, -kHeight / 2.0f, 0.0f}, //  左下
	    {+kWidth / 2.0f, +kHeight / 2.0f, 0.0f}, //  右上
	    {-kWidth / 2.0f, +kHeight / 2.0f, 0.0f}  //  左上
	};
	return center + offsetTable[static_cast<uint32_t>(corner)];
}

// ============================
// 衝突判定後の移動量を適用
// ============================

void Player::MoveByCollisionCheck(const CollisionMapInfo& info) {

	worldTransform_.translation_ += info.move; // 衝突判定後の移動量を適用
}

// ============================
// 衝突判定を行う（方向ごとに）
// ============================

void Player::CollisionOneSide(CollisionMapInfo& info, HitDir dir, const std::array<Vector3, kNumCorner>& positionsNew) {
	// 方向別の早期 return 条件
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

	// 方向ごとに「見る角」「隣セル方向」を切り替える
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
	case HitDir::kUp:
		// 左上/右上、隣セルは +y（天井）
		testTwoPoints(kLeftTop, kRightTop, 0, +1);
		if (!hit)
			return;

		// 「移動後の上端」基準セルを取得して、rect.bottom から詰める（元実装）
		indexSet = mapChipField_->GetMapChipIndexSetByPosition(worldTransform_.translation_ + info.move + Vector3(0.0f, -kHeight / 2.0f + kBlank, 0.0f));
		// 直前セルの yIndex と違うときだけ当て込む（揺れ防止の元ロジック）
		{
			MapChipField::IndexSet now = mapChipField_->GetMapChipIndexSetByPosition(worldTransform_.translation_ + Vector3(0.0f, kHeight / 2.0f + kBlank, 0.0f));
			if (now.yIndex != indexSet.yIndex) {
				MapChipField::Rect rect = mapChipField_->GetRectByIndex(indexSet.xIndex, indexSet.yIndex);
				float moveY = (rect.bottom - worldTransform_.translation_.y) - (kHeight / 2.0f + kBlank);
				info.move.y = (std::max)(0.0f, moveY); // 上方向は正に詰める
				info.cellingCollision = true;
			}
		}
		break;

	case HitDir::kDown:
		// 左下/右下、隣セルは -y（床）
		testTwoPoints(kLeftBottom, kRightBottom, 0, -1);
		if (!hit)
			return;

		// 左下のセルを起点に、直前セル yIndex 変化を見て rect.top から詰める（元実装）
		indexSet = mapChipField_->GetMapChipIndexSetByPosition(positionsNew[kLeftBottom]);
		{
			MapChipField::IndexSet now = mapChipField_->GetMapChipIndexSetByPosition(worldTransform_.translation_ - Vector3(0.0f, kHeight / 2.0f, 0.0f));
			if (now.yIndex != indexSet.yIndex) {
				MapChipField::Rect rect = mapChipField_->GetRectByIndex(indexSet.xIndex, indexSet.yIndex);
				float moveY = (rect.top - worldTransform_.translation_.y) + (kHeight / 2.0f + kBlank);
				info.move.y = (std::min)(0.0f, moveY); // 下方向は負に詰める 
				info.floorCollision = true;
			}
		}
		break;

	case HitDir::kRight:
		// 右下/右上、隣セルは x-1（ブロック左端と接触で詰め）
		testTwoPoints(kRightBottom, kRightTop, -1, 0);
		if (!hit)
			return;

		indexSet = mapChipField_->GetMapChipIndexSetByPosition(worldTransform_.translation_ + info.move + Vector3(kWidth / 2.0f + kBlank, 0.0f, 0.0f));
		{
			MapChipField::Rect rect = mapChipField_->GetRectByIndex(indexSet.xIndex, indexSet.yIndex);
			float moveX = (rect.left - worldTransform_.translation_.x) - (kWidth / 2.0f + kBlank);
			info.move.x = (std::max)(0.0f, moveX); // 右方向は正に詰める
			info.wallCollision = true;
		}
		break;

	case HitDir::kLeft:
		// 左下/左上、隣セルは x+1（ブロック右端と接触で詰め）
		testTwoPoints(kLeftBottom, kLeftTop, +1, 0);
		if (!hit)
			return;

		indexSet = mapChipField_->GetMapChipIndexSetByPosition(worldTransform_.translation_ + info.move - Vector3(kWidth / 2.0f + kBlank, 0.0f, 0.0f));
		{
			MapChipField::Rect rect = mapChipField_->GetRectByIndex(indexSet.xIndex, indexSet.yIndex);
			float moveX = (rect.right - worldTransform_.translation_.x) + (kWidth / 2.0f + kBlank);
			info.move.x = (std::min)(0.0f, moveX); // 左方向は負に詰める 
			info.wallCollision = true;
		}
		break;

	default:
		break;
	}
}

void Player::OnCellingCollision(CollisionMapInfo& info) {
	if (info.cellingCollision) {
		velocity_.y = 0.0f; // 天井に当たったら上昇速度をリセット
	}
}

void Player::OnWallCollision(CollisionMapInfo& info) {
	if (info.wallCollision) {
		// 接地状態ならX速度を減衰
		velocity_.x *= (1.0f - kAttenuationWall);
	}
}

// ============================
// 接地状態の切り替え処理
// ============================

void Player::SwitchOnGround(CollisionMapInfo& info) {

	if (onGround_) {

		// 接地状態の処理
		if (velocity_.y > 0.0f) {
			onGround_ = false; // 接地状態を解除
		} else {

			// 　落下判定
			//  移動後の4つの角の座標を計算
			std::array<Vector3, kNumCorner> positionsNew;
			for (uint32_t i = 0; i < positionsNew.size(); ++i) {
				positionsNew[i] = CornerPosition(worldTransform_.translation_ + info.move, static_cast<Corner>(i));
			}

			MapChipType mapChipType;
			MapChipField::IndexSet indexSet;
			bool hit = false;

			// 左下点の判定
			indexSet = mapChipField_->GetMapChipIndexSetByPosition(positionsNew[kLeftBottom] + Vector3(0, -kBlank, 0));
			mapChipType = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex);
			if (mapChipType == MapChipType::kBlock) {
				hit = true;
			}

			// 右下点の判定
			indexSet = mapChipField_->GetMapChipIndexSetByPosition(positionsNew[kRightBottom] + Vector3(0, -kBlank, 0));
			mapChipType = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex);
			if (mapChipType == MapChipType::kBlock) {
				hit = true;
			}

			// 落下開始
			if (!hit) {
				// 落下なら空中状態に切り替え
				onGround_ = false;
			}
		}

	} else {

		// 空中状態の処理

		// 着地フラグ
		if (info.floorCollision) {
			// 着地状態に切り替える(落下を止める)
			onGround_ = true;
			// 着地時にX速度を減衰
			velocity_.x *= (1.0f - kAttenuationLanding);
			// Y速度をゼロにする
			velocity_.y = 0.0f;

			doubleJump_ = false; // 2段ジャンプをリセット
			wallJump_ = false;   // 壁ジャンプ使用中フラグをリセット
		}
	}
}

// ============================
// 敵との衝突処理
// ============================

void Player::OnEnemyCollision(Enemy* enemy) {

	enemy;

	isDead_ = true; // プレイヤーの死亡フラグを立てる

}
