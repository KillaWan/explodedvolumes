#ifndef MC_MITRA_ROTATIONAL_SYMMETRY_DETECTOR_H
#define MC_MITRA_ROTATIONAL_SYMMETRY_DETECTOR_H

#include "symmetry_detector_interface.h"
#include "vector_ops.h"
#include <Eigen/Dense>
#include <map>

namespace MC {

// Mitra论文中的旋转对称性检测严格实现
class MitraRotationalSymmetryDetector : public RotationalSymmetryDetector {
public:
    bool detect(const std::vector<Vertex>& meshVertices, 
                Vec3& outRotationAxis, 
                int& outSymmetryOrder) override;
    
    std::string getName() const override { return "Mitra Rotational Detector"; }

private:
    // 随机采样算法
    std::vector<Vertex> randomSampling(const std::vector<Vertex>& meshVertices, int sampleCount);
    
    // 计算对称签名
    Eigen::VectorXf computeSignature(const Vertex& vertex, const std::vector<Vertex>& meshVertices);
    
    // 配对点并投票
    std::map<Vec3, int> pairPointsAndVote(const std::vector<Vertex>& samples, const std::vector<Vertex>& meshVertices);
    
    // 从投票中提取最可能的对称轴
    bool extractSymmetryAxisFromVotes(const std::map<Vec3, int>& votes, 
                                      Vec3& outAxis, 
                                      int& outSymmetryOrder,
                                      int minVotes = 5);
    
    // 计算旋转变换
    Eigen::Matrix4f computeRotationTransform(const Vertex& p1, const Vertex& p2, const Vec3& axis);
    
    // 验证对称性
    bool verifySymmetry(const std::vector<Vertex>& meshVertices, 
                        const Vec3& axis, 
                        int& outSymmetryOrder);
};

} // namespace MC

#endif // MC_ROTATIONAL_SYMMETRY_DETECTOR_H