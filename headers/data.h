#pragma once

#include <iostream>
#include <vector>
#include <cmath>
#include <string>
#include <memory>
#include <limits>
#include <filesystem>

#include "nifti1_io.h"

#ifdef _WIN32
#ifndef M_PI
#define M_PI 3.14159265358979323846264338327950288
#endif
#endif

namespace MC
{
    typedef unsigned int IndexType;

    struct Vec3
    {
        float x, y, z;
        Vec3() : x(0), y(0), z(0) {}
        Vec3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}
        bool operator<(const Vec3& other) const {
            if (x != other.x) return x < other.x;
            if (y != other.y) return y < other.y;
            return z < other.z;
        }
    };

    struct Vertex
    {
        float x, y, z;
    };

    struct Camera
    {
        float rotationX = 15.0f;  
        float rotationY = -45.0f; 
        float distance = 5.0f;
        float zoom = 10.0f; 
    };

    struct VolumeData
    {
        std::unique_ptr<float[]> data;
        int dims[3];
        float minValue;
        float maxValue;
        VolumeData() : data(nullptr)
        {
            dims[0] = dims[1] = dims[2] = 0;
            minValue = maxValue = 0.0f;
        }
    };

    struct Mesh
    {
        std::vector<Vertex> vertices;
        std::vector<IndexType> indices;
        Vec3 min_bounds;
        Vec3 max_bounds;
        Vec3 center;
        float max_dimension;
        Mesh() : max_dimension(0.0f) {}
    };

    float getVolumeValue(const float *data, int x, int y, int z, const int dims[3]);
    void calculateMeshBounds(const std::vector<Vertex> &vertices, Vec3 &min_bounds, Vec3 &max_bounds, Vec3 &center);
    // NIfTI file
    bool loadNiiFile(const std::string &filename, VolumeData &volume);
    std::string openNiftiFileDialog();

} 