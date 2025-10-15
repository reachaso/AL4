#include "TitleScene.h"
#include "struct.h"

TitleScene::~TitleScene() {

	delete titleModel_;
	delete title_;
	delete skydomeModel_;
	delete skydome_;
	delete debugCamera_;
	delete fade_;
	delete subTitle3D_;
	delete subTitleModel_;
}

void TitleScene::Initialize() {

	worldTransform_.Initialize();

	title_ = new Title();
	titleModel_ = Model::CreateFromOBJ("title", true);
	title_->SetModel(titleModel_);
	title_->Initialize(&camera_);

	subTitleModel_ = Model::CreateFromOBJ("guide", true);
	subTitle3D_ = new SubTitle3D();
	subTitle3D_->Initialize(subTitleModel_, &camera_);
	subTitle3D_->AttachTo(title_, {0.0f, -4.2f, 0.0f}); // タイトルの真下に配置
	subTitle3D_->SetScale({0.9f, 0.9f, 0.9f});          // 調整用
	subTitle3D_->SetBlinkHard(0.06f, 0.55f);            // カクカク点滅（確実）
	subTitle3D_->Update();

	// =============================
	// 天球の初期化
	// =============================

	skydome_ = new Skydome();
	skydomeModel_ = Model::CreateFromOBJ("skydome", true);
	skydome_->SetModel(skydomeModel_);
	skydome_->Initialize();

	// =============================
	// fadeの初期化
	// =============================

	fade_ = new Fade();
	fade_->Initialize();
	fade_->Start(Fade::Status::FadeIn, 1.0f);

	// =============================
	// デバッグカメラの初期化
	// =============================

	camera_.Initialize();
	camera_.translation_ = title_->GetWorldPosition();
	camera_.translation_.z = -30.0f;

	debugCamera_ = new DebugCamera(1280, 720);
}

void TitleScene::Update() {

#ifdef _DEBUG
	// デバッグカメラの切り替え
	if (Input::GetInstance()->TriggerKey(DIK_TAB)) {
		isDebugCameraActive_ = !isDebugCameraActive_;
	}

	// カメラの処理
	if (isDebugCameraActive_) {
		// デバッグカメラの更新
		debugCamera_->Update();

		camera_.translation_ = debugCamera_->GetCamera().translation_;
		camera_.rotation_ = debugCamera_->GetCamera().rotation_;
		camera_.matView = debugCamera_->GetCamera().matView;
		camera_.matProjection = debugCamera_->GetCamera().matProjection;
		// ビュープロジェクション行列の転送
		camera_.TransferMatrix();
	} else {
		camera_.UpdateMatrix();
		camera_.TransferMatrix();
	}

	// デバッグ情報の表示
	ImGui::Begin("TitleScene");

	ImGui::Text("カメラの位置");
	ImGui::Text("X: %f", camera_.translation_.x);
	ImGui::Text("Y: %f", camera_.translation_.y);
	ImGui::Text("Z: %f", camera_.translation_.z);

	ImGui::Text("デバッグカメラ: %s", isDebugCameraActive_ ? "有効" : "無効");

	ImGui::End();
#endif

	switch (phase_) {
	case TitleScene::Phase::kFadeIn:

		fade_->Update();

		if (subTitle3D_)
			subTitle3D_->SetVisible(false);

		// フェードインが完了したらメインフェーズに移行
		if (fade_->IsFinished()) {
			phase_ = TitleScene::Phase::kMain;
		}

		break;
	case TitleScene::Phase::kMain:

		// ============================
		// 天球の更新
		// ============================
		skydome_->Update();
		title_->Update();

		if (subTitle3D_) {
			subTitle3D_->SetVisible(true);
			subTitle3D_->Update();
		}

		// メインフェーズでSpaceキーが押されたらフェードアウトへ移行
		if (Input::GetInstance()->TriggerKey(DIK_SPACE)) {
			fade_->Start(Fade::Status::FadeOut, 2.0f);
			phase_ = TitleScene::Phase::kFadeOut;
		}

		break;
	case TitleScene::Phase::kFadeOut:

		if (subTitle3D_)
			subTitle3D_->SetVisible(false);
		fade_->Update();

		// フェードアウトが完了したらゲームシーンへ移行
		if (fade_->IsFinished()) {
			isFinished_ = true;
		}

		break;
	}
}

void TitleScene::Draw() {

	// 3Dモデルの描画
	Model::PreDraw();

	// 天球の描画
	skydome_->Draw(camera_);

	// タイトルの描画
	title_->Draw();
	if (subTitle3D_)
		subTitle3D_->Draw();

	Model::PostDraw();

	fade_->Draw();
}
