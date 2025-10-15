#include "Game/Game.h"
#include <KamataEngine.h>
#include <Windows.h>

using namespace KamataEngine;

// Windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {
	// KamataEngineの初期化
	KamataEngine::Initialize(L"LE2B_04_オオシマ_タイガ_奈落ランナー");
	// DirectXCommonインスタンスの取得
	DirectXCommon* dxCommonInstance = DirectXCommon::GetInstance();

	Game* game = new Game();
	game->Inisialize();

	//==============================
	// ゲームループ
	//==============================
	while (true) {
		// エンジンの更新
		if (KamataEngine::Update()) {
			break; // ゲームループを抜ける
		}

		//==============================
		// 更新処理開始
		//==============================

		game->Update();

		//==============================
		// 更新処理終了
		//==============================

		//==============================
		// 描画処理開始
		//==============================
		dxCommonInstance->PreDraw();

		game->Draw();

		dxCommonInstance->PostDraw();
		//==============================
		// 描画処理終了
		//==============================
	}

	game->~Game();

	// KamataEngineの終了処理
	KamataEngine::Finalize();
	return 0;
}
