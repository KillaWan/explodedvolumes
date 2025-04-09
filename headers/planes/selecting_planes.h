#pragma once

#include "data.h"
#include <vector>

namespace MC
{
    struct CuttingPlane
    {
        Vec3 origin; 
        Vec3 normal;
        float distance;
    };

    std::vector<CuttingPlane> generateUniformCuttingPlanes(
        const Mesh &mesh,
        const Vec3 &explosionAxis,
        int numPlanes = 5);

    std::vector<CuttingPlane> generateAdaptiveCuttingPlanes(
        const Mesh &mesh,
        const Vec3 &explosionAxis,
        int numPlanes = 5);

    bool shouldKeepCuttingPlane(const CuttingPlane &plane, const Mesh &mesh);

}