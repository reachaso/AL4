#include "StageSelectScene.h"
#include "struct.h"
#include <algorithm>

using namespace MathUtility;

int StageSelectScene::s_lastSelected_ = 0;
std::string StageSelectScene::s_lastSelectedCSV_ = "Resources/csv/stage1.csv";

StageSelectScene::~StageSelectScene() {
	delete skydomeModel_;
	skydomeModel_ = nullptr;
	delete skydome_;
	skydome_ = nullptr;
	delete debugCamera_;
	debugCamera_ = nullptr;
	delete fade_;
	fade_ = nullptr;

	// ステージモデル（個別ポインタを破棄）
	delete stage1Model_;
	stage1Model_ = nullptr;
	delete stage2Model_;
	stage2Model_ = nullptr;
	delete stage3Model_;
	stage3Model_ = nullptr;
	delete stage4Model_;
	stage4Model_ = nullptr;
	delete stage5Model_;
	stage5Model_ = nullptr;
	delete stage6Model_;
	stage6Model_ = nullptr;

	delete AButtonModel_;
	AButtonModel_ = nullptr;
	delete DButtonModel_;
	DButtonModel_ = nullptr;

	// 配列はポインタの所有権なしなのでクリアのみ
	stageModels_.clear();
	stageWorlds_.clear();
}

void StageSelectScene::Initialize() {
	camera_.Initialize();

	// エントリ
	entries_.push_back({"STAGE 1", "Resources/csv/stage1.csv"});
	entries_.push_back({"STAGE 2", "Resources/csv/stage2.csv"});
	entries_.push_back({"STAGE 3", "Resources/csv/stage3.csv"});
	entries_.push_back({"STAGE 4", "Resources/csv/stage4.csv"});
	entries_.push_back({"STAGE 5", "Resources/csv/stage5.csv"});
	entries_.push_back({"STAGE 6", "Resources/csv/stage6.csv"});

	{
		int restored = -1;
		if (!s_lastSelectedCSV_.empty()) {
			for (int i = 0; i < static_cast<int>(entries_.size()); ++i) {
				if (entries_[i].csvPath == s_lastSelectedCSV_) {
					restored = i;
					break;
				}
			}
		}
		if (restored < 0) {
			restored = std::clamp(s_lastSelected_, 0, static_cast<int>(entries_.size()) - 1);
		}
		selected_ = restored;
		// 一応、現在の選択CSVも合わせておく
		selectedStageCSV_ = entries_[selected_].csvPath;
	}

	// ステージモデル
	stage1Model_ = Model::CreateFromOBJ("stage1", true);
	stage2Model_ = Model::CreateFromOBJ("stage2", true);
	stage3Model_ = Model::CreateFromOBJ("stage3", true);
	stage4Model_ = Model::CreateFromOBJ("stage4", true);
	stage5Model_ = Model::CreateFromOBJ("stage5", true);
	stage6Model_ = Model::CreateFromOBJ("stage6", true);

	AButtonModel_ = Model::CreateFromOBJ("A", true);
	DButtonModel_ = Model::CreateFromOBJ("D", true);

	// 並べたい順に追加（entries_ と同じ順）
	stageModels_.push_back(stage1Model_);
	stageModels_.push_back(stage2Model_);
	stageModels_.push_back(stage3Model_);
	stageModels_.push_back(stage4Model_);
	stageModels_.push_back(stage5Model_);
	stageModels_.push_back(stage6Model_);

	// トランスフォームを用意
	stageWorlds_.clear();
	stageWorlds_.reserve(stageModels_.size());
	for (size_t i = 0; i < stageModels_.size(); ++i) {
		stageWorlds_.push_back(std::make_unique<WorldTransform>());
		stageWorlds_.back()->Initialize();
	}
	UpdateLayout_(true); // 初期整列

	// ガイドのWT
	AButtonWorld_ = std::make_unique<WorldTransform>();
	DButtonWorld_ = std::make_unique<WorldTransform>();
	AButtonWorld_->Initialize();
	DButtonWorld_->Initialize();

	// 初期配置（選択復元後の位置に合わせる）
	UpdateGuides_();

	// 天球
	skydome_ = new Skydome();
	skydomeModel_ = Model::CreateFromOBJ("skydome", true);
	skydome_->SetModel(skydomeModel_);
	skydome_->Initialize();

	// フェード
	fade_ = new Fade();
	fade_->Initialize();
	fade_->Start(Fade::Status::FadeIn, 1.0f);

	// デバッグカメラ
	debugCamera_ = new DebugCamera(1280, 720);

	// カメラ初期位置
	camera_.translation_ = {0.0f, 0.0f, -50.0f};
	camera_.UpdateMatrix();
	camera_.TransferMatrix();
}

void StageSelectScene::Update() {

#ifdef _DEBUG
	if (Input::GetInstance()->TriggerKey(DIK_TAB)) {
		isDebugCameraActive_ = !isDebugCameraActive_;
	}
	if (isDebugCameraActive_) {
		debugCamera_->Update();
		camera_.translation_ = debugCamera_->GetCamera().translation_;
		camera_.rotation_ = debugCamera_->GetCamera().rotation_;
		camera_.matView = debugCamera_->GetCamera().matView;
		camera_.matProjection = debugCamera_->GetCamera().matProjection;
		camera_.TransferMatrix();
	} else {
		camera_.UpdateMatrix();
		camera_.TransferMatrix();
	}

	// ImGui の簡易メニュー
	ImGui::Begin("Stage Select");
	ImGui::Text("Choose Stage (A/D or <-/->, Enter/Space to decide)");
	for (int i = 0; i < (int)entries_.size(); ++i) {
		if (i == selected_) {
			ImGui::TextColored(ImVec4(1, 1, 0, 1), " > %s", entries_[i].label.c_str());
		} else {
			ImGui::Text("   %s", entries_[i].label.c_str());
		}
	}
	ImGui::SeparatorText("Transforms");
	ImGui::Text("selected = %d  spacing = %.2f", selected_, stageSpacing_);
	for (size_t i = 0; i < stageWorlds_.size(); ++i) {
		const auto& wt = *stageWorlds_[i];
		ImGui::Text("%c [%zu] pos = (%.2f, %.2f, %.2f)  scale = %.2f", (i == (size_t)selected_) ? '>' : ' ', i, wt.translation_.x, wt.translation_.y, wt.translation_.z, wt.scale_.x);
	}

	ImGui::SeparatorText("World Matrices (CPU)");
	for (size_t i = 0; i < stageWorlds_.size(); ++i) {
		const auto& wt = *stageWorlds_[i];

		// 行列のどこが平行移動かは実装次第（行/列メジャ）。両方表示して確認。
		const auto& M = wt.matWorld_; // ← public ならそのまま使える

		// ① 「m[3][0..2] が平行移動」の系
		ImGui::Text("[%zu] M(3,*) = (%.2f, %.2f, %.2f)", i, M.m[3][0], M.m[3][1], M.m[3][2]);

		// ② 「m[0..2][3] が平行移動」の系（念のため両方出す）
		ImGui::Text("     M(*,3) = (%.2f, %.2f, %.2f)", M.m[0][3], M.m[1][3], M.m[2][3]);
	}

	ImGui::SeparatorText("Layout Tuning");
	bool changed = false;
	changed |= ImGui::SliderFloat("center scale", &scaleCenter_, 0.5f, 10.0f, "%.2f");
	changed |= ImGui::SliderFloat("side scale", &scaleSide_, 0.5f, 10.0f, "%.2f");
	changed |= ImGui::SliderFloat("spacing", &stageSpacing_, 5.0f, 120.0f, "%.1f");

	// 変更があれば並びを更新
	if (changed) {
		UpdateLayout_();
	}

	ImGui::SeparatorText("Guide (A/D) Tuning");
	bool gchanged = false;
	gchanged |= ImGui::SliderFloat("guide offset X", &guideOffsetX_, 1.0f, 80.0f, "%.1f");
	gchanged |= ImGui::SliderFloat("guide offset Y", &guideOffsetY_, -20.0f, 20.0f, "%.1f");
	gchanged |= ImGui::SliderFloat("guide scale", &guideScale_, 0.2f, 10.0f, "%.2f");
	if (gchanged) {
		UpdateGuides_();
	}

	ImGui::End();
#endif

	switch (phase_) {
	case Phase::kFadeIn:
		fade_->Update();
		if (fade_->IsFinished()) {
			phase_ = Phase::kMain;
		}
		break;

	case Phase::kMain:
		skydome_->Update();

		// 左右（A/←, D/→）
		if (Input::GetInstance()->TriggerKey(DIK_A) || Input::GetInstance()->TriggerKey(DIK_LEFT)) {
			selected_ = (selected_ - 1 + (int)entries_.size()) % (int)entries_.size();
			UpdateLayout_(); // ← アニメ開始
		}
		if (Input::GetInstance()->TriggerKey(DIK_D) || Input::GetInstance()->TriggerKey(DIK_RIGHT)) {
			selected_ = (selected_ + 1) % (int)entries_.size();
			UpdateLayout_(); // ← アニメ開始
		}

		// 決定（SPACE/Enter）
		if (Input::GetInstance()->TriggerKey(DIK_RETURN) || Input::GetInstance()->TriggerKey(DIK_SPACE)) {
			selectedStageCSV_ = entries_[selected_].csvPath;
			s_lastSelected_ = selected_;
			s_lastSelectedCSV_ = selectedStageCSV_;
			fade_->Start(Fade::Status::FadeOut, 0.75f);
			phase_ = Phase::kFadeOut;
		}

		StepLayoutAnimation_();

		// ステージの移動が反映されたあとでガイドを更新
		UpdateGuides_();

		break;

	case Phase::kFadeOut:
		fade_->Update();
		if (fade_->IsFinished()) {
			isFinished_ = true;
		}
		break;
	}
}

void StageSelectScene::Draw() {
	// 3Dモデル描画
	Model::PreDraw();

	// 天球
	skydome_->Draw(camera_);

	// ステージモデル群
	for (size_t i = 0; i < stageModels_.size(); ++i) {
		if (stageModels_[i] && stageWorlds_[i]) {
			stageModels_[i]->Draw(*stageWorlds_[i], camera_);
		}
	}

	// ガイド（アニメ中は非表示）
	if (!animating_) {
		if (AButtonModel_ && AButtonWorld_) {
			AButtonModel_->Draw(*AButtonWorld_, camera_);
		}
		if (DButtonModel_ && DButtonWorld_) {
			DButtonModel_->Draw(*DButtonWorld_, camera_);
		}
	}

	Model::PostDraw();

	// フェード
	fade_->Draw();
}

void StageSelectScene::UpdateLayout_(bool snap) {
	const size_t n = stageWorlds_.size();
	startX_.resize(n);
	targetX_.resize(n);
	startS_.resize(n);
	targetS_.resize(n);

	for (size_t i = 0; i < n; ++i) {
		auto& wt = *stageWorlds_[i];

		// 現在値をスタートとして記録
		startX_[i] = wt.translation_.x;
		startS_[i] = wt.scale_.x;

		// 目標値を計算（横一列）
		const float x = (static_cast<int>(i) - selected_) * stageSpacing_;
		const float s = (i == (size_t)selected_) ? scaleCenter_ : scaleSide_;
		targetX_[i] = x;
		targetS_[i] = s;
	}

	if (snap) {
		// 初期配置や即時反映したい時
		for (size_t i = 0; i < n; ++i) {
			auto& wt = *stageWorlds_[i];
			wt.translation_.x = targetX_[i];
			wt.scale_ = {targetS_[i], targetS_[i], targetS_[i]};
			UpdateAffineTransformMatrix_(wt);
		}
		animating_ = false;
		animT_ = 1.0f;
	} else {
		// アニメ開始
		animT_ = 0.0f;
		animating_ = true;
	}
}

void StageSelectScene::StepLayoutAnimation_() {
	if (!animating_)
		return;

	animT_ += animSpeed_;
	if (animT_ > 1.0f)
		animT_ = 1.0f;

	const float t = easeInOutSine(animT_); // 0→1 を滑らかに

	const size_t n = stageWorlds_.size();
	for (size_t i = 0; i < n; ++i) {
		auto& wt = *stageWorlds_[i];

		const float x = ::lerp(startX_[i], targetX_[i], t);
		const float s = ::lerp(startS_[i], targetS_[i], t);

		wt.translation_.x = x;
		wt.scale_ = {s, s, s};

		UpdateAffineTransformMatrix_(wt);
	}

	if (animT_ >= 1.0f)
		animating_ = false;
}

void StageSelectScene::UpdateAffineTransformMatrix_(WorldTransform& wt) {
	// スケール
	Matrix4x4 S = MakeScaleMatrix(wt.scale_);
	// 回転
	Matrix4x4 Rz = MakeRotateZMatrix(wt.rotation_.z);
	Matrix4x4 Ry = MakeRotateYMatrix(wt.rotation_.y);
	Matrix4x4 Rx = MakeRotateXMatrix(wt.rotation_.x);
	Matrix4x4 R = Rz * Ry * Rx;
	// 平行移動
	Matrix4x4 T = MakeTranslateMatrix(wt.translation_);
	// アフィン（Player と同じ：S * R * T）
	wt.matWorld_ = S * R * T;

	// GPU へ
	wt.TransferMatrix();
}

void StageSelectScene::UpdateGuides_() {
	if (!AButtonWorld_ || !DButtonWorld_) {
		return;
	}
	if (stageWorlds_.empty()) {
		return;
	}

	// 選択中モデルの現在位置（イージング途中でもOK。描画は非表示になる）
	const auto& selWT = *stageWorlds_[static_cast<size_t>(selected_)];

	// 左右に固定オフセット（ワールドX基準）
	AButtonWorld_->translation_ = {selWT.translation_.x - guideOffsetX_, selWT.translation_.y + guideOffsetY_, selWT.translation_.z};
	DButtonWorld_->translation_ = {selWT.translation_.x + guideOffsetX_, selWT.translation_.y + guideOffsetY_, selWT.translation_.z};

	// ガイドのスケール（固定）
	AButtonWorld_->scale_ = {guideScale_, guideScale_, guideScale_};
	DButtonWorld_->scale_ = {guideScale_, guideScale_, guideScale_};

	// 向きは正面のまま
	AButtonWorld_->rotation_ = {0.0f, 0.0f, 0.0f};
	DButtonWorld_->rotation_ = {0.0f, 0.0f, 0.0f};

	// Player と同じアフィン更新で行列→GPU転送
	UpdateAffineTransformMatrix_(*AButtonWorld_);
	UpdateAffineTransformMatrix_(*DButtonWorld_);
}
