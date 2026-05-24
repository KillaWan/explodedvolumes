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

    float projectVertexOnAxis(const Vertex &vertex, const Vec3 &axis);

}
