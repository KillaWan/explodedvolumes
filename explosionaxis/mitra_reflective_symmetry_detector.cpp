#include "explosionaxis/mirta_reflective_symmetry_detector.h"
#include <iostream>

namespace MC {

using namespace VectorOps;

// Mitra反射对称性检测实现
bool MitraReflectiveSymmetryDetector::detect(const std::vector<Vertex>& meshVertices, 
                                             Vec3& outReflectiveNormal) {
    Vec3 centroid = computeCentroid(meshVertices);
    
    // 简单判断：如果模型中心的 Y 坐标接近 0，则认为存在镜面对称（关于 XZ 平面）
    if (std::abs(centroid.y) < 1e-3f) {
        outReflectiveNormal = normalize(Vec3(0, 1, 0));
        std::cout << "Mitra反射对称性检测: 发现反射平面" << std::endl;
        return true;
    }
    
    std::cout << "Mitra反射对称性检测: 未发现反射平面" << std::endl;
    return false;
}

} // namespace MC