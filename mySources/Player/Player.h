#pragma once
#include "../struct.h"
#include <KamataEngine.h>
#include <cmath>
#include <numbers>

using namespace KamataEngine;

enum class LRDirection {
	kRight,
	kLeft,
};

class MapChipField;
class Enemy;

class Player {
private:
	struct CollisionMapInfo {
		// 天井衝突フラグ
		bool cellingCollision = false;
		// 床衝突フラグ
		bool floorCollision = false;
		// 壁衝突フラグ
		bool wallCollision = false;
		// 移動量
		Vector3 move;
	};

	enum Corner {
		kRightBottom, // 右下
		kLeftBottom,  // 左下
		kRightTop,    // 右上
		kLeftTop,     // 左上
		kNumCorner    // 要素数
	};

	// 方向を表す
	enum HitDir { kUp, kDown, kRight, kLeft, kCount };

	// ワールド変換データ
	WorldTransform worldTransform_;
	// モデル
	Model* model_ = nullptr;
	// カメラ
	Camera* camera_ = nullptr;

	Vector3 velocity_ = {};

	float deltaTime_; // デフォルトのデルタタイム

	static inline const float kAcceleration = 32.0f;  // 加速度
	static inline const float kAttenuation = 0.5f;    // 減衰率
	static inline const float kLimitRunSpeed = 40.0f; // 最大速度

	// 移動方向
	LRDirection lrDirection_ = LRDirection::kRight;
	float turnFirstRotation_ = 0.0f; // 初回の向き
	float turnTimer_ = 0.0f;         // 向き変更のタイマー

	static inline const float kTimeTurn = 0.45f; // 向き変更のスピード

	// ジャンプ
	bool onGround_ = false; // 地面にいるかどうか
	bool landing_ = false;  // 着地フラグ
	bool doubleJump_ = false; // 二段ジャンプ可能フラグ
	bool wallJump_ = false;   // 壁ジャンプ可能フラグ

	static inline const float kGravityAcceleration = 30.0f; // 重力加速度
	static inline const float kLimitFallSpeed = 20.0f;      // 最大落下速度
	static inline const float kJumpAcceleration = 20.0f;    // ジャンプ加速度
	static inline const float kJumpX = 40.0f;              // ジャンプ時の横移動力

	// マップチップフィールド
	MapChipField* mapChipField_ = nullptr;

	// プレイヤーの当たり判定サイズ
	static inline const float kWidth = 1.8f;  // プレイヤーの当たり判定サイズ
	static inline const float kHeight = 1.8f; // プレイヤーの当たり判定サイズ

	static inline const float kBlank = 0.2f;              // 当たり判定用の余白
	static inline const float kAttenuationLanding = 0.5f; // 着地時の減衰率
	static inline const float kAttenuationWall = 0.5f;    // 壁に接触したときの減衰率

	bool isDead_ = false; // 死亡フラグ

	CollisionMapInfo collisionMapInfo_;

public:
	void Initialize(Camera* camera, const Vector3& position);

	void Update();

	void Draw();

	// =============================
	// setter/getter
	// =============================

	void SetModel(Model* model) { model_ = model; };

	// 位置を設定する
	void SetPosition(const Vector3& pos) { worldTransform_.translation_ = pos; }

	void SetDeltaTime(float deltaTime) { deltaTime_ = deltaTime; }

	// 位置を取得する
	Vector3 GetPosition() const { return worldTransform_.translation_; }

	Vector3 GetWorldPosition();

	AABB GetAABB();

	const WorldTransform& GetWorldTransform() const { return worldTransform_; }

	const Vector3& GetVelocity() const { return velocity_; }
	bool IsOnGround() const { return onGround_; }

	void SetMapChipField(MapChipField* mapChipField) { mapChipField_ = mapChipField; }

	Vector3 CornerPosition(const Vector3& center, Corner corner);

	bool GetIsDead() const { return isDead_; }

	// =============================
	// メソッド
	// =============================

	void UpdateAffineTransformMatrix();

	// 移動入力
	void Move();
	// ジャンプ入力
	void Jump(CollisionMapInfo& info);
	// モデルの旋回処理
	void ModelRotate();

	// マップ衝突判定
	void CollisionMapCheck(CollisionMapInfo& info);

	// 判定結果を反映して移動させる
	void MoveByCollisionCheck(const CollisionMapInfo& info);

	// 1方向ぶんの当たり判定（positionsNew は事前計算した四隅）
	void CollisionOneSide(CollisionMapInfo& info, HitDir dir, const std::array<Vector3, kNumCorner>& positionsNew);

	// 天井に接触している場合の処理
	void OnCellingCollision(CollisionMapInfo& info);

	// 壁に接触している場合の処理
	void OnWallCollision(CollisionMapInfo& info);

	// 接地状態の切り替え処理
	void SwitchOnGround(CollisionMapInfo& info);

	void OnEnemyCollision(Enemy* enemy);
};
