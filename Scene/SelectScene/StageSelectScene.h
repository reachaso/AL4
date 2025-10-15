#pragma once
#include "Fade/Fade.h"
#include "Skydome/Skydome.h"
#include <KamataEngine.h>
#include <string>
#include <vector>
#include <memory> 
#include <string>

using namespace KamataEngine;

class StageSelectScene {

public:
	enum class Phase { kFadeIn, kMain, kFadeOut };

	~StageSelectScene();

	void Initialize();
	void Update();
	void Draw();

	bool GetIsFinished() const { return isFinished_; }
	const std::string& GetSelectedStageCSV() const { return selectedStageCSV_; }

private:
	bool isFinished_ = false;
	Phase phase_ = Phase::kFadeIn;

	// カメラ
	Camera camera_;
	DebugCamera* debugCamera_ = nullptr;
	bool isDebugCameraActive_ = false;

	// フェード
	Fade* fade_ = nullptr;

	// 天球
	Skydome* skydome_ = nullptr;
	Model* skydomeModel_ = nullptr;

	// エントリ
	struct Entry {
		std::string label;
		std::string csvPath;
	};
	std::vector<Entry> entries_;
	int selected_ = 0;

	// ステージ
	Model* stage1Model_ = nullptr;
	Model* stage2Model_ = nullptr;
	Model* stage3Model_ = nullptr;
	Model* stage4Model_ = nullptr;
	Model* stage5Model_ = nullptr;
	Model* stage6Model_ = nullptr;

	 // ▼ 記憶（シーン間で保持するため static）
	static int s_lastSelected_;            // 最後に確定したインデックス
	static std::string s_lastSelectedCSV_; // 最後に確定したCSVパス

	// 並べて描画するための配列（モデル＆トランスフォーム）
	std::vector<Model*> stageModels_;
	std::vector<std::unique_ptr<WorldTransform>> stageWorlds_;

	// レイアウト設定
	float stageSpacing_ = 30.0f; // モデル間のX間隔
	float scaleCenter_ = 4.00f;  // 中央の拡大率
	float scaleSide_ = 2.70f;    // 周辺の縮小率

	// 決定結果
	std::string selectedStageCSV_ = "Resources/csv/stage1.csv";

	 // ▼アニメ用バッファ
	std::vector<float> startX_, targetX_;
	std::vector<float> startS_, targetS_;
	float animT_ = 1.0f;      // 1.0=停止中
	float animSpeed_ = 0.03f; // 1フレで進める量（好みで調整）
	bool animating_ = false;

	// ▼レイアウト
	// UpdateLayout_ を「目標をセットしてアニメ開始」に変更（初回のみ即時反映したいので引数追加）
	void UpdateLayout_(bool snap = false);
	void StepLayoutAnimation_(); // 毎フレ更新
	void UpdateAffineTransformMatrix_(WorldTransform& wt);

	// ガイド
	Model* AButtonModel_ = nullptr;
	Model* DButtonModel_ = nullptr;

	// ガイドのWT
	std::unique_ptr<WorldTransform> AButtonWorld_;
	std::unique_ptr<WorldTransform> DButtonWorld_;

	// ガイド配置のチューニング
	float guideOffsetX_ = 14.0f; // 選択中モデルの左右距離（spacingが30なら半分弱）
	float guideOffsetY_ = 0.0f;  // 少し上げたい場合に
	float guideScale_ = 1.8f;    // ガイド自体の大きさ

	// ガイド更新
	void UpdateGuides_();
};
