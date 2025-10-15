#pragma once
#include <KamataEngine.h>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

using namespace KamataEngine;

enum class MapChipType {
	kAir,   // 空白
	kBlock, // ブロック
};

struct MapChipData {
	std::vector<std::vector<MapChipType>> data;
};

class MapChipField {

public:
	struct IndexSet {
		uint32_t xIndex;
		uint32_t yIndex;
	};

	struct Rect {
		float left;
		float right;
		float bottom;
		float top;
	};

	MapChipData mapChipData_;

	void ResetMapChipData();

	void LoadMapChipCSV(const std::string& filePath);

	MapChipType GetMapChipTypeByIndex(uint32_t xIndex, uint32_t yIndex);

	Vector3 GetMapChipPositionByIndex(uint32_t xIndex, uint32_t yIndex);

	// ブロックのワールドトランスフォーム配列
	std::vector<std::vector<WorldTransform*>> worldTransformBlocks_;
	// 表示ブロックの生成
	void GenerateBlocks();

	IndexSet GetMapChipIndexSetByPosition(const Vector3& position);

	Rect GetRectByIndex(uint32_t xIndex, uint32_t yIndex);

	// スポーン情報の取得
	const std::optional<IndexSet>& GetPlayerSpawnIndex() const { return playerSpawnIndex_; }
	const std::vector<IndexSet>& GetEnemySpawnIndices() const { return enemySpawnIndices_; }
	const std::vector<IndexSet>& GetCoinSpawnIndices() const { return coinSpawnIndices_; }
	const std::vector<IndexSet>& GetGoalSpawnIndices() const { return goalSpawnIndices_; }


	static float GetBlockWidth() { return kBlockWidth; }
	static float GetBlockHeight() { return kBlockHeight; }
	static uint32_t GetNumBlockVirtical() { return kNumBlockVirtical; }
	static uint32_t GetNumBlockHorizontal() { return kNumBlockHorizontal; }

private:
	static inline const float kBlockWidth = 2.0f;
	static inline const float kBlockHeight = 2.0f;

	static inline const uint32_t kNumBlockVirtical = 20;
	static inline const uint32_t kNumBlockHorizontal = 100;

	// スポーン情報
	std::optional<IndexSet> playerSpawnIndex_;
	std::vector<IndexSet> enemySpawnIndices_;
	std::vector<IndexSet> coinSpawnIndices_;
	std::vector<IndexSet> goalSpawnIndices_;
};
