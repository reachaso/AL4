#pragma once
#include "struct.h"
#include <KamataEngine.h>

using namespace KamataEngine;

class Player; // 前方宣言（衝突時に参照するだけ）

class Coin {
public:
	void Initialize(Camera* camera, const Vector3& position);

	void SetModel(Model* model) { model_ = model; }
	void SetDeltaTime(float dt) { deltaTime_ = dt; }

	void Update();

	void Draw();

	// AABB を返す（小さめに 0.5f）
	AABB GetAABB() const;

	bool IsCollected() const { return isCollected_; }

	// プレイヤーに触れたら取得扱い
	void OnPlayerCollision(Player*);

private:
	WorldTransform worldTransform_{};
	Camera* camera_ = nullptr;
	Model* model_ = nullptr;
	float deltaTime_ = 1.0f / 60.0f;

	bool isCollected_ = false;
	float rotateSpeed_ = 2.5f; // 適当な回転速度
};
