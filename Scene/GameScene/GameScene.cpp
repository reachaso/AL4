#include "GameScene.h"
#include "struct.h"
#include "CommonBGM/CommonBGM.h"

GameScene::~GameScene() {
	// スプライトの解放
	delete model_;
#ifdef _DEBUG
	delete debugCamera_;
	debugCamera_ = nullptr;
#endif
	delete skydome_;
	delete skydomeModel_;
	delete player_;
	delete playerModel_;
	delete mapChipField_;
	delete cameraController_;
	delete enemyModel_;
	model_ = nullptr;

	for (Enemy* enemy : enemies_) {
		delete enemy;
	}

	for (std::vector<WorldTransform*>& worldTransformLine : worldTransformBlocks_) {
		for (WorldTransform* worldTransformBlock : worldTransformLine) {
			delete worldTransformBlock;
		}
	}

	for (Coin* coin : coins_) {
		delete coin;
	}
	coins_.clear();
	delete coinModel_;
	coinModel_ = nullptr;

	for (Goal* goal : goals_) {
		delete goal;
	}
	goals_.clear();
	delete goalModel_;
	goalModel_ = nullptr;

	delete pauseFade_;
	pauseFade_ = nullptr;

	delete starModel_;
	starModel_ = nullptr;

	delete NoStarModel_;
	NoStarModel_ = nullptr;

	worldTransformBlocks_.clear();
}

void GameScene::Initialize() {

	// ============================
	// サウンド用
	// ============================

	// サウンドデータの読み込み
	soundDataHandle_ = Audio::GetInstance()->LoadWave("fanfare.wav");

	// 音楽再生
	// Audio::GetInstance()->PlayWave(soundDataHandle_);

	// 音声再生
	// voiceHandle_ = Audio::GetInstance()->PlayWave(soundDataHandle_, true);

	// =============================
	// マップチップフィールドの初期化
	// =============================

	mapChipField_ = new MapChipField();
	// マップ読み込み後
	mapChipField_->LoadMapChipCSV(stageCSVPath_);
	mapChipField_->GenerateBlocks();

	// ▼ Player
	MapChipField::IndexSet pIndex = {6, 12}; // フォールバック（CSVにpが無い場合）
	if (auto p = mapChipField_->GetPlayerSpawnIndex()) {
		pIndex = *p;
	}
	Vector3 playerPosition = mapChipField_->GetMapChipPositionByIndex(pIndex.xIndex, pIndex.yIndex);

	// =============================
	// 天球の初期化
	// =============================

	skydome_ = new Skydome();
	skydomeModel_ = Model::CreateFromOBJ("skydome", true);
	skydome_->SetModel(skydomeModel_);
	skydome_->Initialize();

	// ============================
	// ブロックの初期化
	// ============================

	// テクスチャの読み込み
	textureHandle_ = TextureManager::Load("stone_bricks.png");

	model_ = Model::Create();

	// ワールドトランスフォームの初期化
	worldTransform_.Initialize();

	// ==============================
	// ゴールの初期化
	// ==============================
	goalModel_ = Model::CreateFromOBJ("goal", true);

	// ▼ Goals（CSVの g をすべて生成）
	for (const auto& idx : mapChipField_->GetGoalSpawnIndices()) {
		Vector3 pos = mapChipField_->GetMapChipPositionByIndex(idx.xIndex, idx.yIndex);
		Goal* g = new Goal();
		g->Initialize(&camera_, pos);
		g->SetModel(goalModel_);
		g->SetDeltaTime(deltaTime_);
		goals_.push_back(g);
	}

	// =============================
	// 星の初期化
	// =============================

	starModel_ = Model::CreateFromOBJ("star", true);
	NoStarModel_ = Model::CreateFromOBJ("nostar", true);

	// =============================
	// カメラの初期化
	// =============================
	camera_.Initialize();

#ifdef _DEBUG
	debugCamera_ = new DebugCamera(1280, 720);
#endif

	// ==============================
	// プレイヤーの初期化
	// ==============================

	player_ = new Player();
	playerModel_ = Model::CreateFromOBJ("player", true);
	player_->SetModel(playerModel_);
	player_->Initialize(&camera_, playerPosition);
	player_->SetMapChipField(mapChipField_);
	player_->SetDeltaTime(deltaTime_);

	// ==============================
	// パーティクルの初期化
	// ==============================

	deathParticles_ = new DeathParticles();
	deathParticles_->Initialize(&camera_, playerPosition);
	deathParticles_->SetDeltaTime(deltaTime_);
	deathParticlesModel_ = Model::CreateFromOBJ("deathparticles", true);
	deathParticles_->SetModel(deathParticlesModel_);

	// ==============================
	// 敵の初期化
	// ==============================

	enemyModel_ = Model::CreateFromOBJ("enemy", true);

	// ▼ Enemies（CSVの e をすべて生成）
	for (const auto& idx : mapChipField_->GetEnemySpawnIndices()) {
		Vector3 pos = mapChipField_->GetMapChipPositionByIndex(idx.xIndex, idx.yIndex);
		Enemy* e = new Enemy();
		e->Initialize(&camera_, pos);
		e->SetModel(enemyModel_);
		e->SetDeltaTime(deltaTime_);
		e->SetMapChipField(mapChipField_);
		e->SetFreefall(false); // 落下しないように設定
		enemies_.push_back(e);
	}

	// ==============================
	// コインの初期化
	// ==============================
	coinModel_ = Model::CreateFromOBJ("coin", true);
	for (const auto& idx : mapChipField_->GetCoinSpawnIndices()) {
		Vector3 pos = mapChipField_->GetMapChipPositionByIndex(idx.xIndex, idx.yIndex);
		Coin* c = new Coin();
		c->Initialize(&camera_, pos);
		c->SetModel(coinModel_);
		c->SetDeltaTime(deltaTime_);
		coins_.push_back(c);
		totalCoins_ = static_cast<int>(mapChipField_->GetCoinSpawnIndices().size());
	}

	// ============================
	// カメラコントローラーの初期化
	// ============================
	cameraController_ = new CameraController();
	cameraController_->SetCamera(&camera_);
	cameraController_->Initialize();
	cameraController_->SetTarget(player_);
	cameraController_->Reset();

	// カメラの視野サイズを計算

	float cameraZ = std::abs(cameraController_->GetTargetOffset().z); // カメ
	float cameraHeight = 2.0f * cameraZ * tanf(camera_.fovAngleY * 0.5f);
	float cameraWidth = cameraHeight * camera_.aspectRatio;

	// マップの端座標
	float mapLeft = 0.0f;
	float mapRight = MapChipField::GetBlockWidth() * MapChipField::GetNumBlockHorizontal();
	float mapBottom = 0.0f;
	float mapTop = MapChipField::GetBlockHeight() * MapChipField::GetNumBlockVirtical();

	// movableAreaをカメラの視野分だけ内側にオフセット
	Rect movableArea;
	movableArea.left = mapLeft + cameraWidth / 2.0f;
	movableArea.right = mapRight - cameraWidth / 2.0f;
	movableArea.bottom = mapBottom + cameraHeight / 2.0f;
	movableArea.top = mapTop - cameraHeight / 2.0f;

	cameraController_->SetMovableArea(movableArea);
	cameraController_->Reset();

	// ▼ プレイ範囲（カメラの可動域より一回り広い“実マップ範囲”）
	playArea_.left = mapLeft;
	playArea_.right = mapRight;
	playArea_.bottom = mapBottom;
	playArea_.top = mapTop;

	cameraController_->SetMovableArea(movableArea);

	// ============================
	// ゲームシーンフェーズ
	// ============================

	// pause用フェードを作成
	pauseFade_ = new Fade();
	pauseFade_->Initialize();

	phase_ = Phase::kFadeIn;

	fade_ = new Fade();
	fade_->Initialize();
	fade_->Start(Fade::Status::FadeIn, 2.0f);


	masterVolume_ = CommonBGM::GetInstance()->GetVolume();
}

void GameScene::Update() {

	switch (phase_) {

	case Phase::kFadeIn:
		UpdateFadeInPhase();
		// フェードインの更新
		fade_->Update();
		if (fade_->IsFinished()) {
			// フェードインが完了したらプレイフェーズに移行
			phase_ = Phase::kPlay;
		}
		break;

	case Phase::kPlay:

		UpdatePlayPhase();

		// --- ポーズ中は画面を薄くしてメニューを描く ---
		if (isPaused_) {
			// 中央のポーズメニュー
			DrawPauseOverlay();
		}

		break;

	case Phase::kDeath:

		UpdateDeathPhase();

		break;

	case Phase::kResult:
		UpdateResultPhase();
		DrawResultOverlay();
		break;

	case Phase::kFadeOut:

		// フェードアウトの更新
		fade_->Update();
		if (fade_->IsFinished()) {
			// フェードアウトが完了したらゲームシーンを終了
			isFinished_ = true;
		}
	}
}

// =================================
// ゲームシーンの描画処理
// =================================

void GameScene::Draw() {

	// 3Dモデルの描画
	Model::PreDraw();

	switch (phase_) {

	case Phase::kFadeIn:

		DrawPlayPhase();
		// フェードインの描画
		fade_->Draw();
		break;

	case Phase::kPlay:
		DrawPlayPhase();

		// --- ポーズ中は画面を薄くしてメニューを描く ---
		if (isPaused_) {
			// 画面を薄く（pauseFade_を使う）
			pauseFade_->Draw();
		}
		break;

	case Phase::kDeath:
		DrawDeathPhase();
		break;

	case Phase::kResult:
		DrawPlayPhase();
		fade_->Draw();

		break;

	case Phase::kFadeOut:

		if (isCleared_) {
			DrawPlayPhase(); // ← クリア時はプレイ画面のままフェード
		} else {
			DrawDeathPhase(); // ← デス時は従来通り
		}
		// フェードアウトの描画
		fade_->Draw();
		break;
	}

	Model::PostDraw();
}

// =================================
// プレイヤーがプレイ可能エリア外にいるかどうかを判定する関数
// =================================

bool GameScene::IsOutOfPlayableArea(const Vector3& p) const {
	// 1ブロック分の余白を持って奈落判定（落下の猶予）
	const float margin = MapChipField::GetBlockHeight();

	const bool belowAbyss = (p.y < (playArea_.bottom - margin));
	const bool outLeft = (p.x < (playArea_.left - margin));
	const bool outRight = (p.x > (playArea_.right + margin));

	return belowAbyss || outLeft || outRight;
}

// =================================
// 全ての当たり判定をチェックする関数
// =================================

void GameScene::CheckAllCollisions() {

#pragma region プレイヤーと敵キャラの当たり判定

	for (Enemy* enemy : enemies_) {
		if (enemy && !enemy->IsDead()) {
			CheckPlayerEnemyCollisions(player_, enemy);
		}
	}

#pragma endregion

#pragma region プレイヤーとアイテムの当たり判定

	// プレイヤーとコインの当たり判定
	for (Coin* coin : coins_) {
		if (coin && !coin->IsCollected()) {
			CheckPlayerCoinCollisions(player_, coin);
		}
	}

#pragma endregion

#pragma region プレイヤーの弾とブロックの当たり判定

#pragma endregion

#pragma region 敵同士の当たり判定
	// Enemy-Enemy collisions (pairwise)
	for (auto it1 = enemies_.begin(); it1 != enemies_.end(); ++it1) {
		auto it2 = it1;
		++it2;
		for (; it2 != enemies_.end(); ++it2) {
			Enemy* a = *it1;
			Enemy* b = *it2;
			if (a && b) {
				a->OnEnemyCollision(b);
			}
		}
	}

#pragma endregion

#pragma region プレイヤーとゴールの当たり判定
	for (Goal* goal : goals_) {
		if (goal && !goal->IsReached()) {
			CheckPlayerGoalCollisions(player_, goal);
		}
	}
#pragma endregion
}

// =================================
// プレイヤーと敵キャラの当たり判定をチェックする関数
// =================================

void GameScene::CheckPlayerEnemyCollisions(Player* player, Enemy* enemy) {

	AABB aabbPlayer = player_->GetAABB();
	AABB aabbEnemy = enemy->GetAABB();

	if (!AABB::CheckCollision(aabbPlayer, aabbEnemy)) {
		return;
	}
	if (enemy->IsDead()) {
		return;
	}

	// 重なり量を計算（Enemy::OnEnemyCollision と同じ考え方）
	const float overlapX = (std::min)(aabbPlayer.max.x - aabbEnemy.min.x, aabbEnemy.max.x - aabbPlayer.min.x);
	const float overlapY = (std::min)(aabbPlayer.max.y - aabbEnemy.min.y, aabbEnemy.max.y - aabbPlayer.min.y);
	const float overlapZ = (std::min)(aabbPlayer.max.z - aabbEnemy.min.z, aabbEnemy.max.z - aabbPlayer.min.z);

	const bool verticalPriority = (overlapY <= overlapX && overlapY <= overlapZ);
	const bool playerAbove = (player->GetWorldPosition().y >= enemy->GetWorldPosition().y + 0.1f);
	const bool fallingFast = player->IsHipDropping() || (player->GetVelocity().y < -1.0f);

	if (verticalPriority && playerAbove && fallingFast) {
		// ★踏みつけ撃破
		enemy->Kill();
		player->BounceFromStomp();
		// ★必要ならここでSEやエフェクトを再生
		// Audio::GetInstance()->PlayWave(...);
		return;
	}

	// ★踏めていない：従来どおり双方の処理（＝プレイヤー死亡）
	player->OnEnemyCollision(enemy);
	enemy->OnPlayerCollision(player);
}

// =================================
// ゲームシーンのフェーズを変更する関数
// =================================

void GameScene::ChangePhase() {

	switch (phase_) {

	case Phase::kFadeIn:

		break;

	case Phase::kPlay:
		//CommonBGM::GetInstance()->Stop();
		phase_ = Phase::kDeath;
		break;

	case Phase::kDeath:

		phase_ = Phase::kFadeOut;

		break;

	case Phase::kFadeOut:

		break;
	}
}

void GameScene::UpdateFadeInPhase() {
	skydome_->Update();

	bool usedDebugCam = false;
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
		usedDebugCam = true;
	}
#endif
	if (!usedDebugCam) {
		cameraController_->Update();
		camera_.UpdateMatrix();
		camera_.TransferMatrix();
	}

	for (auto& worldTransformLine : mapChipField_->worldTransformBlocks_) {
		for (auto* worldTransformBlock : worldTransformLine) {
			if (!worldTransformBlock)
				continue;
			using namespace KamataEngine::MathUtility;
			Matrix4x4 scaleMat = MakeScaleMatrix(worldTransformBlock->scale_);
			Matrix4x4 rotZMat = MakeRotateZMatrix(worldTransformBlock->rotation_.z);
			Matrix4x4 rotYMat = MakeRotateYMatrix(worldTransformBlock->rotation_.y);
			Matrix4x4 rotXMat = MakeRotateXMatrix(worldTransformBlock->rotation_.x);
			Matrix4x4 rotMat = rotZMat * rotYMat * rotXMat;
			Matrix4x4 transMat = MakeTranslateMatrix(worldTransformBlock->translation_);
			worldTransformBlock->matWorld_ = scaleMat * rotMat * transMat;
			worldTransformBlock->TransferMatrix();
		}
	}

	player_->SetDeltaTime(0.0f);
	player_->Update();
	player_->SetDeltaTime(deltaTime_);

	for (Enemy* enemy : enemies_) {
		if (!enemy)
			continue;
		enemy->SetDeltaTime(0.0f);
		enemy->Update();
		enemy->SetDeltaTime(deltaTime_);
	}

	for (Coin* coin : coins_) {
		if (!coin)
			continue;
		coin->SetDeltaTime(0.0f);
		coin->Update();
		coin->SetDeltaTime(deltaTime_);
	}

	for (Goal* goal : goals_) {
		if (!goal)
			continue;
		goal->SetDeltaTime(0.0f);
		goal->Update();
		goal->SetDeltaTime(deltaTime_);
	}
}

// =================================
// ゲームシーンのプレイフェーズの更新処理
// =================================

void GameScene::UpdatePlayPhase() {

	  // ===== ポーズON/OFF（ESCトグル） =====
	if (Input::GetInstance()->TriggerKey(DIK_ESCAPE)) {
		if (!isPaused_) {
			// ポーズ開始：薄くしてメニュー表示へ
			isPaused_ = true;
			pauseHoldReached_ = false;
			pauseFade_->Start(Fade::Status::FadeOut, 0.20f); // 0→1へ上げる（黒く/薄く）
		} else {
			// ポーズ解除：薄さを戻す
			isPaused_ = false;
			pauseHoldReached_ = false;
			pauseFade_->Start(Fade::Status::FadeIn, 0.20f); // 1→0へ下げる
		}
	}

	// 「ステージセレクトへ」ボタンが押されたらここで即遷移開始
	if (leaveToStageSelectRequested_) {
		isPaused_ = false; // ポーズ解除
		pauseHoldReached_ = false;
		// 既存のシーン用フェードで遷移
		fade_->Start(Fade::Status::FadeOut, 0.8f);
		phase_ = Phase::kFadeOut;
		leaveToStageSelectRequested_ = false;
		return; // このフレームはここで終わり
	}

	// ポーズ中はゲームロジックを止める（ImGuiと薄暗フェードだけ動かす）
	if (isPaused_) {
		UpdatePauseOverlay();
		// カメラやプレイヤー・敵・コイン・当たり判定などは更新しない
		// スカイドーム等の見た目更新も止めたい場合はこのreturnをより上に
		return;
	}

	// ============================
	// 天球の更新
	// ============================

	skydome_->Update();

	// ============================
	// プレイヤーの更新
	// ============================

	player_->Update();

	// ============================
	// パーティクルの更新
	// ============================

	// ============================
	// 敵の更新
	// ============================

	// --- 敵の更新（死亡スキップ） ---
	for (Enemy* enemy : enemies_) {
		if (enemy && !enemy->IsDead()) {
			enemy->Update();
		}
	}

	// --- 死亡した敵の破棄 ---
	enemies_.remove_if([&](Enemy* e) {
		if (!e) {
			return true;
		}
		if (e->IsDead()) {
			delete e;
			return true;
		}
		return false;
	});

	// ============================
	// コインの更新
	// ============================

	for (Coin* coin : coins_) {
		if (coin) {
			coin->Update();
		}
	}

	// ============================
	// ゴールの更新
	// ============================

	for (Goal* goal : goals_) {
		if (goal) {
			goal->Update();
		}
	}

	// ============================
	// ブロックの更新
	// ============================

	for (auto& worldTransformLine : mapChipField_->worldTransformBlocks_) {
		for (auto* worldTransformBlock : worldTransformLine) {
			if (!worldTransformBlock)
				continue;

			using namespace KamataEngine::MathUtility;
			Matrix4x4 scaleMat = MakeScaleMatrix(worldTransformBlock->scale_);
			Matrix4x4 rotZMat = MakeRotateZMatrix(worldTransformBlock->rotation_.z);
			Matrix4x4 rotYMat = MakeRotateYMatrix(worldTransformBlock->rotation_.y);
			Matrix4x4 rotXMat = MakeRotateXMatrix(worldTransformBlock->rotation_.x);
			Matrix4x4 rotMat = rotZMat * rotYMat * rotXMat;
			Matrix4x4 transMat = MakeTranslateMatrix(worldTransformBlock->translation_);
			worldTransformBlock->matWorld_ = scaleMat * rotMat * transMat;
			worldTransformBlock->TransferMatrix();
		}
	}

#ifdef _DEBUG

	if (Input::GetInstance()->TriggerKey(DIK_TAB)) {
		// デバッグウィンドウの表示切り替え
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
	}

	ImGui::Begin("Debug");

	if (ImGui::CollapsingHeader("カメラ", ImGuiTreeNodeFlags_DefaultOpen)) {

		if (isDebugCameraActive_) {

			ImGui::Text("デバックカメラ : ");
			ImGui::SameLine();
			ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "有効");
			ImGui::Text("TABキーで切り替え");

			auto cam = const_cast<Camera*>(&debugCamera_->GetCamera());
			Vector3 translation = cam->translation_;
			Vector3 rotation = cam->rotation_;

			if (ImGui::Button("カメラ位置リセット")) {
				cam->translation_ = {0.0f, 0.0f, -50.0f}; // 初期位置にリセット
				cam->rotation_ = {0.0f, 0.0f, 0.0f};      // 初期回転にリセット
			}

			if (ImGui::Button("反対方向に移動")) {
				cam->translation_.z = 50.0f; // 反対方向に移動
				cam->rotation_.y = 135.0f;   // 初期回転にリセット
			}

		} else {
			ImGui::Text("デバックカメラ : ");
			ImGui::SameLine();
			ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "無効");
			ImGui::Text("TABキーで切り替え");
		}
	}

	if (ImGui::CollapsingHeader("プレイヤーの位置", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::Text("X: %.2f", player_->GetPosition().x);
		ImGui::Text("Y: %.2f", player_->GetPosition().y);
		ImGui::Text("Z: %.2f", player_->GetPosition().z);
	}

	if (ImGui::CollapsingHeader("カメラの位置", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::Text("X: %.2f", camera_.translation_.x);
		ImGui::Text("Y: %.2f", camera_.translation_.y);
		ImGui::Text("Z: %.2f", camera_.translation_.z);
	}

	ImGui::End();

#endif // _DEBUG

	if (!isDebugCameraActive_) {
		cameraController_->Update();
		camera_.UpdateMatrix();
		camera_.TransferMatrix();
	}

	camera_.UpdateMatrix();

	// 当たり判定のチェック
	CheckAllCollisions();

	// ▼▼ 奈落チェック：プレイ範囲外に落ちたら即デスフェーズへ ▼▼
	{
		const Vector3& p = player_->GetWorldPosition(); // 既存で使用している取得関数
		if (IsOutOfPlayableArea(p)) {
			// パーティクルの開始位置をプレイヤー位置に設定
			deathParticles_->SetPosition(p);

			// デス演出フェーズへ（既存のフェーズ遷移仕様に合わせる）
			ChangePhase(); // kPlay -> kDeath
			return;        // このフレームの残処理を打ち切り
		}
	}

	if (player_->GetIsDead()) {

		const Vector3& playerPosition = player_->GetWorldPosition();

		// パーティクルの位置をプレイヤーの位置に設定
		deathParticles_->SetPosition(playerPosition);

		// プレイヤーが死亡した場合、フェーズを変更
		ChangePhase();
	}
}

void GameScene::DrawPlayPhase() {

	// 天球の描画
	skydome_->Draw(camera_);

	// プレイヤーの描画
	player_->Draw();

	// 敵の描画
	for (Enemy* enemy : enemies_) {
		if (enemy) {
			enemy->Draw();
		}
	}

	for (Coin* coin : coins_) {
		if (coin) {
			coin->Draw();
		}
	}

	for (Goal* goal : goals_) {
		if (goal) {
			goal->Draw();
		}
	}

	// ブロックの描画
	for (auto& worldTransformLine : mapChipField_->worldTransformBlocks_) {
		for (auto* worldTransformBlock : worldTransformLine) {
			if (!worldTransformBlock)
				continue;
			model_->Draw(*worldTransformBlock, camera_, textureHandle_);
		}
	}
}

void GameScene::UpdateDeathPhase() {

	// ============================
	// 天球の更新
	// ============================

	skydome_->Update();

	// ============================
	// 敵の更新
	// ============================

	for (Enemy* enemy : enemies_) {
		if (enemy) {
			enemy->Update();
		}
	}

	// ============================
	// ブロックの更新
	// ============================

	for (auto& worldTransformLine : mapChipField_->worldTransformBlocks_) {
		for (auto* worldTransformBlock : worldTransformLine) {
			if (!worldTransformBlock)
				continue;

			using namespace KamataEngine::MathUtility;
			Matrix4x4 scaleMat = MakeScaleMatrix(worldTransformBlock->scale_);
			Matrix4x4 rotZMat = MakeRotateZMatrix(worldTransformBlock->rotation_.z);
			Matrix4x4 rotYMat = MakeRotateYMatrix(worldTransformBlock->rotation_.y);
			Matrix4x4 rotXMat = MakeRotateXMatrix(worldTransformBlock->rotation_.x);
			Matrix4x4 rotMat = rotZMat * rotYMat * rotXMat;
			Matrix4x4 transMat = MakeTranslateMatrix(worldTransformBlock->translation_);
			worldTransformBlock->matWorld_ = scaleMat * rotMat * transMat;
			worldTransformBlock->TransferMatrix();
		}
	}

#ifdef _DEBUG

	if (Input::GetInstance()->TriggerKey(DIK_TAB)) {
		// デバッグウィンドウの表示切り替え
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
		// ビュープロジェクション行列の更新と転送
		camera_.UpdateMatrix();
	}

	ImGui::Begin("Debug");

	if (ImGui::CollapsingHeader("カメラ", ImGuiTreeNodeFlags_DefaultOpen)) {

		if (isDebugCameraActive_) {

			ImGui::Text("デバックカメラ : ");
			ImGui::SameLine();
			ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "有効");
			ImGui::Text("TABキーで切り替え");

			auto cam = const_cast<Camera*>(&debugCamera_->GetCamera());
			Vector3 translation = cam->translation_;
			Vector3 rotation = cam->rotation_;

			if (ImGui::Button("カメラ位置リセット")) {
				cam->translation_ = {0.0f, 0.0f, -50.0f}; // 初期位置にリセット
				cam->rotation_ = {0.0f, 0.0f, 0.0f};      // 初期回転にリセット
			}

			if (ImGui::Button("反対方向に移動")) {
				cam->translation_.z = 50.0f; // 反対方向に移動
				cam->rotation_.y = 135.0f;   // 初期回転にリセット
			}

		} else {
			ImGui::Text("デバックカメラ : ");
			ImGui::SameLine();
			ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "無効");
			ImGui::Text("TABキーで切り替え");
		}
	}

	if (ImGui::CollapsingHeader("カメラの位置", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::Text("X: %.2f", camera_.translation_.x);
		ImGui::Text("Y: %.2f", camera_.translation_.y);
		ImGui::Text("Z: %.2f", camera_.translation_.z);
	}

	ImGui::End();

#endif // _DEBUG

	camera_.UpdateMatrix();

	if (deathParticles_ && deathParticles_->GetIsFinished()) {
		// パーティクルが終了したらフェーズを変更
		fade_->Start(Fade::Status::FadeOut, 2.0f);
		ChangePhase();
	} else {
		// パーティクルが終了していない場合は更新
		deathParticles_->Update();
	}
}

void GameScene::DrawDeathPhase() {

	// 天球の描画
	skydome_->Draw(camera_);

	// パーティクルの描画
	deathParticles_->Draw();

	// 敵の描画
	for (Enemy* enemy : enemies_) {
		if (enemy && !enemy->IsDead()) {
			enemy->Draw();
		}
	}

	// ブロックの描画
	for (auto& worldTransformLine : mapChipField_->worldTransformBlocks_) {
		for (auto* worldTransformBlock : worldTransformLine) {
			if (!worldTransformBlock)
				continue;
			model_->Draw(*worldTransformBlock, camera_, textureHandle_);
		}
	}

}

void GameScene::UpdateResultPhase() {

	// 初回だけ安全に集計
	if (!resultComputed_) {
		ComputeResult();
	}

	// 目標の薄さ（半分）※好みで 0.6f とかでもOK
	const float kHold = 0.5f;

	if (!resultWaitingInput_) {
		// フェードを進める
		fade_->Update();

		// 目標まで来たらそこで固定して入力待ちへ
		if (fade_->GetNormalized() >= kHold) {
			fade_->SetNormalized(kHold); // ← 0.5で止める
			resultWaitingInput_ = true;
		}
	} else {
		// 入力受付（ここではフェードを進めない＝0.5の薄さで静止）
		if (Input::GetInstance()->TriggerKey(DIK_SPACE)) {
			// そのままフェーズだけ kFadeOut に遷移（Startし直さない！）
			// statusはFadeOutのまま/counter=0.5なので、続きの0.5→1.0が走る
			phase_ = Phase::kFadeOut;
		}
	}
}

void GameScene::ComputeResult() {
	// 総コイン数はCSV由来のスポーン数を採用（描画漏れ等の影響を受けない）
	totalCoins_ = static_cast<int>(mapChipField_->GetCoinSpawnIndices().size());

	int collected = 0;
	for (Coin* c : coins_) {
		if (c && c->IsCollected()) {
			++collected;
		}
	}
	collectedCoins_ = collected;

	// ☆は「全体の1/3ごとに1個（最大3）」を切り上げ単位で計算
	// 例：総数10 → 区切り=ceil(10/3)=4 → 0~3, 4~7, 8~10 の3段階
	int segment = (totalCoins_ <= 0) ? 1 : ((totalCoins_ + 2) / 3); // ceilDiv
	int stars = (totalCoins_ <= 0) ? 3 : (collectedCoins_ / segment);
	if (stars < 0)
		stars = 0;
	if (stars > 3)
		stars = 3;
	resultStars_ = stars;

	resultComputed_ = true;
}

void GameScene::DrawResultOverlay() {

#ifdef _DEBUG

	// 中央・半透明パネル
	ImGui::SetNextWindowSize(ImVec2(420, 220), ImGuiCond_Always);
	ImGui::SetNextWindowPos(ImVec2(1280 * 0.5f, 720 * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 16.0f);
	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0.72f));

	if (ImGui::Begin("##Result", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings)) {
		ImGui::SetWindowFontScale(1.15f);
		ImGui::TextColored(ImVec4(1, 1, 1, 1), "RESULT");
		ImGui::SetWindowFontScale(1.0f);
		ImGui::Separator();

		ImGui::Spacing();
		ImGui::Text("Coins  :  %d / %d", collectedCoins_, totalCoins_);

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();
		if (resultWaitingInput_) {
			ImGui::TextDisabled("Press SPACE to return to Stage Select");
		} else {
			ImGui::TextDisabled("...fading...");
		}
	}
	ImGui::End();
	ImGui::PopStyleColor();
	ImGui::PopStyleVar();

#endif // _DEBUG
}

void GameScene::CheckPlayerCoinCollisions(Player* player, Coin* coin) {
	AABB aabbPlayer = player_->GetAABB();
	AABB aabbCoin = coin->GetAABB();

	if (AABB::CheckCollision(aabbPlayer, aabbCoin)) {
		coin->OnPlayerCollision(player);
		// 必要ならスコア加算や UI 更新などをここに
	}
}

void GameScene::CheckPlayerGoalCollisions(Player* player, Goal* goal) {
	AABB aabbPlayer = player_->GetAABB();
	AABB aabbGoal = goal->GetAABB();
	if (AABB::CheckCollision(aabbPlayer, aabbGoal)) {
		goal->OnPlayerCollision(player);
		if (!isCleared_) {
			isCleared_ = true;

			//CommonBGM::GetInstance()->Stop();

			//Audio::GetInstance()->PlayWave(soundDataHandle_);
			ComputeResult();
			fade_->Start(Fade::Status::FadeOut, 0.8f);
			phase_ = Phase::kResult;
			resultWaitingInput_ = false;
		}
	}
}

int GameScene::CountCollectedCoins() const {
	int cnt = 0;
	for (Coin* c : coins_) {
		if (c && c->IsCollected()) {
			++cnt;
		}
	}
	return cnt;
}

void GameScene::UpdatePauseOverlay() {
	// 目標の薄さでホールド
	const float kHold = pauseHoldAlpha_; // 例: 0.5
	if (!pauseHoldReached_) {
		pauseFade_->Update();
		if (pauseFade_->GetNormalized() >= kHold) {
			pauseFade_->SetNormalized(kHold);
			pauseHoldReached_ = true;
		}
	}
}


void GameScene::DrawPauseOverlay() {

	#ifdef _DEBUG

	// 半透明の中央ウィンドウ
	ImGui::SetNextWindowSize(ImVec2(520, 420), ImGuiCond_Always);
	ImGui::SetNextWindowPos(ImVec2(1280 * 0.5f, 720 * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 16.0f);
	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0.72f));

	if (ImGui::Begin("##PauseMenu", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings)) {
		ImGui::SetWindowFontScale(1.15f);
		ImGui::TextColored(ImVec4(1, 1, 1, 1), "PAUSE");
		ImGui::SetWindowFontScale(1.0f);
		ImGui::Separator();

		// 現在のコイン状況
		ImGui::Spacing();
		const int collectedNow = CountCollectedCoins();
		ImGui::Text("Coins :  %d / %d", collectedNow, totalCoins_);

		// 操作説明（必要に応じて書き換えてください）
		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();
		ImGui::TextDisabled("Controls");
		ImGui::BulletText("Move : A / D  (or  \xe2\x86\x90 / \xe2\x86\x92 )");
		ImGui::BulletText("Jump : SPACE");
		ImGui::BulletText("Pause: ESC");

		// 音量調整（Audio のAPI名は環境に合わせて変更）
		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();
		ImGui::TextDisabled("Audio");

		float v = masterVolume_;
		if (ImGui::SliderFloat("BGM Volume", &v, 0.0f, 1.0f, "%.2f")) {
			masterVolume_ = v;
			CommonBGM::GetInstance()->SetVolume(masterVolume_);
		}


		// ボタン群
		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		if (ImGui::Button("ステージセレクトに戻る", ImVec2(-FLT_MIN, 0))) {
			leaveToStageSelectRequested_ = true;
		}

		if (ImGui::Button("ゲームに戻る  (ESC)", ImVec2(-FLT_MIN, 0))) {
			// ポーズ解除
			isPaused_ = false;
			pauseHoldReached_ = false;
			pauseFade_->Start(Fade::Status::FadeIn, 0.20f);
		}
	}
	ImGui::End();
	ImGui::PopStyleColor();
	ImGui::PopStyleVar();
#endif // _DEBUG
}
