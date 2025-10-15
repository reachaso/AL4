#pragma once
#include "Title/Title.h"
#include <KamataEngine.h>
#include <algorithm>

using namespace KamataEngine;

class SubTitle3D {
public:
	void Initialize(Model* model, Camera* camera);

	// タイトルに追従させる（タイトルの真下に置く想定）
	void AttachTo(Title* title, const Vector3& offsetFromTitle = {0.0f, -2.0f, 0.0f});

	// 点滅（ON/OFF）…矩形波で確実に効く版（α操作が無くてもOK）
	// speed: 1フレームあたりの位相増分（0.05〜0.2くらい）
	// dutyHigh: ONの時間比率（0〜1）
	void SetBlinkHard(float speed, float dutyHigh = 0.6f);

	// （任意）スムーズなフェード点滅：エンジン側でモデルのαが使える場合のみ
	// minA/maxA は 0〜1。対応APIがなければ見た目はON/OFFのままです（コンパイルは通ります）
	void SetBlinkSmooth(float speed, float minA = 0.2f, float maxA = 1.0f);

	void SetScale(const Vector3& s) { scale_ = s; }
	void SetRotation(const Vector3& r) { rotation_ = r; }
	void SetVisible(bool v) { visible_ = v; }

	void Update();

	void Draw();

private:
	Model* model_ = nullptr;
	Camera* camera_ = nullptr;
	WorldTransform world_;

	Title* attachTitle_ = nullptr;
	Vector3 offset_{0.0f, -2.0f, 0.0f};

	// 変形
	Vector3 scale_{1.0f, 1.0f, 1.0f};
	Vector3 rotation_{0.0f, 0.0f, 0.0f};
	Vector3 baseTranslation_{};

	// 表示制御
	bool visible_ = true;
	bool effectiveVisible_ = true;

	// 点滅
	float phase_ = 0.0f;
	float blinkSpeed_ = 0.10f;

	// ハード点滅
	float dutyHigh_ = 0.6f;

	// スムーズ点滅
	bool useSmoothAlpha_ = false;
	float minAlpha_ = 0.2f;
	float maxAlpha_ = 1.0f;
	float currentAlpha_ = 1.0f; // SetColor等がある場合に使用
};
