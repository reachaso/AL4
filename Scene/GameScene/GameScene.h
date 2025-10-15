#pragma once
#include "CameraController/CameraController.h"
#include "DeathParticles/DeathParticles.h"
#include "Enemy/Enemy.h"
#include "MapChipField/MapChipField.h"
#include "Player/player.h"
#include "Skydome/Skydome.h"
#include "fade/fade.h"
#include "Coin/Coin.h"
#include "Goal/Goal.h"
#include "struct.h"
#include <KamataEngine.h>
#include <vector>
#include <list>

using namespace KamataEngine;

class GameScene {

public:
	enum class Phase {
		kFadeIn,
		kPlay,
		kDeath,
		kResult,
		kFadeOut,
	};

	// デストラクタ
	~GameScene();
	// 初期化
	void Initialize();
	// 更新
	void Update();
	// 描画
	void Draw();

	bool GetIsFinished() const { return isFinished_; } // シーン終了フラグの取得

	void CheckAllCollisions();

	/// <summary>
	/// プレイヤーと敵キャラクターの衝突をチェックします。
	/// </summary>
	void CheckPlayerEnemyCollisions(Player* player, Enemy* enemy);

	void CheckPlayerCoinCollisions(Player* player, Coin* coin);

	void CheckPlayerGoalCollisions(Player* player, Goal* goal);

	void SetStageCSV(const std::string& path) { stageCSVPath_ = path; }

private:
	bool isFinished_ = false; // シーン終了フラグ

	float deltaTime_ = 1.0f / 60.0f; // フレームレートを60FPSに設定

	int totalCoins_ = 0;
	int collectedCoins_ = 0;
	int resultStars_ = 0;
	bool resultComputed_ = false;
	bool resultWaitingInput_ = false;

	 // ▼ プレイ可能範囲（マップ全体の矩形）
	Rect playArea_{};

	// ▼ 位置がプレイ範囲外（奈落含む）か？
	bool IsOutOfPlayableArea(const Vector3& p) const;

	std::string stageCSVPath_ = "Resources/csv/stage1.csv";

	//========================
	// シーンフェーズ
	//========================

	Phase phase_;

	void ChangePhase();

	void UpdateFadeInPhase();

	void UpdatePlayPhase();
	void DrawPlayPhase();

	void UpdateDeathPhase();
	void DrawDeathPhase();

	void UpdateResultPhase();
	void DrawResultOverlay();
	void ComputeResult();

	// =======================
	// fade
	// =======================

	Fade* fade_ = nullptr;

	// =======================
	// 音声
	// =======================
	// サウンドデータハンドル
	uint32_t soundDataHandle_ = 0;

	// 音声再生用ハンドル
	uint32_t voiceHandle_ = 0;

	// =======================
	// マップチップフィールド
	// =======================

	MapChipField* mapChipField_ = nullptr;

	// =======================
	// 天球
	// =======================
	Skydome* skydome_ = nullptr;
	KamataEngine::Model* skydomeModel_ = nullptr;

	// =======================
	// プレイヤー
	// =======================
	Player* player_ = nullptr;
	KamataEngine::Model* playerModel_ = nullptr;

	// =======================
	// パーティクル
	// =======================

	DeathParticles* deathParticles_ = nullptr;
	KamataEngine::Model* deathParticlesModel_ = nullptr;

	// =======================
	// 敵キャラクター
	// =======================

	std::list<Enemy*> enemies_;
	KamataEngine::Model* enemyModel_ = nullptr;

	// =======================
	// ブロック
	// =======================
	KamataEngine::Model* model_ = nullptr;
	// テクスチャハンドル
	uint32_t textureHandle_ = 0;
	// 可変個配列
	std::vector<std::vector<KamataEngine::WorldTransform*>> worldTransformBlocks_;

	// =======================
	// コイン
	// =======================
	std::list<Coin*> coins_;
	KamataEngine::Model* coinModel_ = nullptr;

	// ========================
	// ゴール
	// ========================

	std::list<Goal*> goals_;
	KamataEngine::Model* goalModel_ = nullptr;

	bool isCleared_ = false; // クリアフラグ

	// ========================
	// 星
	// ========================

	Model* starModel_ = nullptr;
	Model* NoStarModel_ = nullptr;

	// =======================
	// カメラ
	// =======================

	KamataEngine::Camera camera_;

	KamataEngine::DebugCamera* debugCamera_ = nullptr;
	bool isDebugCameraActive_ = false;

	CameraController* cameraController_ = nullptr;

	float inputFloat3[3] = {0.0f, 0.0f, 0.0f};

	// ワールドトランスフォーム
	KamataEngine::WorldTransform worldTransform_;

	// ======================
	// ポーズ関連
	// ======================

	bool isPaused_ = false;         // ポーズ中か
	Fade* pauseFade_ = nullptr;     // 画面を薄くする専用フェード
	bool pauseHoldReached_ = false; // 所定の薄さで固定したか
	float pauseHoldAlpha_ = 0.5f;   // 画面の薄さ(0.0~1.0)

	bool leaveToStageSelectRequested_ = false; // 「ステージセレクトへ」押下フラグ

	float masterVolume_ = 1.0f; // マスター音量(0.0~1.0)

	// コイン枚数を随時計算（Resultに依存させない）
	int CountCollectedCoins() const;

	// ポーズ中の更新と描画
	void UpdatePauseOverlay();
	void DrawPauseOverlay();
};
