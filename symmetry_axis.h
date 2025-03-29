#ifndef SYMMETRY_AXIS_H
#define SYMMETRY_AXIS_H

#include <vector>
#include "marching_cubes_v3.h"  // 假设这里声明了 Vertex, Vec3

// 对称类型枚举
enum class SymmetryType {
    None,           // 未检测到对称
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

// 主接口：根据网格顶点计算爆炸轴
// 优先级：
//   1) 若检测到旋转对称（返回阶数最高的），使用其旋转轴；
//   2) 若只检测到镜面对称，则对镜面投影后进行 2D PCA；
//   3) 否则直接对整个模型做 3D PCA。
// userDefinedAxis 可选，用于用户手动指定轴（优先使用）。
Vec3 computeExplosionAxis(const std::vector<Vertex>& meshVertices, 
                            const Vec3* userDefinedAxis = nullptr);

// 辅助接口
SymmetryResult detectSymmetry(const std::vector<Vertex>& meshVertices);
SymmetryResult detectRotationalSymmetry(const std::vector<Vertex>& meshVertices);
SymmetryResult detectReflectiveSymmetry(const std::vector<Vertex>& meshVertices);
Vec3 pca3D(const std::vector<Vertex>& meshVertices);
Vec3 pca2DOnPlane(const std::vector<Vertex>& meshVertices, const Vec3& planeNormal);
Vec3 computeCentroid(const std::vector<Vertex>& meshVertices);
Vec3 projectPointOntoPlane(const Vec3& point, const Vec3& planeNormal, const Vec3& pointOnPlane);

// 内联的向量运算
inline Vec3 normalize(const Vec3& v) {
    float len = std::sqrt(v.x*v.x + v.y*v.y + v.z*v.z);
    return (len < 1e-12f) ? Vec3(0,0,1) : Vec3(v.x/len, v.y/len, v.z/len);
}
inline float dot(const Vec3& a, const Vec3& b) {
    return a.x*b.x + a.y*b.y + a.z*b.z;
}
inline Vec3 cross(const Vec3& a, const Vec3& b) {
    return Vec3(a.y*b.z - a.z*b.y,
                a.z*b.x - a.x*b.z,
                a.x*b.y - a.y*b.x);
}

#endif // SYMMETRY_AXIS_H
