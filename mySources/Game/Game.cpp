#include "Game.h"
#include "CommonBGM/CommonBGM.h"

Game::~Game() {
	CommonBGM::GetInstance()->Stop();

	delete titleScene;
	titleScene = nullptr;
	delete stageSelectScene;
	stageSelectScene = nullptr;
	delete gameScene;
	gameScene = nullptr;
}

void Game::Inisialize() {

#ifdef _DEBUG

	// ImGuiのフォント設定
	ImGuiIO& io = ImGui::GetIO();
	ImFont* font = io.Fonts->AddFontFromFileTTF("Resources/font/Huninn-Regular.ttf", 15.0f, nullptr, io.Fonts->GetGlyphRangesJapanese());
	io.FontDefault = font;

	CommonBGM::GetInstance()->Initialize("kiminomamadeii.mp3");
	CommonBGM::GetInstance()->SetVolume(0.1f);
	CommonBGM::GetInstance()->Play();

	const bool kBootFromStageSelect = true;
	if (kBootFromStageSelect) {
		scene = Scene::kStageSelect;
		stageSelectScene = new StageSelectScene();
		stageSelectScene->Initialize();
		return;
	}
#endif

	scene = Scene::kTitle;

	titleScene = new TitleScene();
	// タイトルシーンの初期化
	titleScene->Initialize();

	CommonBGM::GetInstance()->Initialize("kiminomamadeii.mp3");
	CommonBGM::GetInstance()->SetVolume(0.8f);
	CommonBGM::GetInstance()->Play();
}

void Game::Update() {

	// ImGuiの開始
	imguiManager->Begin();

	UpdateScene();

	// ImGuiの終了
	imguiManager->End();
}

void Game::Draw() {

	DrawScene();

	// ImGuiの描画
	imguiManager->Draw();
}

void Game::ChangeScene() {
	switch (scene) {
	case Scene::kTitle:
		if (titleScene->GetIsFinished()) {
			scene = Scene::kStageSelect;
			delete titleScene;
			titleScene = nullptr;

			stageSelectScene = new StageSelectScene();
			stageSelectScene->Initialize();
			CommonBGM::GetInstance()->Resume();
		}
		break;

	case Scene::kStageSelect:
		if (stageSelectScene->GetIsFinished()) {
			lastSelectedCSV_ = stageSelectScene->GetSelectedStageCSV();
			scene = Scene::kGame;
			delete stageSelectScene;
			stageSelectScene = nullptr;

			gameScene = new GameScene();
			gameScene->SetStageCSV(lastSelectedCSV_);
			gameScene->Initialize();
		}
		break;

	case Scene::kGame:
		if (gameScene->GetIsFinished()) {
			scene = Scene::kStageSelect;
			delete gameScene;
			gameScene = nullptr;

			stageSelectScene = new StageSelectScene();
			stageSelectScene->Initialize();
			CommonBGM::GetInstance()->Resume();
		}
		break;
	}
}

void Game::UpdateScene() {

#ifdef _DEBUG
	if (Input::GetInstance()->TriggerKey(DIK_F1)) {
		ForceChangeScene(Scene::kTitle);
	}
	// F2: ステージセレクトへ
	if (Input::GetInstance()->TriggerKey(DIK_F2)) {
		ForceChangeScene(Scene::kStageSelect);
	}
	// F3: 直前に選んだCSVでゲームを即開始
	if (Input::GetInstance()->TriggerKey(DIK_F3)) {
		ForceChangeScene(Scene::kGame);
	}
#endif

	ChangeScene();

	switch (scene) {
	case Scene::kTitle:

		// タイトルシーンの更新
		titleScene->Update();

		break;
	case Scene::kStageSelect:

		stageSelectScene->Update();

		break;
	case Scene::kGame:

		// ゲームシーンの更新
		gameScene->Update();

		break;
	}
}

void Game::DrawScene() {

	switch (scene) {
	case Scene::kTitle:
		// タイトルシーンの描画
		titleScene->Draw();
		break;
	case Scene::kStageSelect: // ←追加
		stageSelectScene->Draw();
		break;
	case Scene::kGame:
		// ゲームシーンの描画
		gameScene->Draw();
		break;
	}
}

void Game::ForceChangeScene(Scene next) {
	// 現在のシーンを解放
	switch (scene) {
	case Scene::kTitle:
		delete titleScene;
		titleScene = nullptr;
		break;
	case Scene::kStageSelect:
		delete stageSelectScene;
		stageSelectScene = nullptr;
		break;
	case Scene::kGame:
		delete gameScene;
		gameScene = nullptr;
		break;
	default:
		break;
	}

	// 目的のシーンへ
	scene = next;
	switch (next) {
	case Scene::kTitle:
		titleScene = new TitleScene();
		titleScene->Initialize();
		break;
	case Scene::kStageSelect:
		stageSelectScene = new StageSelectScene();
		stageSelectScene->Initialize();
		break;
	case Scene::kGame:
		gameScene = new GameScene();
		gameScene->SetStageCSV(lastSelectedCSV_);
		gameScene->Initialize();
		break;
	default:
		break;
	}
}
