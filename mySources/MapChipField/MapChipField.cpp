#include "MapChipField.h"
#include <cassert>
#include <fstream>
#include <map>
#include <sstream>

namespace {
std::map<std::string, MapChipType> mapChipTable = {
    {"0", MapChipType::kAir  },
    {"1", MapChipType::kBlock},
};
}

void MapChipField::ResetMapChipData() {

	mapChipData_.data.clear();
	mapChipData_.data.resize(kNumBlockVirtical);

	for (std::vector<MapChipType>& mapChipDaraLine : mapChipData_.data) {
		mapChipDaraLine.resize(kNumBlockHorizontal);
	}

	playerSpawnIndex_.reset();
	enemySpawnIndices_.clear();
	coinSpawnIndices_.clear();
}

void MapChipField::LoadMapChipCSV(const std::string& filePath) {

	ResetMapChipData();

	std::ifstream file;
	file.open(filePath);
	assert(file.is_open());

	std::stringstream mapChipCSV;
	// ファイルの内容を文字列ストリームに読み込む
	mapChipCSV << file.rdbuf();
	// ファイルを閉じる
	file.close();

	for (uint32_t i = 0; i < kNumBlockVirtical; ++i) {
		std::string line;
		std::getline(mapChipCSV, line);

		// 1行分の文字列をストリームに変換して解析しやすくする
		std::istringstream line_stream(line);

		for (uint32_t j = 0; j < kNumBlockHorizontal; ++j) {
			std::string word;
			getline(line_stream, word, ',');

			if (word == "p" || word == "P") {
				playerSpawnIndex_ = IndexSet{j, i};
				mapChipData_.data[i][j] = MapChipType::kAir; // スポーンは空扱い
				continue;
			} else if (word == "e" || word == "E") {
				enemySpawnIndices_.push_back(IndexSet{j, i});
				mapChipData_.data[i][j] = MapChipType::kAir; // スポーンは空扱い
				continue;
			} else if (word == "c" || word == "C") {
				coinSpawnIndices_.push_back(IndexSet{j, i});
				mapChipData_.data[i][j] = MapChipType::kAir;
				continue;
			} else if (word == "g" || word == "G") {
				goalSpawnIndices_.push_back(IndexSet{j, i});
				continue;
			}

			// 既存の "0"/"1" ルックアップ（未知は assert
			if (mapChipTable.contains(word)) {
				mapChipData_.data[i][j] = mapChipTable[word];
			} else {
				assert(false && "Unknown map chip type in CSV file.");
			}
		}
	}
}

MapChipType MapChipField::GetMapChipTypeByIndex(uint32_t xIndex, uint32_t yIndex) {

	if (xIndex < 0 || kNumBlockHorizontal - 1 < xIndex) {
		return MapChipType::kAir;
	}
	if (yIndex < 0 || kNumBlockVirtical - 1 < yIndex) {
		return MapChipType::kAir;
	}

	return mapChipData_.data[yIndex][xIndex];
}

Vector3 MapChipField::GetMapChipPositionByIndex(uint32_t xIndex, uint32_t yIndex) { return Vector3(kBlockWidth * xIndex, kBlockHeight * (kNumBlockVirtical - 1 - yIndex), 0); }

void MapChipField::GenerateBlocks() {
	// 既存のブロックを解放
	for (auto& line : worldTransformBlocks_) {
		for (auto* block : line) {
			delete block;
		}
	}
	worldTransformBlocks_.clear();

	const uint32_t numVertical = static_cast<uint32_t>(mapChipData_.data.size());
	const uint32_t numHorizontal = numVertical > 0 ? static_cast<uint32_t>(mapChipData_.data[0].size()) : 0;

	worldTransformBlocks_.resize(numVertical);
	for (uint32_t i = 0; i < numVertical; ++i) {
		worldTransformBlocks_[i].resize(numHorizontal, nullptr);
		for (uint32_t j = 0; j < numHorizontal; ++j) {
			MapChipType mapChipType = GetMapChipTypeByIndex(j, i);
			if (mapChipType == MapChipType::kBlock) { // ブロックの場合ブロックを描画
				// 位置を取得
				Vector3 position = GetMapChipPositionByIndex(j, i);
				// ワールドトランスフォームを生成
				WorldTransform* worldTransform = new WorldTransform();
				worldTransform->Initialize();
				worldTransform->translation_ = position;
				worldTransformBlocks_[i][j] = worldTransform;
			}
		}
	}
}

MapChipField::IndexSet MapChipField::GetMapChipIndexSetByPosition(const Vector3& position) {

	IndexSet indexSet = {};

	indexSet.xIndex = static_cast<uint32_t>((position.x + kBlockWidth / 2) / kBlockWidth);

	// 反転前のy座標を計算
	IndexSet indexSetTemp;
	indexSetTemp.yIndex = static_cast<uint32_t>((position.y + kBlockHeight / 2) / kBlockHeight);

	// 反転後のy座標を計算
	indexSet.yIndex = static_cast<uint32_t>(kNumBlockVirtical - 1 - indexSetTemp.yIndex);

	return indexSet;
}

MapChipField::Rect MapChipField::GetRectByIndex(uint32_t xIndex, uint32_t yIndex) {

	Vector3 center = GetMapChipPositionByIndex(xIndex, yIndex);

	Rect rect;

	rect.left = center.x - kBlockWidth / 2;
	rect.right = center.x + kBlockWidth / 2;
	rect.top = center.y + kBlockHeight / 2;
	rect.bottom = center.y - kBlockHeight / 2;

	return rect;
}
