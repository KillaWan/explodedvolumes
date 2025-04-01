#pragma once

#include "data.h"
#include <vector>
#include <map>
#include <Eigen/Dense>

namespace MC {

class MitraReflectiveSymmetryDetector {
public:
    MitraReflectiveSymmetryDetector()
        : m_sampleCount(100), m_useCustomNormal(false), m_cosineThreshold(0.95f)
    {}

    // 执行反射对称性检测，返回是否检测到，并在 outReflectiveNormal 中输出候选平面法向量
    bool detect(const std::vector<Vertex>& meshVertices, Vec3& outReflectiveNormal);

    // 设置采样数
    void setSampleCount(int count) { m_sampleCount = count; }
    // 是否使用自定义法向量
    void useCustomNormal(bool use) { m_useCustomNormal = use; }
    // 设置自定义法向量
    void setCustomNormal(const Vec3& normal) { m_customNormal = normal; }
    // 设置余弦相似度阈值（0~1之间，数值越大越严格）
    void setCosineThreshold(float threshold) { m_cosineThreshold = threshold; }

private:
    int m_sampleCount;
    bool m_useCustomNormal;
    Vec3 m_customNormal;
    // 采用余弦相似度判断采样点对是否相似，默认 0.95，越接近 1 表示要求越严格
    float m_cosineThreshold;

    // 随机采样部分顶点
    std::vector<Vertex> randomSampling(const std::vector<Vertex>& meshVertices, int sampleCount);
    // 计算给定点的几何签名（归一化后）
    Eigen::VectorXf computeSignature(const Vertex& vertex, const std::vector<Vertex>& meshVertices);
    // 对采样点对计算签名相似性，并对候选对称平面法向量投票
    std::map<Vec3, int> pairPointsAndVote(const std::vector<Vertex>& samples, const std::vector<Vertex>& meshVertices);
    // 从投票中提取候选对称平面法向量，票数足够则返回 true
    bool extractSymmetryPlaneFromVotes(const std::map<Vec3, int>& votes, Vec3& outNormal, int minVotes = 10);
    // 验证候选法向量是否能较好地反映反射对称性（简化版本）
    bool verifySymmetry(const std::vector<Vertex>& meshVertices, const Vec3& normal);

    // 计算反射变换矩阵（用于其他用途，此处保留）
    Eigen::Matrix4f computeReflectionTransform(const Vertex& p1, const Vertex& p2);
};

} // namespace MC
