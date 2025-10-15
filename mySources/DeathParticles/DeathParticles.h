#pragma once
#include <KamataEngine.h>
#include <array>
#include <numbers>

using namespace KamataEngine;

class DeathParticles {

public:
	void Initialize(Camera* camera,Vector3& position);

	void Update();

	void Draw();

	void SetModel(Model* model) { model_ = model; }

	void SetDeltaTime(float deltaTime) { deltaTime_ = deltaTime; }

	void SetPosition(const Vector3& position);

	bool GetIsFinished() const { return isFinished_; }

	void UpdateAffineTransformMatrix();

private:
	Model* model_ = nullptr;

	Camera* camera_ = nullptr;

	// パーティクルの個数
	static inline const uint32_t kNumParticles = 8;

	std::array<WorldTransform, kNumParticles> worldTransforms_;

	// 存続時間
	static inline const float kDuration = 1.0f;
	// 移動の速さ
	static inline const float kSpeed = 0.1f;
	// 分割した1個分の角度
	static inline const float kAngle = std::numbers::pi_v<float> * 2 / kNumParticles;

	bool isFinished_ = false;

	float counter_ = 0.0f;

	float deltaTime_ = 0.0f;

	Vector3 position_;

	ObjectColor objectColor_;
	Vector4 color_;
};
