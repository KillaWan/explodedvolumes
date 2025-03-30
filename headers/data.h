#pragma once

#include <iostream>
#include <vector>
#include <cmath>
#include <string>
#include <memory>
#include <limits>
#include <filesystem>

#include "nifti1_io.h"

namespace MC
{

    // 3D vector for internal calculations
    struct Vec3
    {
        float x, y, z;
        Vec3() : x(0), y(0), z(0) {}
        Vec3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}
    };

    // Mesh vertex
    struct Vertex
    {
        float x, y, z;
    };

    // Triangle index type
    typedef unsigned int IndexType;

    // Camera parameters
    struct Camera
    {
        float rotationX = 15.0f;  // Initial tilt down
        float rotationY = -45.0f; // Initial rotation for better view
        float distance = 5.0f;
        float zoom = 10.0f; // Initial zoom for larger appearance
    };

    // Volume data representation
    struct VolumeData
    {
        std::unique_ptr<float[]> data;
        int dims[3];
        float minValue;
        float maxValue;

        // Default constructor
        VolumeData() : data(nullptr)
        {
            dims[0] = dims[1] = dims[2] = 0;
            minValue = maxValue = 0.0f;
        }
    };

    // Mesh representation
    struct Mesh
    {
        std::vector<Vertex> vertices;
        std::vector<IndexType> indices;
        Vec3 min_bounds;
        Vec3 max_bounds;
        Vec3 center;
        float max_dimension;

        // Default constructor
        Mesh() : max_dimension(0.0f) {}
    };

    // 实用函数
    float getVolumeValue(const float *data, int x, int y, int z, const int dims[3]);
    void calculateMeshBounds(const std::vector<Vertex> &vertices, Vec3 &min_bounds, Vec3 &max_bounds, Vec3 &center);

    // NIfTI文件相关函数
    bool loadNiiFile(const std::string &filename, VolumeData &volume);
    std::string openNiftiFileDialog();

} // namespace MC