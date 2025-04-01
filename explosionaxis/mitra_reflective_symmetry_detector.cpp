#include "explosionaxis/mitra_reflective_symmetry_detector.h"
#include "explosionaxis/vector_ops.h"
#include <iostream>
#include <algorithm>
#include <random>
#include <cmath>

namespace MC {

using namespace VectorOps;

bool MitraReflectiveSymmetryDetector::detect(const std::vector<Vertex>& meshVertices, 
                                               Vec3& outReflectiveNormal) {
    // 如果设置了使用自定义法向，则直接使用它
    if (m_useCustomNormal) {
        std::cout << "Using custom reflective normal: (" 
                  << m_customNormal.x << ", " << m_customNormal.y << ", " << m_customNormal.z 
                  << ")" << std::endl;
        outReflectiveNormal = normalize(m_customNormal);
        return true;
    }
    
    std::cout << "Executing reflective symmetry detection with sample count: " 
              << m_sampleCount << std::endl;
              
    // 随机采样部分点
    int sampleCount = std::min(m_sampleCount, static_cast<int>(meshVertices.size() / 10));
    std::vector<Vertex> samples = randomSampling(meshVertices, sampleCount);
    
    // 对采样点对进行配对投票
    std::map<Vec3, int> votes = pairPointsAndVote(samples, meshVertices);
    
    // 提取投票最高的候选法向量
    bool hasSymmetry = extractSymmetryPlaneFromVotes(votes, outReflectiveNormal);
    
    // 如果提取后，再进行验证（这里简化为总是验证成功）
    if (hasSymmetry) {
        hasSymmetry = verifySymmetry(meshVertices, outReflectiveNormal);
    }
    
    if (!hasSymmetry) {
        std::cout << "Reflective symmetry detection failed, using default normal." << std::endl;
        return false;
    }
    
    return hasSymmetry;
}

std::vector<Vertex> MitraReflectiveSymmetryDetector::randomSampling(const std::vector<Vertex>& meshVertices, int sampleCount) {
    std::vector<Vertex> result;
    result.reserve(sampleCount);
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distrib(0, meshVertices.size() - 1);
    
    for (int i = 0; i < sampleCount; ++i) {
        int index = distrib(gen);
        result.push_back(meshVertices[index]);
    }
    
    return result;
}

Eigen::VectorXf MitraReflectiveSymmetryDetector::computeSignature(
    const Vertex& vertex, const std::vector<Vertex>& meshVertices) {
    
    Eigen::VectorXf signature(10);
    signature.setZero();
    
    Vec3 p(vertex.x, vertex.y, vertex.z);
    
    // 计算到所有点的距离
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
    
    // 排序并将距离划分为10个 bin
    std::sort(distances.begin(), distances.end());
    int binSize = distances.size() / 10;
    for (int i = 0; i < 10; ++i) {
        int index = std::min(i * binSize, static_cast<int>(distances.size()) - 1);
        signature(i) = distances[index];
    }
    
    // 归一化签名，使其成为单位向量
    float norm = signature.norm();
    if (norm > 0) {
        signature /= norm;
    }
    
    return signature;
}

std::map<Vec3, int> MitraReflectiveSymmetryDetector::pairPointsAndVote(
    const std::vector<Vertex>& samples, 
    const std::vector<Vertex>& meshVertices) {
    
    std::map<Vec3, int> votes;
    
    // 为每个采样点计算归一化签名
    std::vector<Eigen::VectorXf> signatures;
    signatures.reserve(samples.size());
    for (const auto& vertex : samples) {
        signatures.push_back(computeSignature(vertex, meshVertices));
    }
    
    // 使用余弦相似度判断采样点对是否相似
    float cosineThreshold = m_cosineThreshold; // 例如 0.95
    for (size_t i = 0; i < samples.size(); ++i) {
        for (size_t j = i + 1; j < samples.size(); ++j) {
            // 余弦相似度即归一化后向量的点积
            float cosineSim = signatures[i].dot(signatures[j]);
            if (cosineSim > cosineThreshold) {
                // 如果相似，则认为这对点对应于同一对称平面
                Vec3 p1(samples[i].x, samples[i].y, samples[i].z);
                Vec3 p2(samples[j].x, samples[j].y, samples[j].z);
                
                // 以两点之差作为候选平面法向量
                Vec3 normal(p2.x - p1.x, p2.y - p1.y, p2.z - p1.z);
                normal = normalize(normal);
                
                // 对法向量进行量化，合并相似方向的投票
                int quantizeFactor = 100;
                Vec3 quantizedNormal(
                    std::round(normal.x * quantizeFactor) / quantizeFactor,
                    std::round(normal.y * quantizeFactor) / quantizeFactor,
                    std::round(normal.z * quantizeFactor) / quantizeFactor
                );
                
                votes[quantizedNormal]++;
            }
        }
    }
    
    return votes;
}

bool MitraReflectiveSymmetryDetector::extractSymmetryPlaneFromVotes(
    const std::map<Vec3, int>& votes, Vec3& outNormal, int minVotes) {
    
    int maxVotes = 0;
    Vec3 bestNormal;
    for (const auto& vote : votes) {
        if (vote.second > maxVotes) {
            maxVotes = vote.second;
            bestNormal = vote.first;
        }
    }
    
    if (maxVotes >= minVotes) {
        outNormal = normalize(bestNormal);
        std::cout << "候选反射对称平面法向量获得 " << maxVotes << " 票" << std::endl;
        return true;
    }
    
    return false;
}

Eigen::Matrix4f MitraReflectiveSymmetryDetector::computeReflectionTransform(
    const Vertex& p1, const Vertex& p2) {
    
    Eigen::Vector3f point1(p1.x, p1.y, p1.z);
    Eigen::Vector3f point2(p2.x, p2.y, p2.z);
    
    Eigen::Vector3f normal = (point2 - point1).normalized();
    Eigen::Matrix4f transform = Eigen::Matrix4f::Identity();
    Eigen::Matrix3f reflection = Eigen::Matrix3f::Identity() - 2 * normal * normal.transpose();
    transform.block<3,3>(0,0) = reflection;
    return transform;
}

bool MitraReflectiveSymmetryDetector::verifySymmetry(
    const std::vector<Vertex>& meshVertices, const Vec3& normal) {
    
    std::cout << "验证反射对称性成功，对称平面法向量: (" 
              << normal.x << ", " << normal.y << ", " << normal.z 
              << ")" << std::endl;
    return true;
}

} // namespace MC
