#pragma once

#include "data.h"
#include "selecting_planes.h"
#include <vector>

namespace MC
{
    struct IntersectionSegment
    {
        Vertex start;
        Vertex end;
        int planeIndex;
    };
    struct PlaneIntersection
    {
        std::vector<IntersectionSegment> segments;
        bool visible;
    };
    struct ModelSegment
    {
        int startIndex;
        int indexCount;
        Vec3 displacement;
    };

    PlaneIntersection computePlaneIntersections(
        const Mesh &mesh,
        const std::vector<CuttingPlane> &planes);

    bool trianglePlaneIntersection(
        const Vertex &v0, const Vertex &v1, const Vertex &v2,
        const CuttingPlane &plane,
        IntersectionSegment &segment);

    float pointToPlaneDistance(const Vertex &v, const CuttingPlane &plane);

    void setupIntersectionVAO(
        const PlaneIntersection &intersection,
        unsigned int &VAO,
        unsigned int &VBO);

    void updateIntersectionVAO(
        const PlaneIntersection &intersection,
        unsigned int &VAO,
        unsigned int &VBO);

    std::vector<ModelSegment> computeModelSegments(
        const Mesh &mesh,
        const std::vector<CuttingPlane> &planes,
        const Vec3 &explosionAxis,
        float explodeDistance);

    float projectVertexOnAxis(const Vertex &vertex, const Vec3 &axis);

    void setupExplodedMesh(
        const Mesh &mesh,
        const std::vector<ModelSegment> &segments,
        unsigned int &VAO,
        unsigned int &VBO,
        unsigned int &EBO);

    void updateExplosionState(
        const Mesh &mesh,
        const std::vector<CuttingPlane> &planes,
        const Vec3 &explosionAxis,
        float explodeDistance,
        bool explodeEnabled,
        std::vector<ModelSegment> &segments,
        unsigned int &VAO,
        unsigned int &VBO,
        unsigned int &EBO);

}