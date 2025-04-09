#pragma once

#include "data.h"
#include "cutting_planes.h"
#include <vector>
#include <glm/glm.hpp>

// #include <vtkSmartPointer.h>
// #include <vtkPolyData.h>

namespace MC
{
    struct ExplodedSegment
    {
        std::vector<Vertex> vertices; 
        std::vector<IndexType> indices;
        Vec3 center;
        Vec3 displacement;
        unsigned int VAO = 0;
        unsigned int VBO = 0;
        unsigned int EBO = 0;

        // vtkSmartPointer<vtkPolyData> vtkPolyData;
    };

    struct ExplodedView
    {
        std::vector<ExplodedSegment> segments;
        bool enabled = false;
        float explosionDistance = 2.0f;
    };

    ExplodedView computeExplodedView(
        const Mesh &mesh,
        const std::vector<CuttingPlane> &planes,
        const Vec3 &explosionAxis,
        float explosionDistance = 35.0f);

    void updateExplodedViewDisplacements(
        ExplodedView &explodedView,
        const Vec3 &explosionAxis,
        float explosionDistance);

    void setupSegmentMesh(ExplodedSegment &segment);

    void updateSegmentMesh(ExplodedSegment &segment);

    void cleanupExplodedView(ExplodedView &explodedView);

} 