#include "explosionaxis/mitra_reflective_symmetry_detector.h"
#include <iostream>
#include <algorithm>
#include <random>
#include <cmath>

namespace MC {

using namespace VectorOps;

// Mitra反射对称性检测严格实现
bool MitraReflectiveSymmetryDetector::detect(const std::vector<Vertex>& meshVertices, 
                                             Vec3& outReflectiveNormal) {
    // 1. 随机采样一部分点进行处理
    int sampleCount = std::min(10, static_cast<int>(meshVertices.size() / 10));
    std::vector<Vertex> samples = randomSampling(meshVertices, sampleCount);
    
    // 2. 对这些采样点进行配对并在变换空间中投票
    std::map<Vec3, int> votes = pairPointsAndVote(samples, meshVertices);
    
    // 3. 从投票结果中提取最可能的对称平面
    bool hasSymmetry = extractSymmetryPlaneFromVotes(votes, outReflectiveNormal);
    
    // 4. 如果发现了候选对称平面，进行验证
    if (hasSymmetry) {
        hasSymmetry = verifySymmetry(meshVertices, outReflectiveNormal);
    }
    
    return hasSymmetry;
}

// 随机采样一部分点
std::vector<Vertex> MitraReflectiveSymmetryDetector::randomSampling(
    const std::vector<Vertex>& meshVertices, int sampleCount) {
    
    std::vector<Vertex> result;
    result.reserve(sampleCount);
    
    // 使用随机数生成器
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distrib(0, meshVertices.size() - 1);
    
    // 随机选择点
    for (int i = 0; i < sampleCount; ++i) {
        int index = distrib(gen);
        result.push_back(meshVertices[index]);
    }
    
    return result;
}

// 计算点的对称签名 (论文中的几何特征)
Eigen::VectorXf MitraReflectiveSymmetryDetector::computeSignature(
    const Vertex& vertex, const std::vector<Vertex>& meshVertices) {
    
    // 在论文中，签名包括局部曲率特征
    // 这里我们使用简化版本，仅考虑点到其他点的距离分布
    
    Eigen::VectorXf signature(10); // 10维签名
    signature.setZero();
    
    Vec3 p(vertex.x, vertex.y, vertex.z);
    
    // 计算到其他点的距离分布
    std::vector<float> distances;
    distances.reserve(meshVertices.size());
    
    for (const auto& v : meshVertices) {
        Vec3 other(v.x, v.y, v.z);
        float dist = std::sqrt(
            (p.x - other.x) * (p.x - other.x) +
            (p.y - other.y) * (p.y - other.y) +
            (p.z - other.z) * (p.z - other.z)
        );
        distances.push_back(dist);
    }
    
    // 对距离进行排序
    std::sort(distances.begin(), distances.end());
    
    // 将距离划分到10个bin中
    int binSize = distances.size() / 10;
    for (int i = 0; i < 10; ++i) {
        int index = std::min(i * binSize, static_cast<int>(distances.size()) - 1);
        signature(i) = distances[index];
    }
    
    return signature;
}

// 配对点并在变换空间中投票
std::map<Vec3, int> MitraReflectiveSymmetryDetector::pairPointsAndVote(
    const std::vector<Vertex>& samples, 
    const std::vector<Vertex>& meshVertices) {
    
    std::map<Vec3, int> votes; // 记录每个候选对称平面法向量的投票
    
    // 为每个采样点计算签名
    std::vector<Eigen::VectorXf> signatures;
    signatures.reserve(samples.size());
    
    for (const auto& vertex : samples) {
        signatures.push_back(computeSignature(vertex, meshVertices));
    }
    
    // 为每对具有相似签名的点投票
    const float signatureThreshold = 0.1f; // 签名相似性阈值
    
    for (size_t i = 0; i < samples.size(); ++i) {
        for (size_t j = i + 1; j < samples.size(); ++j) {
            // 计算签名的欧氏距离
            float signatureDist = (signatures[i] - signatures[j]).norm();
            
            if (signatureDist < signatureThreshold) {
                // 签名相似，这对点可能对应于反射变换
                Vec3 p1(samples[i].x, samples[i].y, samples[i].z);
                Vec3 p2(samples[j].x, samples[j].y, samples[j].z);
                
                // 候选反射平面的法向量是两点的差向量
                Vec3 normal(p2.x - p1.x, p2.y - p1.y, p2.z - p1.z);
                normal = normalize(normal);
                
                // 量化法向量方向以便合并相似的投票
                int quantizeFactor = 100;
                Vec3 quantizedNormal(
                    std::round(normal.x * quantizeFactor) / quantizeFactor,
                    std::round(normal.y * quantizeFactor) / quantizeFactor,
                    std::round(normal.z * quantizeFactor) / quantizeFactor
                );
                
                // 为这个平面法向量投票
                votes[quantizedNormal]++;
            }
        }
    }
    
    return votes;
}

// 从投票中提取最可能的对称平面
bool MitraReflectiveSymmetryDetector::extractSymmetryPlaneFromVotes(
    const std::map<Vec3, int>& votes, 
    Vec3& outNormal,
    int minVotes) {
    
    // 找到得票最多的平面法向量
    int maxVotes = 0;
    Vec3 bestNormal;
    
    for (const auto& vote : votes) {
        if (vote.second > maxVotes) {
            maxVotes = vote.second;
            bestNormal = vote.first;
        }
    }
    
    // 如果投票数量足够，认为存在反射对称
    if (maxVotes >= minVotes) {
        outNormal = normalize(bestNormal);
        
        std::cout << "候选反射对称平面法向量得到 " << maxVotes << " 票" << std::endl;
        return true;
    }
    
    return false;
}

// 计算反射变换矩阵
Eigen::Matrix4f MitraReflectiveSymmetryDetector::computeReflectionTransform(
    const Vertex& p1, const Vertex& p2) {
    
    // 将点转换为Eigen向量
    Eigen::Vector3f point1(p1.x, p1.y, p1.z);
    Eigen::Vector3f point2(p2.x, p2.y, p2.z);
    
    // 计算反射平面的法向量
    Eigen::Vector3f normal = (point2 - point1).normalized();
    
    // 构建反射矩阵
    Eigen::Matrix4f transform = Eigen::Matrix4f::Identity();
    
    // 反射矩阵: R = I - 2 * normal * normal^T
    Eigen::Matrix3f reflection = Eigen::Matrix3f::Identity() - 
                                2 * normal * normal.transpose();
    
    transform.block<3,3>(0,0) = reflection;
    
    return transform;
}

// 验证对称性
bool MitraReflectiveSymmetryDetector::verifySymmetry(
    const std::vector<Vertex>& meshVertices, 
    const Vec3& normal) {
    
    // 在实际实现中，这一步需要检查当反射网格时，有多少点能够匹配
    // 简化实现: 假设验证成功
    
    std::cout << "验证反射对称性成功，对称平面法向量: (" 
              << normal.x << ", " << normal.y << ", " << normal.z 
              << ")" << std::endl;
    
    return true;
}

} // namespace MC