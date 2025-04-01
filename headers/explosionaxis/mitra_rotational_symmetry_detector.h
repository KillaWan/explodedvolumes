#ifndef MC_MITRA_ROTATIONAL_SYMMETRY_DETECTOR_H
#define MC_MITRA_ROTATIONAL_SYMMETRY_DETECTOR_H

#include <vector>
#include <map>
#include <Eigen/Dense>
#include "data.h"

namespace MC {

// Mitra旋转对称性检测器
class MitraRotationalSymmetryDetector {
public:
    MitraRotationalSymmetryDetector() = default;
    ~MitraRotationalSymmetryDetector() = default;
    
    // 检测模型的旋转对称性
    bool detect(const std::vector<Vertex>& meshVertices, Vec3& outRotationAxis, int& outSymmetryOrder);
    
    // 参数设置方法
    void setSampleCount(int count) { m_sampleCount = count; }
    int getSampleCount() const { return m_sampleCount; }
    
    void setSymmetryOrder(int order) { m_symmetryOrder = order; }
    int getSymmetryOrder() const { return m_symmetryOrder; }
    
    void useCustomAxis(bool use) { m_useCustomAxis = use; }
    bool isUsingCustomAxis() const { return m_useCustomAxis; }
    
    void setCustomAxis(const Vec3& axis) { m_customAxis = axis; }
    const Vec3& getCustomAxis() const { return m_customAxis; }
    
    // 配置检测器
    void configure(int sampleCount, int symmetryOrder, bool useCustomAxis, const Vec3& customAxis) {
        m_sampleCount = sampleCount;
        m_symmetryOrder = symmetryOrder;
        m_useCustomAxis = useCustomAxis;
        m_customAxis = customAxis;
    }
    
private:
    // 随机采样点
    std::vector<Vertex> randomSampling(const std::vector<Vertex>& meshVertices, int sampleCount);
    
    // 计算点的对称签名 (论文中的几何特征)
    Eigen::VectorXf computeSignature(const Vertex& vertex, const std::vector<Vertex>& meshVertices);
    
    // 配对点并在变换空间中投票
    std::map<Vec3, int> pairPointsAndVote(const std::vector<Vertex>& samples, const std::vector<Vertex>& meshVertices);
    
    // 从投票中提取最可能的对称轴
    bool extractSymmetryAxisFromVotes(const std::map<Vec3, int>& votes, Vec3& outAxis, int& outSymmetryOrder, int minVotes = 3);
    
    // 计算旋转变换矩阵
    Eigen::Matrix4f computeRotationTransform(const Vertex& p1, const Vertex& p2, const Vec3& axis);
    
    // 验证对称性
    bool verifySymmetry(const std::vector<Vertex>& meshVertices, const Vec3& axis, int& outSymmetryOrder);
    
    // 参数
    int m_sampleCount = 100;          // 采样点数量
    int m_symmetryOrder = 4;          // 对称阶数
    bool m_useCustomAxis = false;     // 是否使用自定义轴
    Vec3 m_customAxis = {0.0f, 0.0f, 1.0f};    // 自定义轴（默认z轴）
    float m_signatureRatio = 0.1f;

};

} // namespace MC

#endif // MC_MITRA_ROTATIONAL_SYMMETRY_DETECTOR_H