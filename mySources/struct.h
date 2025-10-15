#pragma once
#include <KamataEngine.h>
#include <numbers>

using namespace KamataEngine;

// DirectXCommonの共有インスタンス
extern DirectXCommon* dxCommon;

struct AABB {  
    Vector3 min;  
    Vector3 max;  

    // AABB同士の衝突判定関数  
    static bool CheckCollision(const AABB& a, const AABB& b) {  
        return (a.min.x <= b.max.x && a.max.x >= b.min.x) &&  
               (a.min.y <= b.max.y && a.max.y >= b.min.y) &&  
               (a.min.z <= b.max.z && a.max.z >= b.min.z);  
    }  
};

// easeInOutSine関数の実装
inline float easeInOutSine(float x) { return -(std::cos(std::numbers::pi_v<float> * x) - 1.0f) / 2.0f; }

// 線形補間関数
inline float lerp(float a, float b, float t) { return a + (b - a) * t; }
