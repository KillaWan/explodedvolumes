#include "explosionaxis/pca_analyzer.h"
#include <Eigen/Dense>
#include <iostream>

namespace MC {

using namespace VectorOps;

// 主方法：分析模型并确定主轴方向
Vec3 PCAAnalyzer::analyzePrincipalAxis(const std::vector<Vertex>& meshVertices) {
    std::cout << "Executing PCA analysis, using " 
              << (m_useLongestAxis ? "longest" : "shortest") << " principal axis." << std::endl;
    
    Vec3 centroid = computeCentroid(meshVertices);
    Eigen::Matrix3f covMatrix = Eigen::Matrix3f::Zero();
    
    for (const auto& vertex : meshVertices) {
        Vec3 v(vertex.x - centroid.x, vertex.y - centroid.y, vertex.z - centroid.z);
        covMatrix(0, 0) += v.x * v.x;
        covMatrix(0, 1) += v.x * v.y;
        covMatrix(0, 2) += v.x * v.z;
        covMatrix(1, 0) += v.y * v.x;
        covMatrix(1, 1) += v.y * v.y;
        covMatrix(1, 2) += v.y * v.z;
        covMatrix(2, 0) += v.z * v.x;
        covMatrix(2, 1) += v.z * v.y;
        covMatrix(2, 2) += v.z * v.z;
    }
    
    covMatrix /= meshVertices.size();
    
    Eigen::SelfAdjointEigenSolver<Eigen::Matrix3f> solver(covMatrix);
    
    // 根据设置选择最长或最短主轴
    int eigenVecIndex = m_useLongestAxis ? 2 : 0; // 2为最长轴，0为最短轴
    Eigen::Vector3f eigenVec = solver.eigenvectors().col(eigenVecIndex);
    
    Vec3 axis(eigenVec(0), eigenVec(1), eigenVec(2));
    std::cout << "Selected principal axis: (" << axis.x << ", " << axis.y << ", " << axis.z << ")" << std::endl;
    
    return normalize(axis);
}

// 3D PCA：计算协方差矩阵，并返回第一主成分
Vec3 PCAAnalyzer::compute3DPCA(const std::vector<Vertex>& meshVertices) {
    std::cout << "Execute 3D PCA analyzation." << std::endl;
    
    Vec3 centroid = computeCentroid(meshVertices);
    Eigen::Matrix3f covMatrix = Eigen::Matrix3f::Zero();
    
    for (const auto& vertex : meshVertices) {
        Vec3 v(vertex.x - centroid.x, vertex.y - centroid.y, vertex.z - centroid.z);
        covMatrix(0, 0) += v.x * v.x;
        covMatrix(0, 1) += v.x * v.y;
        covMatrix(0, 2) += v.x * v.z;
        covMatrix(1, 0) += v.y * v.x;
        covMatrix(1, 1) += v.y * v.y;
        covMatrix(1, 2) += v.y * v.z;
        covMatrix(2, 0) += v.z * v.x;
        covMatrix(2, 1) += v.z * v.y;
        covMatrix(2, 2) += v.z * v.z;
    }
    
    covMatrix /= meshVertices.size();
    
    Eigen::SelfAdjointEigenSolver<Eigen::Matrix3f> solver(covMatrix);
    Eigen::Vector3f eigenVec = solver.eigenvectors().col(2);
    return normalize(Vec3(eigenVec(0), eigenVec(1), eigenVec(2)));
}

// 2D PCA：将顶点投影到指定平面，然后在该平面上计算第一主成分，最后映射回3D
Vec3 PCAAnalyzer::compute2DPCAOnPlane(const std::vector<Vertex>& meshVertices, const Vec3& planeNormal) {
    std::cout << "Execute 2D PCA analyzation." << std::endl;
    
    Vec3 n = normalize(planeNormal);
    Vec3 centroid = computeCentroid(meshVertices);
    
    std::vector<Vec3> projectedPoints;
    projectedPoints.reserve(meshVertices.size());
    for (const auto& vertex : meshVertices) {
        Vec3 point(vertex.x, vertex.y, vertex.z);
        Vec3 proj = projectPointOntoPlane(point, n, centroid);
        projectedPoints.push_back(proj);
    }
    
    Vec3 projectedCentroid(0, 0, 0);
    for (const auto& p : projectedPoints) {
        projectedCentroid.x += p.x;
        projectedCentroid.y += p.y;
        projectedCentroid.z += p.z;
    }
    float count = static_cast<float>(projectedPoints.size());
    if (count > 0) {
        projectedCentroid.x /= count;
        projectedCentroid.y /= count;
        projectedCentroid.z /= count;
    }
    
    // 建立局部二维坐标系：选一个与 n 垂直的向量作为 u
    Vec3 u;
    if (std::abs(n.x) < std::abs(n.y) && std::abs(n.x) < std::abs(n.z))
        u = normalize(cross(Vec3(1, 0, 0), n));
    else
        u = normalize(cross(Vec3(0, 1, 0), n));
    Vec3 v = cross(n, u);
    
    std::vector<Eigen::Vector2f> points2D;
    points2D.reserve(projectedPoints.size());
    for (const auto& p : projectedPoints) {
        Vec3 rel = Vec3(p.x - projectedCentroid.x, p.y - projectedCentroid.y, p.z - projectedCentroid.z);
        float u_coord = dot(rel, u);
        float v_coord = dot(rel, v);
        points2D.push_back(Eigen::Vector2f(u_coord, v_coord));
    }
    
    Eigen::Matrix2f cov2D = Eigen::Matrix2f::Zero();
    for (const auto& p : points2D) {
        cov2D(0, 0) += p(0) * p(0);
        cov2D(0, 1) += p(0) * p(1);
        cov2D(1, 0) += p(1) * p(0);
        cov2D(1, 1) += p(1) * p(1);
    }
    cov2D /= points2D.size();
    
    Eigen::SelfAdjointEigenSolver<Eigen::Matrix2f> solver2D(cov2D);
    Eigen::Vector2f eigenVec2D = solver2D.eigenvectors().col(1);
    
    Vec3 result(
        eigenVec2D(0) * u.x + eigenVec2D(1) * v.x,
        eigenVec2D(0) * u.y + eigenVec2D(1) * v.y,
        eigenVec2D(0) * u.z + eigenVec2D(1) * v.z
    );
    return normalize(result);
}

} // namespace MC