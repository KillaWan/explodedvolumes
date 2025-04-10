#include "planes/selecting_planes.h"
#include <algorithm>
#include <iostream>
#include <vector>
#include <cmath>
#include <limits>
#include <array>

namespace MC
{
    // 3x3 matrix
    struct Matrix3x3
    {
        float m[3][3] = {{0}};

        // Basic operation
        void zero()
        {
            for (int i = 0; i < 3; i++)
            {
                for (int j = 0; j < 3; j++)
                {
                    m[i][j] = 0.0f;
                }
            }
        }

        // Mul vec
        Vec3 multiply(const Vec3 &v) const
        {
            Vec3 result;
            result.x = m[0][0] * v.x + m[0][1] * v.y + m[0][2] * v.z;
            result.y = m[1][0] * v.x + m[1][1] * v.y + m[1][2] * v.z;
            result.z = m[2][0] * v.x + m[2][1] * v.y + m[2][2] * v.z;
            return result;
        }
    };

    // Normalize
    Vec3 normalize(const Vec3 &v)
    {
        float length = std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
        if (length > 1e-6f)
        {
            return {v.x / length, v.y / length, v.z / length};
        }
        return {0.0f, 0.0f, 0.0f};
    }

    // cross
    Vec3 cross(const Vec3 &a, const Vec3 &b)
    {
        Vec3 result;
        result.x = a.y * b.z - a.z * b.y;
        result.y = a.z * b.x - a.x * b.z;
        result.z = a.x * b.y - a.y * b.x;
        return result;
    }

    // dot
    float dot(const Vec3 &a, const Vec3 &b)
    {
        return a.x * b.x + a.y * b.y + a.z * b.z;
    }

    // OBB
    struct OBB
    {
        Vec3 center;
        Vec3 axes[3];
        Vec3 extents;
    };

    // 使用幂迭代法计算矩阵的主特征向量
    Vec3 computeDominantEigenvector(const Matrix3x3 &matrix, int maxIterations = 20)
    {
        // 初始向量 (1,1,1)
        Vec3 v = {1.0f, 1.0f, 1.0f};
        v = normalize(v);

        // 幂迭代
        for (int i = 0; i < maxIterations; i++)
        {
            // 应用矩阵
            Vec3 new_v = matrix.multiply(v);

            // 标准化
            float length = std::sqrt(new_v.x * new_v.x + new_v.y * new_v.y + new_v.z * new_v.z);
            if (length < 1e-6f)
            {
                break; // 收敛到零向量，可能是零矩阵
            }

            new_v.x /= length;
            new_v.y /= length;
            new_v.z /= length;

            // 检查收敛
            float dot_product = new_v.x * v.x + new_v.y * v.y + new_v.z * v.z;
            if (std::abs(std::abs(dot_product) - 1.0f) < 1e-6f)
            {
                v = new_v;
                break;
            }

            v = new_v;
        }

        return v;
    }

    // 计算正交的第三个轴
    void computeOrthogonalAxes(Vec3 &axis1, Vec3 &axis2, Vec3 &axis3)
    {
        // 确保轴1是单位向量
        axis1 = normalize(axis1);

        // 选择不与轴1平行的向量
        Vec3 temp;
        if (std::abs(axis1.x) < std::abs(axis1.y) && std::abs(axis1.x) < std::abs(axis1.z))
        {
            temp = {1.0f, 0.0f, 0.0f};
        }
        else if (std::abs(axis1.y) < std::abs(axis1.z))
        {
            temp = {0.0f, 1.0f, 0.0f};
        }
        else
        {
            temp = {0.0f, 0.0f, 1.0f};
        }

        // 计算正交轴
        axis2 = normalize(cross(axis1, temp));
        axis3 = normalize(cross(axis1, axis2));
    }

    // 计算OBB，不使用外部库
    OBB computeOBB(const Mesh &mesh)
    {
        OBB obb;

        // 如果网格为空，返回默认OBB
        if (mesh.vertices.empty())
        {
            obb.center = {0, 0, 0};
            obb.axes[0] = {1, 0, 0};
            obb.axes[1] = {0, 1, 0};
            obb.axes[2] = {0, 0, 1};
            obb.extents = {0, 0, 0};
            return obb;
        }

        // 计算网格重心
        Vec3 mean = {0, 0, 0};
        for (const auto &vertex : mesh.vertices)
        {
            mean.x += vertex.x;
            mean.y += vertex.y;
            mean.z += vertex.z;
        }
        mean.x /= mesh.vertices.size();
        mean.y /= mesh.vertices.size();
        mean.z /= mesh.vertices.size();

        // 构建协方差矩阵
        Matrix3x3 covMatrix;
        covMatrix.zero();

        for (const auto &vertex : mesh.vertices)
        {
            float dx = vertex.x - mean.x;
            float dy = vertex.y - mean.y;
            float dz = vertex.z - mean.z;

            covMatrix.m[0][0] += dx * dx;
            covMatrix.m[0][1] += dx * dy;
            covMatrix.m[0][2] += dx * dz;
            covMatrix.m[1][0] += dy * dx;
            covMatrix.m[1][1] += dy * dy;
            covMatrix.m[1][2] += dy * dz;
            covMatrix.m[2][0] += dz * dx;
            covMatrix.m[2][1] += dz * dy;
            covMatrix.m[2][2] += dz * dz;
        }

        // 归一化
        for (int i = 0; i < 3; i++)
        {
            for (int j = 0; j < 3; j++)
            {
                covMatrix.m[i][j] /= mesh.vertices.size();
            }
        }

        // 计算主特征向量（第一个轴）
        obb.axes[0] = computeDominantEigenvector(covMatrix);

        // 计算其他两个正交轴
        computeOrthogonalAxes(obb.axes[0], obb.axes[1], obb.axes[2]);

        // 设置中心点
        obb.center = mean;

        // 初始化范围
        obb.extents = {0.0f, 0.0f, 0.0f};

        // 计算在三个主轴上的投影范围
        for (const auto &vertex : mesh.vertices)
        {
            Vec3 diff = {
                vertex.x - mean.x,
                vertex.y - mean.y,
                vertex.z - mean.z};

            float projX = std::abs(dot(diff, obb.axes[0]));
            float projY = std::abs(dot(diff, obb.axes[1]));
            float projZ = std::abs(dot(diff, obb.axes[2]));

            obb.extents.x = std::max(obb.extents.x, projX);
            obb.extents.y = std::max(obb.extents.y, projY);
            obb.extents.z = std::max(obb.extents.z, projZ);
        }

        return obb;
    }

    // 自适应切割平面方法 - 使用简化的OBB，不依赖外部库
    std::vector<CuttingPlane> generateAdaptiveCuttingPlanes(
        const Mesh &mesh,
        const Vec3 &explosionAxis,
        int numPlanes)
    {
        std::vector<CuttingPlane> planes;

        // 归一化爆炸轴
        float axisLength = std::sqrt(
            explosionAxis.x * explosionAxis.x +
            explosionAxis.y * explosionAxis.y +
            explosionAxis.z * explosionAxis.z);

        Vec3 normalizedAxis;
        if (axisLength > 1e-6f)
        {
            normalizedAxis.x = explosionAxis.x / axisLength;
            normalizedAxis.y = explosionAxis.y / axisLength;
            normalizedAxis.z = explosionAxis.z / axisLength;
        }
        else
        {
            normalizedAxis.x = 0.0f;
            normalizedAxis.y = 0.0f;
            normalizedAxis.z = 1.0f;
        }

        // 计算模型的OBB
        OBB obb = computeOBB(mesh);

        // 直接使用标准化后的爆炸轴作为切割平面法线
        // 可选：这里可以使用OBB的主轴来改进切割方向
        Vec3 cuttingAxis = normalizedAxis;

        // 计算模型在切割轴（爆炸轴）上的投影范围
        float minProj = std::numeric_limits<float>::max();
        float maxProj = std::numeric_limits<float>::lowest();
        std::vector<float> projections;

        for (const auto &vertex : mesh.vertices)
        {
            // 计算点在法线方向上的投影
            float proj = dot({vertex.x, vertex.y, vertex.z}, cuttingAxis);

            projections.push_back(proj);
            minProj = std::min(minProj, proj);
            maxProj = std::max(maxProj, proj);
        }

        // 对投影值进行排序，用于后续自适应切割
        std::sort(projections.begin(), projections.end());

        // 自适应确定切割位置
        std::vector<float> cutPositions;

        // 根据顶点分布，选择切割位置
        for (int i = 1; i <= numPlanes; ++i)
        {
            // 根据顶点数量的百分比位置选择切割点
            size_t index = (i * projections.size()) / (numPlanes + 1);
            if (index < projections.size())
            {
                cutPositions.push_back(projections[index]);
            }
        }

        // 创建切割平面
        for (const float &position : cutPositions)
        {
            CuttingPlane plane;
            plane.normal = cuttingAxis; // 使用爆炸轴作为平面法线
            plane.distance = position;

            // 计算平面上的一点
            plane.origin.x = mesh.center.x + cuttingAxis.x * (position - (minProj + maxProj) / 2.0f);
            plane.origin.y = mesh.center.y + cuttingAxis.y * (position - (minProj + maxProj) / 2.0f);
            plane.origin.z = mesh.center.z + cuttingAxis.z * (position - (minProj + maxProj) / 2.0f);

            planes.push_back(plane);
        }

        std::cout << "Generated " << planes.size() << " cutting planes perpendicular to explosion axis" << std::endl;

        return planes;
    }

} // namespace MC