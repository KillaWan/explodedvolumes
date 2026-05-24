#include "planes/selecting_planes.h"
#include <algorithm>
#include <iostream>
#include <vector>
#include <cmath>
#include <limits>
#include <array>

namespace MC
{
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
