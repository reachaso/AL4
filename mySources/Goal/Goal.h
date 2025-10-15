#pragma once
#include "Player/player.h"
#include "struct.h"
#include <KamataEngine.h>

using namespace KamataEngine;

class Goal {
public:
	void Initialize(Camera* camera, const Vector3& pos);
	void SetModel(Model* model) { model_ = model; }
	void SetDeltaTime(float dt) { deltaTime_ = dt; }

	void Update();

	void Draw();

	AABB GetAABB() const;

	void OnPlayerCollision(Player* /*player*/) { reached_ = true; }
	bool IsReached() const { return reached_; }

private:
	WorldTransform worldTransform_{};
	Model* model_ = nullptr;
	Camera* camera_ = nullptr;
	float deltaTime_ = 0.0f;
	bool reached_ = false;
};
