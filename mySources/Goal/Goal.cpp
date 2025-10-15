#include "Goal.h"

using namespace KamataEngine::MathUtility;

void Goal::Initialize(Camera* camera, const Vector3& pos) {
	camera_ = camera;
	worldTransform_.Initialize();
	worldTransform_.translation_ = pos;
}

void Goal::Update() {
	using namespace KamataEngine::MathUtility;
	// ほんの少し回して存在感を出す（不要なら消してOK）
	worldTransform_.rotation_.y += (90.0f * 3.1415926535f / 180.0f) * deltaTime_;
	Matrix4x4 S = MakeScaleMatrix(worldTransform_.scale_);
	Matrix4x4 Rz = MakeRotateZMatrix(worldTransform_.rotation_.z);
	Matrix4x4 Ry = MakeRotateYMatrix(worldTransform_.rotation_.y);
	Matrix4x4 Rx = MakeRotateXMatrix(worldTransform_.rotation_.x);
	Matrix4x4 R = Rz * Ry * Rx;
	Matrix4x4 T = MakeTranslateMatrix(worldTransform_.translation_);
	worldTransform_.matWorld_ = S * R * T;
	worldTransform_.TransferMatrix();
}

void Goal::Draw() {
	if (model_) {
		model_->Draw(worldTransform_, *camera_);
	}
}

AABB Goal::GetAABB() const {
	// ブロック中央返却前提の座標系なので、控えめな半径で当たりを取る
	const Vector3 half = {0.45f, 0.45f, 0.45f};
	return AABB{worldTransform_.translation_ - half, worldTransform_.translation_ + half};
}
