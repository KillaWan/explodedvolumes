#include "explosionaxis/mitra_rotational_symmetry_detector.h"
#include "explosionaxis/pca_analyzer.h"
#include "explosionaxis/vector_ops.h"
#include <iostream>

namespace MC {

using namespace VectorOps;

// Mitra旋转对称性检测实现
bool MitraRotationalSymmetryDetector::detect(const std::vector<Vertex>& meshVertices, 
                                             Vec3& outRotationAxis, 
                                             int& outSymmetryOrder) {
    // 采用 3D PCA 得到主轴
    Vec3 pcaAxis = PCAAnalyzer::compute3DPCA(meshVertices);
    
    // 这里作为简化策略：如果 PCA 结果与 Z 轴近似对齐，则认为存在旋转对称性
    Vec3 zAxis(0, 0, 1);
    float cosine = std::abs(dot(normalize(pcaAxis), zAxis));
    
    // 如果夹角小于约 25°（cosine > 0.9），则认为存在旋转对称性；同时设定对称阶数为 4（示例）
    if (cosine > 0.9f) {
        outRotationAxis = normalize(pcaAxis);
        outSymmetryOrder = 4;
        std::cout << "Mitra旋转对称性检测: 发现对称轴" << std::endl;
        return true;
    }
    
    std::cout << "Mitra旋转对称性检测: 未发现对称轴" << std::endl;
    return false;
}

} // namespace MC