#pragma once
#include <KamataEngine.h>
#include <algorithm>

using namespace KamataEngine;

class Fade {

public:

	enum class Status {
		None,
		FadeIn,
		FadeOut,
	};

	void Initialize();

	void Update();

	void Draw();

	void Start(Status status, float duration);

	void Stop();

	bool IsFinished();

	Status GetStatus() const { return status_; }
	// 0.0～1.0 の正規化進捗
	float GetNormalized() const { return (duration_ > 0.0f) ? std::clamp(counter_ / duration_, 0.0f, 1.0f) : 1.0f; }
	// 0.0～1.0 で任意の位置に固定（counter_を書き換える）
	void SetNormalized(float t) {
		t = std::clamp(t, 0.0f, 1.0f);
		counter_ = duration_ * t;
	}

private:

	Sprite* sprite_ = nullptr;
	uint32_t textureHandle_ = 0;

	Status status_ = Status::None;

	float duration_ = 0.0f;
	float counter_ = 0.0f;

	bool isFinished_ = false;
};
