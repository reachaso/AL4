#pragma once
#include <KamataEngine.h>

class Player;

using namespace KamataEngine;

// +演算子のオーバーロード
inline Vector3 operator+(const Vector3& lhs, const Vector3& rhs) { return {lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z}; }
inline Vector3 operator*(const Vector3& vec, float scalar) { return {vec.x * scalar, vec.y * scalar, vec.z * scalar}; }

inline Vector3 lerp(const Vector3& a, const Vector3& b, float t) { return {a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t}; }

struct Rect {
	float left = 0.0f;
	float right = 1.0f;
	float bottom = 0.0f;
	float top = 1.0f;
};

class CameraController {

public:
	void Initialize();

	void Update();

	void SetTarget(Player* target) { target_ = target; }

	void SetCamera(Camera* camera) { camera_ = camera; }

	void Reset();

	void SetMovableArea(const Rect& area) { movableArea_ = area; }

	Vector3 GetTargetOffset() const { return targetOffset_; }
private:
	Camera* camera_ = nullptr;

	Player* target_ = nullptr;

	Vector3 targetOffset_ = Vector3(0.0f, 5.0f, -30.0f);

	Rect movableArea_ = {0.0f, 100.0f, 0.0f, 100.0f};

	// 座標補間
	Vector3 targetPosition_ = Vector3(0.0f, 0.0f, 0.0f);
	static inline const float kInterpolationRate = 0.1f; // 補間率

	static inline const float kVelocityBias = 0.25f; // 速度のバイアス

	// 追従対象の各方向へのカメラ移動制限
	static inline const Rect kMargin = {-400.0f, 400.0f, -400.0f, 400.0f};
};
