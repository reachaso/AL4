#include "Coin.h"

using namespace KamataEngine::MathUtility;

void Coin::Initialize(Camera* camera, const Vector3& position) {
	camera_ = camera;
	worldTransform_.Initialize();
	worldTransform_.translation_ = position;
	worldTransform_.scale_ = {1.0f, 1.0f, 1.0f};
}

void Coin::Update() {
	if (isCollected_) {
		return;
	}
	// くるくる回すだけの簡単なアニメ
	worldTransform_.rotation_.y += rotateSpeed_ * deltaTime_;
	Matrix4x4 S = MakeScaleMatrix(worldTransform_.scale_);
	Matrix4x4 Rz = MakeRotateZMatrix(worldTransform_.rotation_.z);
	Matrix4x4 Ry = MakeRotateYMatrix(worldTransform_.rotation_.y);
	Matrix4x4 Rx = MakeRotateXMatrix(worldTransform_.rotation_.x);
	Matrix4x4 R = Rz * Ry * Rx;
	Matrix4x4 T = MakeTranslateMatrix(worldTransform_.translation_);
	worldTransform_.matWorld_ = S * R * T;
	worldTransform_.TransferMatrix();
}

void Coin::Draw() {
	if (isCollected_ || !model_) {
		return;
	}
	model_->Draw(worldTransform_, *camera_);
}

// AABB を返す（小さめに 0.5f）
AABB Coin::GetAABB() const {
	const float half = 0.5f;
	Vector3 c = worldTransform_.translation_;
	if (isCollected_) {
		return AABB{
		    {c.x, c.y, c.z},
            {c.x, c.y, c.z}
        }; // 収集後は実質 0
	}
	return AABB{
	    {c.x - half, c.y - half, c.z - half},
        {c.x + half, c.y + half, c.z + half}
    };
}

// プレイヤーに触れたら取得扱い
void Coin::OnPlayerCollision(Player*) {
	if (!isCollected_) {
		isCollected_ = true;
		// 必要なら効果音をここで再生してOK
		// Audio::GetInstance()->PlayWave(someHandle);
	}
}
