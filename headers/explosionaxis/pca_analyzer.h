#ifndef MC_PCA_ANALYZER_H
#define MC_PCA_ANALYZER_H

#include <vector>
#include "data.h"
#include "explosionaxis/vector_ops.h"

namespace MC {

// PCA分析器类
class PCAAnalyzer {
public:
    PCAAnalyzer() = default;
    ~PCAAnalyzer() = default;
    
    // 主方法：分析模型并确定主轴方向
    Vec3 analyzePrincipalAxis(const std::vector<Vertex>& meshVertices);
    
    // 设置参数
    void setUseLongestAxis(bool use) { m_useLongestAxis = use; }
    bool isUsingLongestAxis() const { return m_useLongestAxis; }
    
    // 3D PCA计算
    Vec3 compute3DPCA(const std::vector<Vertex>& meshVertices);
    
    // 在特定平面上计算2D PCA
    Vec3 compute2DPCAOnPlane(const std::vector<Vertex>& meshVertices, const Vec3& planeNormal);
    
private:
    bool m_useLongestAxis = true;
};

} // namespace MC

#endif // MC_PCA_ANALYZER_H