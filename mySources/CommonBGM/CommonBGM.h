#pragma once
#include <KamataEngine.h>
using namespace KamataEngine;

/// 共通BGM(きみのままでいい.mp3)を管理する簡易シングルトン
class CommonBGM {
public:
	static CommonBGM* GetInstance() {
		static CommonBGM i;
		return &i;
	}

	// 初回だけ読み込み。デフォルトで「きみのままでいい.mp3」
	void Initialize(const char* filename);

	// 常時ループ再生（すでに鳴っていたら安全に鳴らし直す）
	void Play();

	// 完全停止（クリア/デスなど、一時的に別BGMへ切り替える前提）
	void Stop();

	// 一時停止/再開がある実装なら使えるようにしておく（なければ Stop/Play で代替）
	void Pause();
	void Resume();

	void SetVolume(float v);
	float GetVolume() const { return volume_; }

	bool IsPlaying() const { return isPlaying_; }

private:
	bool initialized_ = false;
	bool isPlaying_ = false;
	uint32_t dataHandle_ = 0;  // BGMデータ
	uint32_t voiceHandle_ = 0; // 再生中ハンドル

	float volume_ = 1.0f;
	void ApplyVolume();
};
