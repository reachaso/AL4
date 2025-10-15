#pragma once
#include <KamataEngine.h>

using namespace KamataEngine;

class Title {

public:
	void Initialize(Camera* camera);
	void Update();
	void Draw();

	void SetModel(Model* model) { model_ = model; }

	// アニメーションのパラメータ設定（必要に応じて呼び出し）
	void SetFloatParams(float amplitude, float speed) {
		floatAmplitude_ = amplitude;
		floatSpeed_ = speed;
	}
	// 基準位置を外から決めたい場合はこれで設定（未呼び出しならInitialize時点の位置が基準）
	void SetBaseTranslation(const Vector3& t) {
		baseTranslation_ = t;
		worldTransform_.translation_ = t;
	}

	void UpdateAffineTransformMatrix();
	Vector3 GetWorldPosition();

	// 必要なら外から直接Transformをいじれるように
	WorldTransform& GetWorldTransform() { return worldTransform_; }

private:
	Model* model_ = nullptr;
	Camera* camera_ = nullptr;
	WorldTransform worldTransform_;

	// ==== アニメーション ====
	Vector3 baseTranslation_{};    // 揺らす前の基準位置
	float floatAmplitude_ = 0.25f; // 上下の振れ幅（お好みで）
	float floatSpeed_ = 0.04f;     // 速さ（ラジアン/フレーム）
	float floatPhase_ = 0.0f;      // 現在位相
};
