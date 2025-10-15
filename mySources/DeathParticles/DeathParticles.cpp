#include "DeathParticles.h"
#include <algorithm>

using namespace KamataEngine::MathUtility;

void DeathParticles::Initialize(Camera* camera, Vector3& position) {

	camera_ = camera;
	camera_->Initialize();

	objectColor_.Initialize();
	color_ = {1.0f, 1.0f, 1.0f, 1.0f};

	position_ = position;

	for (WorldTransform& worldTransform : worldTransforms_) {
		worldTransform.Initialize();
		worldTransform.translation_ = position_;
	}
}

void DeathParticles::Update() {

	if (isFinished_) {
		return; // すでに終了している場合は何もしない
	}

	for (uint32_t i = 0; i < kNumParticles; ++i) {
		// 基本となる速度ベクトル
		Vector3 velocity = {kSpeed, 0.0f, 0.0f};
		// 回転角を計算する
		float angle = kAngle * i;

		// z軸回り回転行列
		Matrix4x4 matrixRotation = MakeRotateZMatrix(angle);
		// 基本ベクトルを回転させて速度ベクトルを得る
		velocity = Transform(velocity, matrixRotation);
		// 移動処理
		worldTransforms_[i].translation_ += velocity;
	}

	counter_ += deltaTime_;

	if (counter_ >= kDuration) {

		counter_ = kDuration;
		isFinished_ = true;
	}

	color_.w = std::clamp(kDuration - counter_, 0.0f, 1.0f);
	objectColor_.SetColor(color_);

	// アフィン変換行列の更新
	UpdateAffineTransformMatrix();

}

void DeathParticles::Draw() {

	if (isFinished_) {
		return; // すでに終了している場合は何もしない
	}

	for (WorldTransform& worldTransform : worldTransforms_) {
		
		model_->Draw(worldTransform, *camera_,&objectColor_);
	}

}

// =======================
// アフィン変換行列の更新処理
// =======================

void DeathParticles::UpdateAffineTransformMatrix() {

	for (WorldTransform& worldTransform : worldTransforms_) {

		// スケール行列
		Matrix4x4 scaleMat = MakeScaleMatrix(worldTransform.scale_);
		// 回転行列
		Matrix4x4 rotZMat = MakeRotateZMatrix(worldTransform.rotation_.z);
		Matrix4x4 rotYMat = MakeRotateYMatrix(worldTransform.rotation_.y);
		Matrix4x4 rotXMat = MakeRotateXMatrix(worldTransform.rotation_.x);
		Matrix4x4 rotMat = rotZMat * rotYMat * rotXMat;
		// 平行移動行列
		Matrix4x4 transMat = MakeTranslateMatrix(worldTransform.translation_);
		// アフィン変換行列の計算
		worldTransform.matWorld_ = scaleMat * rotMat * transMat;

		// 行列を転送
		worldTransform.TransferMatrix();
	}
}

void DeathParticles::SetPosition(const Vector3& position) {
	position_ = position;
	for (WorldTransform& worldTransform : worldTransforms_) {
		worldTransform.translation_ = position_;
	}
}
