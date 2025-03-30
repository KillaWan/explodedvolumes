#ifndef SYMMETRY_TYPES_H
#define SYMMETRY_TYPES_H

#include "data.h"

namespace MC {

// 对称类型枚举
enum class SymmetryType {
    None,           // 没有检测到对称
    Rotational,     // 检测到旋转对称
    Reflective      // 检测到镜面对称
};

// 对称检测结果结构
struct SymmetryResult {
    SymmetryType type;
    int symmetryOrder;      // 旋转对称阶数（如果 type = Rotational）
    Vec3 rotationAxis;      // 旋转对称轴
    Vec3 reflectiveNormal;  // 镜面对称的平面法向（如果 type = Reflective）
};

} // namespace MC

#endif // SYMMETRY_TYPES_H