#include "CommonBGM.h"
#include <algorithm>

void CommonBGM::Initialize(const char* filename = "kiminomamadeii.mp3") {
	if (initialized_)
		return;
	dataHandle_ = Audio::GetInstance()->LoadWave(filename);
	initialized_ = true;
}

// 常時ループ再生（すでに鳴っていたら安全に鳴らし直す）
void CommonBGM::Play() {
	if (!initialized_)
		Initialize("kiminomamadeii.mp3");
	if (isPlaying_) {
		Audio::GetInstance()->StopWave(voiceHandle_);
		isPlaying_ = false;
	}
	voiceHandle_ = Audio::GetInstance()->PlayWave(dataHandle_, /*loop=*/true);
	isPlaying_ = true;

	ApplyVolume();
}

// 完全停止（クリア/デスなど、一時的に別BGMへ切り替える前提）
void CommonBGM::Stop() {
	if (isPlaying_) {
		Audio::GetInstance()->StopWave(voiceHandle_);
		isPlaying_ = false;
	}
}

// 一時停止/再開がある実装なら使えるようにしておく（なければ Stop/Play で代替）
void CommonBGM::Pause() {
	if (isPlaying_) {
		Audio::GetInstance()->PauseWave(voiceHandle_);
	}
}
void CommonBGM::Resume() {
	if (!initialized_)
		return;
	// Resume がなければ Play() で鳴らし直す
	if (/* 実装に Resume がある想定 */ false) {
		Audio::GetInstance()->ResumeWave(voiceHandle_);
	} else {
		Play();
	}
}

void CommonBGM::ApplyVolume() {
	if (!isPlaying_)
		return;
	Audio::GetInstance()->SetVolume(voiceHandle_, volume_);
}

void CommonBGM::SetVolume(float v) {
	volume_ = std::clamp(v, 0.0f, 1.0f);
	ApplyVolume(); // 再生中なら即反映
}
