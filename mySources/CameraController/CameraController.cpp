#include "CameraController.h"
#include "Player/Player.h"
#include <algorithm>

void CameraController::Initialize() {

	camera_->Initialize();

};

void CameraController::Update() {
	// 追従対象のワールド変換を取得
	const WorldTransform& targetWorldTransform = target_->GetWorldTransform();
	// 追従対象とオフセットと追従対象の速度からカメラの目標位置を計算
	targetPosition_ = targetWorldTransform.translation_ + targetOffset_ + target_->GetVelocity() * kVelocityBias;

	// 座標補間によりゆったりとカメラを追従させる
	camera_->translation_ = lerp(camera_->translation_, targetPosition_, kInterpolationRate);

	// 追従対象が画面外に出ないようにカメラの位置を制限
	camera_->translation_.x = std::clamp(camera_->translation_.x, kMargin.left, kMargin.right);
	camera_->translation_.y = std::clamp(camera_->translation_.y, kMargin.bottom, kMargin.top);

	camera_->translation_.x = std::clamp(camera_->translation_.x, movableArea_.left, movableArea_.right);
	camera_->translation_.y = std::clamp(camera_->translation_.y, movableArea_.bottom, movableArea_.top);


	camera_->UpdateMatrix();

};

void CameraController::Reset() {
	const WorldTransform& targetWorldTransform = target_->GetWorldTransform();
	camera_->translation_ = targetWorldTransform.translation_ + targetOffset_;
	// 範囲制限
	camera_->translation_.x = std::clamp(camera_->translation_.x, movableArea_.left, movableArea_.right);
	camera_->translation_.y = std::clamp(camera_->translation_.y, movableArea_.bottom, movableArea_.top);
	camera_->UpdateMatrix();
}
