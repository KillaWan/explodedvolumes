#include "planes/cutting_planes.h"
#include <algorithm>
#include <iostream>
#include <vector>
#include <glad/glad.h>
#include <omp.h>
#ifdef _WIN32
#include <mutex>
#endif

namespace MC
{

    // calculate the distance between point and plane
    float pointToPlaneDistance(const Vertex &v, const CuttingPlane &plane)
    {
        return (v.x - plane.origin.x) * plane.normal.x +
               (v.y - plane.origin.y) * plane.normal.y +
               (v.z - plane.origin.z) * plane.normal.z;
    }

    // calclute linear interpolation between two points
    Vertex interpolateVertex(const Vertex &v1, const Vertex &v2, float t)
    {
        Vertex result;
        result.x = v1.x + t * (v2.x - v1.x);
        result.y = v1.y + t * (v2.y - v1.y);
        result.z = v1.z + t * (v2.z - v1.z);
        return result;
    }

    // calculate the projection of the vertex on the explode axis
    float projectVertexOnAxis(const Vertex &vertex, const Vec3 &axis)
    {
        return vertex.x * axis.x + vertex.y * axis.y + vertex.z * axis.z;
    }


    bool trianglePlaneIntersection(
        const Vertex &v0, const Vertex &v1, const Vertex &v2,
        const CuttingPlane &plane,
        IntersectionSegment &segment)
    {

        float d0 = pointToPlaneDistance(v0, plane);
        float d1 = pointToPlaneDistance(v1, plane);
        float d2 = pointToPlaneDistance(v2, plane);

        // if all points are on the same side of the plane, then there is no intersection
        if ((d0 > 0 && d1 > 0 && d2 > 0) || (d0 < 0 && d1 < 0 && d2 < 0))
        {
            return false;
        }

        bool v0OnPlane = std::abs(d0) < 1e-6f;
        bool v1OnPlane = std::abs(d1) < 1e-6f;
        bool v2OnPlane = std::abs(d2) < 1e-6f;

        std::vector<Vertex> intersections;

        // Check intersection
        if (!v0OnPlane && !v1OnPlane && d0 * d1 <= 0.0f)
        {
            // v0-v1 intersects
            float t = d0 / (d0 - d1);
            intersections.push_back(interpolateVertex(v0, v1, t));
        }

        if (!v1OnPlane && !v2OnPlane && d1 * d2 <= 0.0f)
        {
            float t = d1 / (d1 - d2);
            intersections.push_back(interpolateVertex(v1, v2, t));
        }

        if (!v2OnPlane && !v0OnPlane && d2 * d0 <= 0.0f)
        {
            float t = d2 / (d2 - d0);
            intersections.push_back(interpolateVertex(v2, v0, t));
        }

        // if the vertex is on a plane
        if (v0OnPlane)
        {
            intersections.push_back(v0);
        }
        if (v1OnPlane)
        {
            intersections.push_back(v1);
        }
        if (v2OnPlane)
        {
            intersections.push_back(v2);
        }

        // remove duplicate intersections
        if (intersections.size() > 1)
        {
            for (size_t i = 0; i < intersections.size(); i++)
            {
                for (size_t j = i + 1; j < intersections.size();)
                {
                    float dx = intersections[i].x - intersections[j].x;
                    float dy = intersections[i].y - intersections[j].y;
                    float dz = intersections[i].z - intersections[j].z;
                    float distSq = dx * dx + dy * dy + dz * dz;

                    if (distSq < 1e-10f)
                    {
                        intersections.erase(intersections.begin() + j);
                    }
                    else
                    {
                        j++;
                    }
                }
            }
        }

        if (intersections.size() >= 2)
        {
            segment.start = intersections[0];
            segment.end = intersections[1];
            return true;
        }
        else if (intersections.size() == 1)
        {
            segment.start = intersections[0];
            segment.end = intersections[0];
         
            segment.end.x += 0.0001f;
            segment.end.y += 0.0001f;
            segment.end.z += 0.0001f;
            return true;
        }

        return false;
    }


    PlaneIntersection computePlaneIntersections(
        const Mesh &mesh,
        const std::vector<CuttingPlane> &planes)
    {
        PlaneIntersection intersection;
        intersection.visible = false;

        // Check valid
        if (mesh.vertices.empty() || mesh.indices.empty())
        {
            std::cerr << "Empty mesh, cannot compute plane intersections" << std::endl;
            return intersection;
        }

        std::vector<IntersectionSegment> threadSafeSegments;
        std::mutex segmentMutex;

#pragma omp parallel for
        for (size_t planeIdx = 0; planeIdx < planes.size(); planeIdx++)
        {
            const CuttingPlane &plane = planes[planeIdx];
            std::vector<IntersectionSegment> localSegments;

            // Iterate over triangles
            for (size_t i = 0; i < mesh.indices.size(); i += 3)
            {
                // Get vertices
                const Vertex &v0 = mesh.vertices[mesh.indices[i]];
                const Vertex &v1 = mesh.vertices[mesh.indices[i + 1]];
                const Vertex &v2 = mesh.vertices[mesh.indices[i + 2]];

                // Check intersection
                IntersectionSegment segment;
                if (trianglePlaneIntersection(v0, v1, v2, plane, segment))
                {
                    segment.planeIndex = planeIdx;
                    localSegments.push_back(segment);
                }
            }

// Merge safely
#pragma omp critical
            {
                threadSafeSegments.insert(
                    threadSafeSegments.end(),
                    localSegments.begin(),
                    localSegments.end());
            }
        }

        intersection.segments = std::move(threadSafeSegments);

        std::cout << "Computed " << intersection.segments.size()
                  << " intersection segments for " << planes.size() << " planes" << std::endl;

        return intersection;
    }

    // VAO and VBO
    void setupIntersectionVAO(
        const PlaneIntersection &intersection,
        unsigned int &VAO,
        unsigned int &VBO)
    {

        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);

        glBindVertexArray(VAO);

        std::vector<Vertex> lineVertices;
        for (const auto &segment : intersection.segments)
        {
            lineVertices.push_back(segment.start);
            lineVertices.push_back(segment.end);
        }

        // Bind and buffer
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, lineVertices.size() * sizeof(Vertex),
                     lineVertices.data(), GL_STATIC_DRAW);

        // set attr
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)0);
        glEnableVertexAttribArray(0);

        glBindVertexArray(0);
    }

    // update intersection VAO
    void updateIntersectionVAO(
        const PlaneIntersection &intersection,
        unsigned int &VAO,
        unsigned int &VBO)
    {

        // set all line vertices in one array
        std::vector<Vertex> lineVertices;
        for (const auto &segment : intersection.segments)
        {
            lineVertices.push_back(segment.start);
            lineVertices.push_back(segment.end);
        }

        // update VBO
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, lineVertices.size() * sizeof(Vertex),
                     lineVertices.data(), GL_STATIC_DRAW);
    }

} // namespace MC
