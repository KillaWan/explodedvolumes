#include "planes/cutting_planes.h"
#include <algorithm>
#include <iostream>
#include <vector>
#include <map>
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

    // compute Model Segments
    std::vector<ModelSegment> computeModelSegments(
        const Mesh &mesh,
        const std::vector<CuttingPlane> &planes,
        const Vec3 &explosionAxis,
        float explodeDistance)
    {

        std::vector<ModelSegment> segments;

        if (planes.empty() || mesh.vertices.empty() || mesh.indices.empty())
        {
            ModelSegment wholeMesh;
            wholeMesh.startIndex = 0;
            wholeMesh.indexCount = mesh.indices.size();
            wholeMesh.displacement = {0, 0, 0};
            segments.push_back(wholeMesh);
            return segments;
        }

        // Sort according to proj
        std::vector<CuttingPlane> sortedPlanes = planes;
        std::sort(sortedPlanes.begin(), sortedPlanes.end(),
                  [&explosionAxis](const CuttingPlane &a, const CuttingPlane &b)
                  {
                      float projA = a.origin.x * explosionAxis.x +
                                    a.origin.y * explosionAxis.y +
                                    a.origin.z * explosionAxis.z;
                      float projB = b.origin.x * explosionAxis.x +
                                    b.origin.y * explosionAxis.y +
                                    b.origin.z * explosionAxis.z;
                      return projA < projB;
                  });

        std::vector<int> triangleSegments(mesh.indices.size() / 3, -1);
        std::vector<float> planePositions(sortedPlanes.size());

        for (size_t i = 0; i < sortedPlanes.size(); ++i)
        {
            planePositions[i] = sortedPlanes[i].origin.x * explosionAxis.x +
                                sortedPlanes[i].origin.y * explosionAxis.y +
                                sortedPlanes[i].origin.z * explosionAxis.z;
        }

        for (size_t triIndex = 0; triIndex < mesh.indices.size() / 3; ++triIndex)
        {
            const Vertex &v0 = mesh.vertices[mesh.indices[triIndex * 3]];
            const Vertex &v1 = mesh.vertices[mesh.indices[triIndex * 3 + 1]];
            const Vertex &v2 = mesh.vertices[mesh.indices[triIndex * 3 + 2]];

            float centerProj = (projectVertexOnAxis(v0, explosionAxis) +
                                projectVertexOnAxis(v1, explosionAxis) +
                                projectVertexOnAxis(v2, explosionAxis)) /
                               3.0f;

            int segmentIndex = 0;
            for (size_t planeIdx = 0; planeIdx < planePositions.size(); ++planeIdx)
            {
                if (centerProj < planePositions[planeIdx])
                {
                    break;
                }
                segmentIndex++;
            }

            triangleSegments[triIndex] = segmentIndex;
        }

        // Number of tri
        std::map<int, int> segmentTriangleCounts;
        for (int segIndex : triangleSegments)
        {
            segmentTriangleCounts[segIndex]++;
        }

        // Temp array
        std::vector<IndexType> newIndices;
        newIndices.reserve(mesh.indices.size());

        // New pos in temp array
        std::vector<int> segmentStartIndices(segmentTriangleCounts.size(), 0);
        int segmentCount = segmentTriangleCounts.size();

        // Allocation
        for (int i = 0; i < segmentCount; ++i)
        {
            if (i > 0)
            {
                segmentStartIndices[i] = segmentStartIndices[i - 1] + segmentTriangleCounts[i - 1] * 3;
            }
            newIndices.resize(newIndices.size() + segmentTriangleCounts[i] * 3);
        }

        // fill in new index array
        std::vector<int> segmentCurrentIndices = segmentStartIndices;
        for (size_t triIndex = 0; triIndex < triangleSegments.size(); ++triIndex)
        {
            int segIdx = triangleSegments[triIndex];
            int newStartIdx = segmentCurrentIndices[segIdx];

            // copy indices
            newIndices[newStartIdx] = mesh.indices[triIndex * 3];
            newIndices[newStartIdx + 1] = mesh.indices[triIndex * 3 + 1];
            newIndices[newStartIdx + 2] = mesh.indices[triIndex * 3 + 2];

            segmentCurrentIndices[segIdx] += 3;
        }

        segments.resize(segmentCount);

        // calculate fixed point
        int fixedSegmentIndex;
        if (segmentCount % 2 == 1)
        {
            // odd
            fixedSegmentIndex = segmentCount / 2;
        }
        else
        {
            // even
            fixedSegmentIndex = segmentCount / 2 - 1;
        }

        // Calculate offset for each
        for (int i = 0; i < segmentCount; ++i)
        {
            segments[i].startIndex = segmentStartIndices[i];
            segments[i].indexCount = segmentTriangleCounts[i] * 3;

            // 计算位移
            float displacement = (i - fixedSegmentIndex) * explodeDistance;
            segments[i].displacement.x = explosionAxis.x * displacement;
            segments[i].displacement.y = explosionAxis.y * displacement;
            segments[i].displacement.z = explosionAxis.z * displacement;
        }

        std::cout << "Model divided into " << segmentCount << " segments for explosion" << std::endl;

        return segments;
    }

    // Set VAO VBO EBO
    void setupExplodedMesh(
        const Mesh &mesh,
        const std::vector<ModelSegment> &segments,
        unsigned int &VAO,
        unsigned int &VBO,
        unsigned int &EBO)
    {

        // If existed
        if (VAO != 0)
        {
            glDeleteVertexArrays(1, &VAO);
            glDeleteBuffers(1, &VBO);
            glDeleteBuffers(1, &EBO);
        }

        // Create new indices and vertices
        std::vector<Vertex> explodedVertices;
        std::vector<IndexType> explodedIndices;

        // Calculate total
        size_t totalVertexCount = mesh.vertices.size();
        size_t totalIndexCount = 0;
        for (const auto &segment : segments)
        {
            totalIndexCount += segment.indexCount;
        }

        explodedVertices.reserve(totalVertexCount);
        explodedIndices.reserve(totalIndexCount);

        // Copy vertices
        for (const auto &vertex : mesh.vertices)
        {
            explodedVertices.push_back(vertex);
        }

        // Calculate offsets
        std::vector<Vec3> vertexOffsets(explodedVertices.size(), Vec3(0, 0, 0));

        // Calculate offsets for each
        for (const auto &segment : segments)
        {
            for (int i = 0; i < segment.indexCount; ++i)
            {
                IndexType idx = mesh.indices[segment.startIndex + i];
                vertexOffsets[idx] = segment.displacement;
            }
        }

        // Applying offsets
        for (size_t i = 0; i < explodedVertices.size(); ++i)
        {
            explodedVertices[i].x += vertexOffsets[i].x;
            explodedVertices[i].y += vertexOffsets[i].y;
            explodedVertices[i].z += vertexOffsets[i].z;
        }

        // Using original indices
        explodedIndices = mesh.indices;

        // Create VAO and VBO
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        glBindVertexArray(VAO);

        // Bind buffer
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, explodedVertices.size() * sizeof(Vertex),
                     explodedVertices.data(), GL_STATIC_DRAW);

        // Set attribute
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)0);
        glEnableVertexAttribArray(0);

        // Bind index buffer
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, explodedIndices.size() * sizeof(IndexType),
                     explodedIndices.data(), GL_STATIC_DRAW);

        glBindVertexArray(0);
    }

    // Update exp state
    void updateExplosionState(
        const Mesh &mesh,
        const std::vector<CuttingPlane> &planes,
        const Vec3 &explosionAxis,
        float explodeDistance,
        bool explodeEnabled,
        std::vector<ModelSegment> &segments,
        unsigned int &VAO,
        unsigned int &VBO,
        unsigned int &EBO)
    {

        if (explodeEnabled)
        {
            // compute segments
            segments = computeModelSegments(mesh, planes, explosionAxis, explodeDistance);

            // calculate updated model
            setupExplodedMesh(mesh, segments, VAO, VBO, EBO);
        }
        else
        {
            // original mesh
            if (VAO != 0)
            {
                glDeleteVertexArrays(1, &VAO);
                glDeleteBuffers(1, &VBO);
                glDeleteBuffers(1, &EBO);
                VAO = VBO = EBO = 0;
            }

            // clear info
            segments.clear();
        }
    }

} // namespace MC
