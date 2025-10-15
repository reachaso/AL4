#include "Fade.h"
#include "struct.h"
#include <algorithm>

void Fade::Initialize() {

	sprite_ = Sprite::Create(textureHandle_, {0, 0});
	sprite_->SetSize(Vector2(1280.0f, 720.0f));
	sprite_->SetColor(Vector4(0, 0, 0, 1));
}

void Fade::Update() {

	switch (status_) {

	case Status::None:

		break;

	case Status::FadeIn:

		counter_ += 1.0f / 60.0f; // Assuming 60 FPS
		if (counter_ >= duration_) {
			counter_ = duration_;
		}
		sprite_->SetColor(Vector4(0, 0, 0, std::clamp(1.0f - counter_ / duration_, 0.0f, 1.0f)));

		break;

	case Status::FadeOut:

		counter_ += 1.0f / 60.0f; // Assuming 60 FPS
		if (counter_ >= duration_) {
			counter_ = duration_;
		}

		sprite_->SetColor(Vector4(0, 0, 0, std::clamp(counter_ / duration_, 0.0f, 1.0f)));

		break;
	}
}

void Fade::Draw() {

	if (status_ == Status::None) {
		return;
	}

	Sprite::PreDraw();
	sprite_->Draw();
	Sprite::PostDraw();
}

void Fade::Start(Status status, float duration) {

	status_ = status;
	duration_ = duration;
	counter_ = 0.0f;
}

void Fade::Stop() {
	status_ = Status::None;
	counter_ = 0.0f;
	sprite_->SetColor(Vector4(0, 0, 0, 1));
}

bool Fade::IsFinished() {
	switch (status_) {
	case Fade::Status::FadeIn:
	case Fade::Status::FadeOut:
		if (counter_ >= duration_) {
			return true;
		} else {
			return false;
		}
	}
	return true;
}
