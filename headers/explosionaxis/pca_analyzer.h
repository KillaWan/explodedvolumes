#ifndef MC_PCA_ANALYZER_H
#define MC_PCA_ANALYZER_H

#include "data.h"
#include "vector_ops.h"
#include <vector>

namespace MC {

// PCA分析类
class PCAAnalyzer {
public:
    // 3D PCA：计算协方差矩阵，并返回第一主成分
    static Vec3 compute3DPCA(const std::vector<Vertex>& meshVertices);
    
    // 2D PCA：将顶点投影到指定平面，然后在该平面上计算第一主成分，最后映射回3D
    static Vec3 compute2DPCAOnPlane(const std::vector<Vertex>& meshVertices, const Vec3& planeNormal);
};

} // namespace MC

#endif // MC_PCA_ANALYZER_H