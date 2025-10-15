#include "Title.h"
#include <cmath> // std::sin

using namespace KamataEngine::MathUtility;

void Title::Initialize(Camera* camera) {
	camera_ = camera;
	worldTransform_.Initialize();

	// 現在の位置を基準にする
	baseTranslation_ = worldTransform_.translation_;
}

void Title::Update() {
	// --- 上下 ---
	floatPhase_ += floatSpeed_;
	// 位相を2πでループ
	if (floatPhase_ > 6.283185307f) {
		floatPhase_ -= 6.283185307f;
	}

	// 基準をベースにYだけサインで上下
	worldTransform_.translation_ = baseTranslation_;
	worldTransform_.translation_.y = baseTranslation_.y + std::sin(floatPhase_) * floatAmplitude_;

	UpdateAffineTransformMatrix();
}

void Title::Draw() {
	if (model_) {
		model_->Draw(worldTransform_, *camera_);
	}
}

// =======================
// アフィン変換行列の更新処理
// =======================
void Title::UpdateAffineTransformMatrix() {
	Matrix4x4 scaleMat = MakeScaleMatrix(worldTransform_.scale_);
	Matrix4x4 rotZMat = MakeRotateZMatrix(worldTransform_.rotation_.z);
	Matrix4x4 rotYMat = MakeRotateYMatrix(worldTransform_.rotation_.y);
	Matrix4x4 rotXMat = MakeRotateXMatrix(worldTransform_.rotation_.x);
	Matrix4x4 rotMat = rotZMat * rotYMat * rotXMat;
	Matrix4x4 transMat = MakeTranslateMatrix(worldTransform_.translation_);
	worldTransform_.matWorld_ = scaleMat * rotMat * transMat;

	worldTransform_.TransferMatrix();
}

Vector3 Title::GetWorldPosition() {
	Vector3 worldPos;
	worldPos.x = worldTransform_.translation_.x;
	worldPos.y = worldTransform_.translation_.y;
	worldPos.z = worldTransform_.translation_.z;
	return worldPos;
}
