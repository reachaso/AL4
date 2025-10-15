#pragma once
#include "Fade/Fade.h"
#include "Skydome/Skydome.h"
#include "Title/SubTitle.h"
#include "Title/Title.h"
#include <KamataEngine.h>

using namespace KamataEngine;

class TitleScene {

public:
	enum class Phase {
		kFadeIn,
		kMain,
		kFadeOut,
	};

	~TitleScene();

	void Initialize();

	void Update();

	void Draw();

	bool GetIsFinished() const { return isFinished_; }

private:
	bool isFinished_ = false;

	Camera camera_;
	DebugCamera* debugCamera_ = nullptr;
	bool isDebugCameraActive_ = false;

	WorldTransform worldTransform_;

	Title* title_ = nullptr;
	Model* titleModel_ = nullptr;

	SubTitle3D* subTitle3D_ = nullptr;
	Model* subTitleModel_ = nullptr;

	// =======================
	// 天球
	// =======================
	Skydome* skydome_ = nullptr;
	Model* skydomeModel_ = nullptr;

	// =======================
	// fade
	// =======================

	Fade* fade_ = nullptr;
	Phase phase_ = Phase::kFadeIn;
};
