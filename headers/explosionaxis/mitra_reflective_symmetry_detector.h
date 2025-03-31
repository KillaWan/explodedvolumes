#ifndef MC_MITRA_REFLECTIVE_SYMMETRY_DETECTOR_H
#define MC_MITRA_REFLECTIVE_SYMMETRY_DETECTOR_H

#include <vector>
#include <map>
#include <Eigen/Dense>
#include "data.h"

namespace MC {

// Mitra反射对称性检测器
class MitraReflectiveSymmetryDetector {
public:
    MitraReflectiveSymmetryDetector() = default;
    ~MitraReflectiveSymmetryDetector() = default;
    
    // 检测模型的反射对称性
    bool detect(const std::vector<Vertex>& meshVertices, Vec3& outReflectiveNormal);
    
    // 参数设置方法
    void setSampleCount(int count) { m_sampleCount = count; }
    int getSampleCount() const { return m_sampleCount; }
    
    void useCustomNormal(bool use) { m_useCustomNormal = use; }
    bool isUsingCustomNormal() const { return m_useCustomNormal; }
    
    void setCustomNormal(const Vec3& normal) { m_customNormal = normal; }
    const Vec3& getCustomNormal() const { return m_customNormal; }
    
    // 配置检测器
    void configure(int sampleCount, bool useCustomNormal, const Vec3& customNormal) {
        m_sampleCount = sampleCount;
        m_useCustomNormal = useCustomNormal;
        m_customNormal = customNormal;
    }
    
private:
    // 随机采样点
    std::vector<Vertex> randomSampling(const std::vector<Vertex>& meshVertices, int sampleCount);
    
    // 计算点的对称签名 (论文中的几何特征)
    Eigen::VectorXf computeSignature(const Vertex& vertex, const std::vector<Vertex>& meshVertices);
    
    // 配对点并在变换空间中投票
    std::map<Vec3, int> pairPointsAndVote(const std::vector<Vertex>& samples, const std::vector<Vertex>& meshVertices);
    
    // 从投票中提取最可能的对称平面
    bool extractSymmetryPlaneFromVotes(const std::map<Vec3, int>& votes, Vec3& outNormal, int minVotes = 3);
    
    // 计算反射变换矩阵
    Eigen::Matrix4f computeReflectionTransform(const Vertex& p1, const Vertex& p2);
    
    // 验证对称性
    bool verifySymmetry(const std::vector<Vertex>& meshVertices, const Vec3& normal);
    
    // 参数
    int m_sampleCount = 100;            // 采样点数量
    bool m_useCustomNormal = false;     // 是否使用自定义法向
    Vec3 m_customNormal = {0.0f, 0.0f, 1.0f};    // 自定义法向（默认z轴）
};

} // namespace MC

#endif // MC_MITRA_REFLECTIVE_SYMMETRY_DETECTOR_H