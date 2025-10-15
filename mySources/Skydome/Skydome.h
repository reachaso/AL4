#pragma once
#include <KamataEngine.h>

using namespace KamataEngine;

class Skydome {

public:
	void Initialize();

	void Update();

	void Draw(const Camera& camera);

	void SetModel(Model* model) { model_ = model; }

private:
	WorldTransform worldTransform_;

	Model* model_ = nullptr;
};
