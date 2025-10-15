#include "Skydome.h"

void Skydome::Initialize() {

	worldTransform_.Initialize();
}

void Skydome::Update() {
	
}

void Skydome::Draw(const Camera& camera) {

	model_->Draw(worldTransform_, camera); 

}
