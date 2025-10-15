#pragma once
#include "GameScene/GameScene.h"
#include "TitleScene/TitleScene.h"
#include "SelectScene/StageSelectScene.h"
#include <KamataEngine.h>

using namespace KamataEngine;

class Game {

public:

	enum class Scene {
		kUnknown,
		kTitle,
		kStageSelect,
		kGame,
	};

	~Game();

	void Inisialize();

	void Update();

	void Draw();

private:

	// ImGuiインスタンスの取得
	ImGuiManager* imguiManager = ImGuiManager::GetInstance();

	Scene scene = Scene::kUnknown;

	TitleScene* titleScene = nullptr;
	StageSelectScene* stageSelectScene = nullptr;
	GameScene* gameScene = nullptr;

	std::string lastSelectedCSV_ = "Resources/csv/stage1.csv";

	void ChangeScene();

	void UpdateScene();

	void DrawScene();

	void ForceChangeScene(Scene next);
};
