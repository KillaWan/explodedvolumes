#pragma once

#include "data.h"

namespace MC
{

    // Lookup tables for Marching Cubes algorithm
    extern const Vec3 cubeCornerOffsets[8];
    extern const int edgeTable[256];
    extern const int triTable[256][16];

    // Linear interpolation function
    Vec3 interpolate(float isoLevel, const Vec3 &p1, const Vec3 &p2, float valp1, float valp2);

    // Process a single cube in the marching cubes algorithm
    void polygoniseCube(const float *volume, const int dims[3],
                        int cellX, int cellY, int cellZ,
                        float isoLevel,
                        std::vector<Vec3> &triangleVerts);

    // Generate mesh using marching cubes algorithm
    void generateMesh(const VolumeData &volume, float isoLevel, Mesh &mesh);

} // namespace MC