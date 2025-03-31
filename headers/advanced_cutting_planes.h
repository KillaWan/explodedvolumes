#pragma once
#ifndef MC_ADVANCED_CUTTING_PLANES_H
#define MC_ADVANCED_CUTTING_PLANES_H

#include "cutting_planes.h"
#include "data.h"
#include <vector>
#include <memory>
#include <string>

namespace MC {

// Remove the redundant IntersectionSegment struct definition

// Enhanced IntersectionSegment with importance (optional extension)
struct EnhancedIntersectionSegment : public IntersectionSegment {
    float importance = 0.0f;
};

// Feature-based metrics computation
float calculateCurvature(const Mesh& mesh, int vertexIndex);
std::vector<float> calculateFeatureMetricAlongAxis(const Mesh& mesh, const Vec3& normalizedAxis);
void smoothFeatureMetric(std::vector<float>& featureMetric, const Mesh& mesh);
std::vector<int> getAdjacentVertices(const Mesh& mesh, int vertexIndex);
std::vector<std::vector<int>> getAdjacentTriangles(const Mesh& mesh, int vertexIndex);
int findClosestVertexToAxisPosition(const Mesh& mesh, const Vec3& normalizedAxis, float position);

// Advanced adaptive cutting plane generation
std::vector<CuttingPlane> generateAdaptiveCuttingPlanes(
    const Mesh& mesh,
    const Vec3& explosionAxis,
    int minNumPlanes,
    float adaptivityFactor = 1.0f);

// Enhanced intersection processing
float distance(const Vertex& v1, const Vertex& v2);
std::vector<std::vector<Vertex>> connectIntersectionSegments(
    const std::vector<IntersectionSegment>& segments, 
    float tolerance);
float computePolygonArea(const std::vector<Vertex>& polygon);
bool isOuterBoundary(const std::vector<Vertex>& path, const Vec3& planeNormal);
float calculatePlaneImportance(
    const Mesh& mesh,
    const CuttingPlane& plane,
    const std::vector<std::vector<Vertex>>& intersectionPaths);
PlaneIntersection computeAdvancedPlaneIntersections(
    const Mesh& mesh,
    const std::vector<CuttingPlane>& planes);

// Optimize selection of cutting planes
std::vector<CuttingPlane> optimizePlaneSelection(
    const Mesh& mesh,
    const std::vector<CuttingPlane>& candidatePlanes,
    int maxPlanes,
    float minSpacing);

} // namespace MC

#endif // MC_ADVANCED_CUTTING_PLANES_H