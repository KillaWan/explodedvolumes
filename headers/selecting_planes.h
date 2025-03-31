#pragma once

#include "data.h"
#include <vector>

namespace MC
{

    // 表示一个切割平面的结构
    struct CuttingPlane
    {
        Vec3 origin;    // 平面上的一点（通常在爆炸轴上）
        Vec3 normal;    // 平面法向量（通常就是爆炸轴方向）
        float distance; // 沿爆炸轴的距离参数
    };

    // 生成均匀分布的切割平面
    std::vector<CuttingPlane> generateUniformCuttingPlanes(
        const Mesh &mesh,
        const Vec3 &explosionAxis,
        int numPlanes = 5);

    // 检查某个切割平面是否应该被保留（第二阶段实现）
    bool shouldKeepCuttingPlane(const CuttingPlane &plane, const Mesh &mesh);

} // namespace MC