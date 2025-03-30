#include "explosionaxis/symmetry_axis.h"
#include <cmath>
#include <iostream>
#include <algorithm>
// Remove problematic includes
// #include <map>
#include <Eigen/Dense>

namespace MC
{

    // ---------- 辅助函数 ---------- //

    // 计算所有顶点的质心
    Vec3 computeCentroid(const std::vector<Vertex> &meshVertices)
    {
        Vec3 centroid(0, 0, 0);
        for (const auto &v : meshVertices)
        {
            centroid.x += v.x;
            centroid.y += v.y;
            centroid.z += v.z;
        }
        float count = static_cast<float>(meshVertices.size());
        if (count > 0)
        {
            centroid.x /= count;
            centroid.y /= count;
            centroid.z /= count;
        }
        return centroid;
    }

    // 将点投影到由平面法向量和平面上一点定义的平面上
    Vec3 projectPointOntoPlane(const Vec3 &point, const Vec3 &planeNormal, const Vec3 &pointOnPlane)
    {
        Vec3 n = normalize(planeNormal);
        float d = dot(n, Vec3(point.x - pointOnPlane.x, point.y - pointOnPlane.y, point.z - pointOnPlane.z));
        return Vec3(point.x - d * n.x, point.y - d * n.y, point.z - d * n.z);
    }

    // ---------- 对称性检测 ---------- //

    // 检测旋转对称性的简化实现
    SymmetryResult detectRotationalSymmetry(const std::vector<Vertex> &meshVertices)
    {
        SymmetryResult result;
        result.type = SymmetryType::None;
        result.symmetryOrder = 1;
        result.rotationAxis = Vec3(0, 0, 1);

        // 采用 3D PCA 得到主轴
        Vec3 pcaAxis = pca3D(meshVertices);
        // 这里作为简化策略：如果 PCA 结果与 Z 轴近似对齐，则认为存在旋转对称性
        Vec3 zAxis(0, 0, 1);
        float cosine = std::abs(dot(normalize(pcaAxis), zAxis));
        // 如果夹角小于约 25°（cosine > 0.9），则认为存在旋转对称性；同时设定对称阶数为 4（示例）
        if (cosine > 0.9f)
        {
            result.type = SymmetryType::Rotational;
            result.symmetryOrder = 4;
            result.rotationAxis = pcaAxis;
        }
        return result;
    }

    // 检测镜面对称性的简化实现
    SymmetryResult detectReflectiveSymmetry(const std::vector<Vertex> &meshVertices)
    {
        SymmetryResult result;
        result.type = SymmetryType::None;
        result.symmetryOrder = 1;
        result.reflectiveNormal = Vec3(0, 1, 0);

        Vec3 centroid = computeCentroid(meshVertices);
        // 简单判断：如果模型中心的 Y 坐标接近 0，则认为存在镜面对称（关于 XZ 平面）
        if (std::abs(centroid.y) < 1e-3f)
        {
            result.type = SymmetryType::Reflective;
            result.reflectiveNormal = Vec3(0, 1, 0);
        }
        return result;
    }

    // 主对称性检测：先检测旋转对称，再检测镜面对称
    SymmetryResult detectSymmetry(const std::vector<Vertex> &meshVertices)
    {
        SymmetryResult rotational = detectRotationalSymmetry(meshVertices);
        if (rotational.type == SymmetryType::Rotational)
            return rotational;
        return detectReflectiveSymmetry(meshVertices);
    }

    // ---------- PCA 实现 ---------- //

    // 3D PCA：计算协方差矩阵，并返回第一主成分
    Vec3 pca3D(const std::vector<Vertex> &meshVertices)
    {
        Vec3 centroid = computeCentroid(meshVertices);
        Eigen::Matrix3f covMatrix = Eigen::Matrix3f::Zero();

        for (const auto &vertex : meshVertices)
        {
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

        if (meshVertices.size() > 0)
        {
            covMatrix /= meshVertices.size();
        }

        Eigen::SelfAdjointEigenSolver<Eigen::Matrix3f> solver(covMatrix);
        Eigen::Vector3f eigenVec = solver.eigenvectors().col(2);
        return Vec3(eigenVec(0), eigenVec(1), eigenVec(2));
    }

    // 2D PCA：将顶点投影到指定平面，然后在该平面上计算第一主成分，最后映射回 3D
    Vec3 pca2DOnPlane(const std::vector<Vertex> &meshVertices, const Vec3 &planeNormal)
    {
        Vec3 n = normalize(planeNormal);
        Vec3 centroid = computeCentroid(meshVertices);

        std::vector<Vec3> projectedPoints;
        projectedPoints.reserve(meshVertices.size());
        for (const auto &vertex : meshVertices)
        {
            Vec3 point(vertex.x, vertex.y, vertex.z);
            Vec3 proj = projectPointOntoPlane(point, n, centroid);
            projectedPoints.push_back(proj);
        }

        Vec3 projectedCentroid(0, 0, 0);
        for (const auto &p : projectedPoints)
        {
            projectedCentroid.x += p.x;
            projectedCentroid.y += p.y;
            projectedCentroid.z += p.z;
        }
        float count = static_cast<float>(projectedPoints.size());
        if (count > 0)
        {
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
        for (const auto &p : projectedPoints)
        {
            Vec3 rel = Vec3(p.x - projectedCentroid.x, p.y - projectedCentroid.y, p.z - projectedCentroid.z);
            float u_coord = dot(rel, u);
            float v_coord = dot(rel, v);
            points2D.push_back(Eigen::Vector2f(u_coord, v_coord));
        }

        Eigen::Matrix2f cov2D = Eigen::Matrix2f::Zero();
        for (const auto &p : points2D)
        {
            cov2D(0, 0) += p(0) * p(0);
            cov2D(0, 1) += p(0) * p(1);
            cov2D(1, 0) += p(1) * p(0);
            cov2D(1, 1) += p(1) * p(1);
        }
        if (points2D.size() > 0)
        {
            cov2D /= points2D.size();
        }

        Eigen::SelfAdjointEigenSolver<Eigen::Matrix2f> solver2D(cov2D);
        Eigen::Vector2f eigenVec2D = solver2D.eigenvectors().col(1);

        Vec3 result(
            eigenVec2D(0) * u.x + eigenVec2D(1) * v.x,
            eigenVec2D(0) * u.y + eigenVec2D(1) * v.y,
            eigenVec2D(0) * u.z + eigenVec2D(1) * v.z);
        return normalize(result);
    }

    // ---------- 主函数: computeExplosionAxis ---------- //
    Vec3 computeExplosionAxis(const std::vector<Vertex> &meshVertices, const Vec3 *userDefinedAxis)
    {
        if (userDefinedAxis)
        {
            return normalize(*userDefinedAxis);
        }

        SymmetryResult sym = detectSymmetry(meshVertices);
        if (sym.type == SymmetryType::Rotational)
        {
            std::cout << "Using rotational symmetry axis (order " << sym.symmetryOrder << ")\n";
            return normalize(sym.rotationAxis);
        }
        else if (sym.type == SymmetryType::Reflective)
        {
            std::cout << "Using 2D PCA on reflection plane\n";
            return normalize(pca2DOnPlane(meshVertices, sym.reflectiveNormal));
        }
        else
        {
            std::cout << "No symmetry detected, using 3D PCA\n";
            return normalize(pca3D(meshVertices));
        }
    }

} // namespace MC