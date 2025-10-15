#pragma once
#include "../struct.h"
#include <KamataEngine.h>
#include <array>

using namespace KamataEngine;

class Player;
class MapChipField;

class Enemy {
private:
	// モデル
	Model* model_ = nullptr;

	// カメラ
	Camera* camera_ = nullptr;

	// worldTransform
	WorldTransform worldTransform_;

	float deltaTime_; // デフォルトのデルタタイム（60FPS）

	// 当たり判定サイズと余白（Playerと同値）
	static inline const float kWidth = 1.8f;  // 敵の当たり判定サイズ
	static inline const float kHeight = 1.8f; // 敵の当たり判定サイズ
	static inline const float kBlank = 0.2f;  // 当たり判定の余白

	// 歩行の速さ
	static inline const float kWalkSpeed = 3.0f;

	// --- 重力・落下 ---
	static inline const float kGravityAcceleration = 30.0f; // 重力加速度
	static inline const float kLimitFallSpeed = 20.0f;      // 最大落下速度
	static inline const float kAttenuationLanding = 0.0f;   // 着地時の減衰率（Player準拠）

	// 速度
	Vector3 velocity_ = {};

	// 最初の角度[度]
	static inline const float kWalkMotionAngleStart = 0.0f;
	// 最後の角度[度]
	static inline const float kWalkMotionAngleEnd = 20.0f;
	// アニメーションの周期となる時間[秒]
	static inline const float kWalkMotionTime = 1.45f;

	float walkTimer_ = 0.0f; // 歩行アニメーションのタイマー

	// ===== 衝突系の構造 =====
	struct CollisionMapInfo {
		bool cellingCollision = false; // 天井衝突フラグ
		bool floorCollision = false;   // 床衝突フラグ
		bool wallCollision = false;    // 壁衝突フラグ
		Vector3 move;                  // 今フレームの移動量
	};

	enum Corner {
		kRightBottom, // 右下
		kLeftBottom,  // 左下
		kRightTop,    // 右上
		kLeftTop,     // 左上
		kNumCorner    // 要素数
	};

	enum HitDir { kUp, kDown, kRight, kLeft, kCount };

	MapChipField* mapChipField_ = nullptr;

	// 接地状態
	bool onGround_ = false;
	bool isDead_ = false;

	bool freefall_ = true;

	// === 振り向き（イージング） ===
	enum class LRDirection { kRight, kLeft };
	LRDirection lrDirection_ = LRDirection::kLeft; // 初期速度が左のため
	float turnFirstRotation_ = 0.0f;               // 回転開始時の角度
	float turnTimer_ = 0.0f;                       // 残り回転時間
	static inline const float kTimeTurn = 0.45f;   // 回転にかける時間（Playerと同値）

public:
	void Initialize(Camera* camera, const Vector3& position);
	void Update();
	void Draw();

	void SetModel(Model* model) { model_ = model; }
	void SetFreefall(bool enable) { freefall_ = enable; }

	Vector3 GetPosition() const { return worldTransform_.translation_; }
	Vector3 GetRotation() const { return worldTransform_.rotation_; }

	Vector3 GetWorldPosition();
	AABB GetAABB();

	void SetDeltaTime(float deltaTime) { deltaTime_ = deltaTime; }
	void SetMapChipField(MapChipField* mapChipField) { mapChipField_ = mapChipField; }

	void UpdateAffineTransformMatrix();

	void OnPlayerCollision(Player* player);
	void OnEnemyCollision(Enemy* other);

	void Kill() { isDead_ = true; }
	bool IsDead() const { return isDead_; }

private:
	// ====== 分割関数 ======
	void CollisionMapCheck(CollisionMapInfo& info);
	void CollisionOneSide(CollisionMapInfo& info, HitDir dir, const std::array<Vector3, kNumCorner>& positionsNew);
	void MoveByCollisionCheck(const CollisionMapInfo& info);
	Vector3 CornerPosition(const Vector3& center, Corner corner);

	void OnCellingCollision(CollisionMapInfo& info);
	void OnWallCollision(CollisionMapInfo& info);
	void SwitchOnGround(CollisionMapInfo& info);

	void LedgeTurnCheck();

	// 進行方向に合わせて見た目の向きをY回転で合わせる（即時版：初期化で使用）
	void FaceByVelocity();

	// “回転開始要求”（速度の符号変化を検出して turnTimer_ をセット）
	void RequestTurnByVelocity();

	// イージング回転
	void ModelRotate();
};
