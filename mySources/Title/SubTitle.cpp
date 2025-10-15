#include "SubTitle.h"

void SubTitle3D::Initialize(Model* model, Camera* camera) {
	model_ = model;
	camera_ = camera;
	world_.Initialize();
	baseTranslation_ = world_.translation_;
}

// タイトルに追従させる（タイトルの真下に置く想定）
void SubTitle3D::AttachTo(Title* title, const Vector3& offsetFromTitle) {
	attachTitle_ = title;
	offset_ = offsetFromTitle;
}

// 点滅（ON/OFF）…矩形波で確実に効く版（α操作が無くてもOK）
// speed: 1フレームあたりの位相増分（0.05〜0.2くらい）
// dutyHigh: ONの時間比率（0〜1）
void SubTitle3D::SetBlinkHard(float speed, float dutyHigh) {
	blinkSpeed_ = speed;
	dutyHigh_ = std::clamp(dutyHigh, 0.0f, 1.0f);
	useSmoothAlpha_ = false;
}

// （任意）スムーズなフェード点滅：エンジン側でモデルのαが使える場合のみ
// minA/maxA は 0〜1。対応APIがなければ見た目はON/OFFのままです（コンパイルは通ります）
void SubTitle3D::SetBlinkSmooth(float speed, float minA, float maxA) {
	blinkSpeed_ = speed;
	minAlpha_ = minA;
	maxAlpha_ = maxA;
	useSmoothAlpha_ = true;
}

void SubTitle3D::Update() {
	// タイトルに追従
	if (attachTitle_) {
		const auto& t = attachTitle_->GetWorldTransform().translation_;
		world_.translation_ = {t.x + offset_.x, t.y + offset_.y, t.z + offset_.z};
	}

	// 点滅位相
	phase_ += blinkSpeed_;
	if (phase_ > 6.2831853f)
		phase_ -= 6.2831853f;

	// ハード点滅：表示/非表示の切り替えだけ（確実に効く）
	if (!useSmoothAlpha_) {
		float t01 = (std::sin(phase_) * 0.5f + 0.5f); // 0〜1
		bool on = (t01 <= dutyHigh_);
		effectiveVisible_ = visible_ && on;
	} else {
		// スムーズ点滅（αフェード）…対応APIが無い場合は effectiveVisible_ のみ効きます
		float t01 = (std::sin(phase_) * 0.5f + 0.5f);
		currentAlpha_ = minAlpha_ + (maxAlpha_ - minAlpha_) * t01;
		effectiveVisible_ = visible_;
	}

	// 行列更新
	using namespace KamataEngine::MathUtility;
	Matrix4x4 S = MakeScaleMatrix(scale_);
	Matrix4x4 Rx = MakeRotateXMatrix(rotation_.x);
	Matrix4x4 Ry = MakeRotateYMatrix(rotation_.y);
	Matrix4x4 Rz = MakeRotateZMatrix(rotation_.z);
	Matrix4x4 R = Rz * Ry * Rx;
	Matrix4x4 T = MakeTranslateMatrix(world_.translation_);
	world_.matWorld_ = S * R * T;
	world_.TransferMatrix();
}

void SubTitle3D::Draw() {
	if (!effectiveVisible_ || !model_)
		return;
	model_->Draw(world_, *camera_);
}
