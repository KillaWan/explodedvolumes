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

    Vec3 computeDominantEigenvector(const Matrix3x3 &matrix, int maxIterations = 20)
    {
        // 1 1 1
        Vec3 v = {1.0f, 1.0f, 1.0f};
        v = normalize(v);

        // Iteration
        for (int i = 0; i < maxIterations; i++)
        {
            Vec3 new_v = matrix.multiply(v);

            float length = std::sqrt(new_v.x * new_v.x + new_v.y * new_v.y + new_v.z * new_v.z);
            if (length < 1e-6f)
            {
                break;
            }

            new_v.x /= length;
            new_v.y /= length;
            new_v.z /= length;

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

    void computeOrthogonalAxes(Vec3 &axis1, Vec3 &axis2, Vec3 &axis3)
    {
        axis1 = normalize(axis1);

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

        axis2 = normalize(cross(axis1, temp));
        axis3 = normalize(cross(axis1, axis2));
    }

    OBB computeOBB(const Mesh &mesh)
    {
        OBB obb;

        if (mesh.vertices.empty())
        {
            obb.center = {0, 0, 0};
            obb.axes[0] = {1, 0, 0};
            obb.axes[1] = {0, 1, 0};
            obb.axes[2] = {0, 0, 1};
            obb.extents = {0, 0, 0};
            return obb;
        }

        // Centre of gravity
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

        // covMatrix
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

        // Normalization
        for (int i = 0; i < 3; i++)
        {
            for (int j = 0; j < 3; j++)
            {
                covMatrix.m[i][j] /= mesh.vertices.size();
            }
        }

        // Dominant Eigenvector
        obb.axes[0] = computeDominantEigenvector(covMatrix);

        computeOrthogonalAxes(obb.axes[0], obb.axes[1], obb.axes[2]);

        // Center
        obb.center = mean;

        // Init
        obb.extents = {0.0f, 0.0f, 0.0f};

        // Calculate range
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

    // Adaptive Cutting Planes
    std::vector<CuttingPlane> generateAdaptiveCuttingPlanes(
        const Mesh &mesh,
        const Vec3 &explosionAxis,
        int numPlanes)
    {
        std::vector<CuttingPlane> planes;

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

        // Calculate OBB for model
        OBB obb = computeOBB(mesh);

        // Directly use normalized exploded axis
        // TODO Using main axis of OBB to improve
        Vec3 cuttingAxis = normalizedAxis;

        // Calculate projection range
        float minProj = std::numeric_limits<float>::max();
        float maxProj = std::numeric_limits<float>::lowest();
        std::vector<float> projections;

        for (const auto &vertex : mesh.vertices)
        {
            // Calculate projection
            float proj = dot({vertex.x, vertex.y, vertex.z}, cuttingAxis);

            projections.push_back(proj);
            minProj = std::min(minProj, proj);
            maxProj = std::max(maxProj, proj);
        }

        // Sort values
        std::sort(projections.begin(), projections.end());

        std::vector<float> cutPositions;

        for (int i = 1; i <= numPlanes; ++i)
        {
            // Set cutting point according to the percent
            size_t index = (i * projections.size()) / (numPlanes + 1);
            if (index < projections.size())
            {
                cutPositions.push_back(projections[index]);
            }
        }

        // Create cutting plane
        for (const float &position : cutPositions)
        {
            CuttingPlane plane;
            plane.normal = cuttingAxis;
            plane.distance = position;

            plane.origin.x = mesh.center.x + cuttingAxis.x * (position - (minProj + maxProj) / 2.0f);
            plane.origin.y = mesh.center.y + cuttingAxis.y * (position - (minProj + maxProj) / 2.0f);
            plane.origin.z = mesh.center.z + cuttingAxis.z * (position - (minProj + maxProj) / 2.0f);

            planes.push_back(plane);
        }

        std::cout << "Generated " << planes.size() << " cutting planes perpendicular to explosion axis" << std::endl;

        return planes;
    }

} // namespace MC