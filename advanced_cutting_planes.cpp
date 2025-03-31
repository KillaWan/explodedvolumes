#include "headers/advanced_cutting_planes.h"
#include <algorithm>
#include <iostream>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <cmath>

namespace MC {

// Helper struct for tracking connected components
struct SegmentNode {
    int index;
    bool visited = false;
    std::vector<int> connections;
};

// ------------------------------------------------------------------------
// Feature-based metrics computation
// ------------------------------------------------------------------------

// Calculate the curvature at a vertex using the angle deficit method
float calculateCurvature(const Mesh& mesh, int vertexIndex) {
    // We need the vertex and all its adjacent triangles
    const Vertex& v = mesh.vertices[vertexIndex];
    
    // Get all neighboring faces (triangles) that share this vertex
    std::vector<std::vector<int>> adjacentTriangles = getAdjacentTriangles(mesh, vertexIndex);
    
    if (adjacentTriangles.empty()) {
        return 0.0f; // No adjacent triangles, can't compute curvature
    }
    
    // Sum of angles around the vertex
    float angleSum = 0.0f;
    
    for (const auto& triangle : adjacentTriangles) {
        // Get the other two vertices of this triangle
        int idx1 = triangle[0] == vertexIndex ? triangle[1] : triangle[0];
        int idx2 = triangle[2] == vertexIndex ? triangle[1] : triangle[2];
        
        // Get vectors from v to the other two vertices
        Vec3 edge1 = {
            mesh.vertices[idx1].x - v.x,
            mesh.vertices[idx1].y - v.y,
            mesh.vertices[idx1].z - v.z
        };
        
        Vec3 edge2 = {
            mesh.vertices[idx2].x - v.x,
            mesh.vertices[idx2].y - v.y,
            mesh.vertices[idx2].z - v.z
        };
        
        // Normalize vectors
        float len1 = std::sqrt(edge1.x * edge1.x + edge1.y * edge1.y + edge1.z * edge1.z);
        float len2 = std::sqrt(edge2.x * edge2.x + edge2.y * edge2.y + edge2.z * edge2.z);
        
        if (len1 > 1e-6f && len2 > 1e-6f) {
            edge1.x /= len1; edge1.y /= len1; edge1.z /= len1;
            edge2.x /= len2; edge2.y /= len2; edge2.z /= len2;
            
            // Calculate dot product
            float dotProduct = edge1.x * edge2.x + edge1.y * edge2.y + edge1.z * edge2.z;
            
            // Clamp to valid range for acos
            dotProduct = std::max(-1.0f, std::min(1.0f, dotProduct));
            
            // Calculate angle and add to sum
            float angle = std::acos(dotProduct);
            angleSum += angle;
        }
    }
    
    // For a flat surface, the sum of angles is 2π
    // The difference (2π - sum) is the angle deficit, which is proportional to Gaussian curvature
    float curvature = 2.0f * M_PI - angleSum;
    return std::abs(curvature); // Return absolute value for simplicity
}

// Calculate feature metric along the explosion axis
std::vector<float> calculateFeatureMetricAlongAxis(const Mesh& mesh, const Vec3& normalizedAxis) {
    std::vector<float> featureMetric(mesh.vertices.size(), 0.0f);
    
    // Calculate curvature for each vertex
    for (size_t i = 0; i < mesh.vertices.size(); i++) {
        featureMetric[i] = calculateCurvature(mesh, i);
    }
    
    // Optional: Apply smoothing to the feature metric
    smoothFeatureMetric(featureMetric, mesh);
    
    return featureMetric;
}

// Apply Laplacian smoothing to the feature metric
void smoothFeatureMetric(std::vector<float>& featureMetric, const Mesh& mesh) {
    std::vector<float> smoothedMetric = featureMetric;
    
    for (size_t i = 0; i < mesh.vertices.size(); i++) {
        // Get adjacent vertices
        std::vector<int> neighbors = getAdjacentVertices(mesh, i);
        
        if (!neighbors.empty()) {
            float sum = featureMetric[i];  // Include the vertex itself
            
            for (int neighborIdx : neighbors) {
                sum += featureMetric[neighborIdx];
            }
            
            smoothedMetric[i] = sum / (neighbors.size() + 1);
        }
    }
    
    featureMetric = std::move(smoothedMetric);
}

// Get adjacent vertices to a given vertex
std::vector<int> getAdjacentVertices(const Mesh& mesh, int vertexIndex) {
    std::unordered_set<int> neighbors;
    
    // Iterate through triangles to find adjacent vertices
    for (size_t i = 0; i < mesh.indices.size(); i += 3) {
        for (size_t j = 0; j < 3; j++) {
            if (mesh.indices[i + j] == vertexIndex) {
                // Add the other two vertices of this triangle
                neighbors.insert(mesh.indices[i + (j + 1) % 3]);
                neighbors.insert(mesh.indices[i + (j + 2) % 3]);
                break;
            }
        }
    }
    
    return std::vector<int>(neighbors.begin(), neighbors.end());
}

// Get adjacent triangles to a vertex
std::vector<std::vector<int>> getAdjacentTriangles(const Mesh& mesh, int vertexIndex) {
    std::vector<std::vector<int>> triangles;
    
    // Iterate through all triangles
    for (size_t i = 0; i < mesh.indices.size(); i += 3) {
        // Check if this triangle contains our vertex
        if (mesh.indices[i] == vertexIndex || 
            mesh.indices[i + 1] == vertexIndex || 
            mesh.indices[i + 2] == vertexIndex) {
            
            triangles.push_back({
                static_cast<int>(mesh.indices[i]),
                static_cast<int>(mesh.indices[i + 1]),
                static_cast<int>(mesh.indices[i + 2])
            });
        }
    }
    
    return triangles;
}

// Find the vertex closest to a position along the explosion axis
int findClosestVertexToAxisPosition(const Mesh& mesh, const Vec3& normalizedAxis, float position) {
    int closestIdx = -1;
    float minDist = std::numeric_limits<float>::max();
    
    for (size_t i = 0; i < mesh.vertices.size(); i++) {
        const Vertex& v = mesh.vertices[i];
        
        // Project vertex onto axis
        float proj = v.x * normalizedAxis.x + v.y * normalizedAxis.y + v.z * normalizedAxis.z;
        
        // Calculate distance to specified position
        float dist = std::abs(proj - position);
        
        if (dist < minDist) {
            minDist = dist;
            closestIdx = i;
        }
    }
    
    return closestIdx;
}

// ------------------------------------------------------------------------
// Advanced adaptive cutting plane generation
// ------------------------------------------------------------------------

std::vector<CuttingPlane> generateAdaptiveCuttingPlanes(
    const Mesh& mesh,
    const Vec3& explosionAxis,
    int minNumPlanes,
    float adaptivityFactor) 
{
    std::vector<CuttingPlane> planes;
    
    // Normalize explosion axis
    Vec3 normalizedAxis;
    float axisLength = std::sqrt(
        explosionAxis.x * explosionAxis.x +
        explosionAxis.y * explosionAxis.y +
        explosionAxis.z * explosionAxis.z);
    
    if (axisLength > 1e-6f) {
        normalizedAxis.x = explosionAxis.x / axisLength;
        normalizedAxis.y = explosionAxis.y / axisLength;
        normalizedAxis.z = explosionAxis.z / axisLength;
    } else {
        // Default to Z-axis
        normalizedAxis.x = 0.0f;
        normalizedAxis.y = 0.0f;
        normalizedAxis.z = 1.0f;
    }
    
    // Calculate feature metric along the explosion axis
    std::vector<float> featureMetric = calculateFeatureMetricAlongAxis(mesh, normalizedAxis);
    
    // Find range along axis
    float minProj = std::numeric_limits<float>::max();
    float maxProj = std::numeric_limits<float>::lowest();
    std::vector<float> vertexProjections(mesh.vertices.size());
    
    for (size_t i = 0; i < mesh.vertices.size(); i++) {
        const auto& vertex = mesh.vertices[i];
        float proj = vertex.x * normalizedAxis.x +
                     vertex.y * normalizedAxis.y +
                     vertex.z * normalizedAxis.z;
        
        vertexProjections[i] = proj;
        minProj = std::min(minProj, proj);
        maxProj = std::max(maxProj, proj);
    }
    
    // Total length along axis
    float totalLength = maxProj - minProj;
    
    // Generate plane positions based on adaptive spacing
    std::vector<float> planePositions;
    float currentPos = minProj + totalLength * 0.05f; // Start slightly inside the model
    float endPos = maxProj - totalLength * 0.05f; // End slightly inside the model
    
    // Always add the first position
    planePositions.push_back(currentPos);
    
    while (currentPos < endPos) {
        // Find closest vertex to current position
        int closestIdx = findClosestVertexToAxisPosition(mesh, normalizedAxis, currentPos);
        
        // Calculate step size based on feature metric (smaller step for higher feature value)
        float baseStep = totalLength / (minNumPlanes + 1);
        float adaptiveStep;
        
        if (closestIdx >= 0) {
            // Adjust step based on curvature (or other feature metric)
            adaptiveStep = baseStep / (1.0f + adaptivityFactor * featureMetric[closestIdx]);
        } else {
            adaptiveStep = baseStep;
        }
        
        // Ensure minimum step size
        adaptiveStep = std::max(adaptiveStep, totalLength * 0.01f);
        
        // Move to next position
        currentPos += adaptiveStep;
        
        if (currentPos < endPos) {
            planePositions.push_back(currentPos);
        }
    }
    
    // Ensure we have at least the minimum number of planes by adding more if needed
    if (planePositions.size() < minNumPlanes) {
        // Calculate positions for additional uniform planes
        int additionalPlanes = minNumPlanes - planePositions.size();
        float uniformStep = totalLength / (additionalPlanes + 1);
        
        std::vector<float> uniformPositions;
        for (int i = 1; i <= additionalPlanes; i++) {
            uniformPositions.push_back(minProj + i * uniformStep);
        }
        
        // Merge with existing positions and sort
        planePositions.insert(planePositions.end(), uniformPositions.begin(), uniformPositions.end());
        std::sort(planePositions.begin(), planePositions.end());
    }
    
    // Convert positions to cutting planes
    for (float pos : planePositions) {
        CuttingPlane plane;
        plane.normal = normalizedAxis;
        plane.distance = pos;
        
        // Calculate origin point on the plane (relative to mesh center)
        plane.origin.x = mesh.center.x + normalizedAxis.x * (pos - (minProj + maxProj) / 2.0f);
        plane.origin.y = mesh.center.y + normalizedAxis.y * (pos - (minProj + maxProj) / 2.0f);
        plane.origin.z = mesh.center.z + normalizedAxis.z * (pos - (minProj + maxProj) / 2.0f);
        
        planes.push_back(plane);
    }
    
    std::cout << "Generated " << planes.size() << " adaptive cutting planes along the explosion axis" << std::endl;
    
    return planes;
}

// ------------------------------------------------------------------------
// Enhanced intersection processing
// ------------------------------------------------------------------------

// Calculate the Euclidean distance between two vertices
float distance(const Vertex& v1, const Vertex& v2) {
    float dx = v1.x - v2.x;
    float dy = v1.y - v2.y;
    float dz = v1.z - v2.z;
    return std::sqrt(dx*dx + dy*dy + dz*dz);
}

// Connect intersection segments into continuous paths
std::vector<std::vector<Vertex>> connectIntersectionSegments(
    const std::vector<IntersectionSegment>& segments, 
    float tolerance)
{
    std::vector<std::vector<Vertex>> paths;
    if (segments.empty()) {
        return paths;
    }
    
    // Create a graph of segment connections
    std::vector<SegmentNode> graph(segments.size());
    for (size_t i = 0; i < segments.size(); i++) {
        graph[i].index = i;
    }
    
    // Build connections between segments
    for (size_t i = 0; i < segments.size(); i++) {
        for (size_t j = 0; j < segments.size(); j++) {
            if (i == j) continue;
            
            // Check if end of segment i connects to start of segment j
            if (distance(segments[i].end, segments[j].start) < tolerance) {
                graph[i].connections.push_back(j);
            }
            // Check if end of segment i connects to end of segment j (reversed)
            else if (distance(segments[i].end, segments[j].end) < tolerance) {
                graph[i].connections.push_back(-j - 1); // Negative to indicate reversed
            }
        }
    }
    
    // Traverse the graph to build paths
    for (size_t i = 0; i < graph.size(); i++) {
        if (graph[i].visited) continue;
        
        // Start a new path with this segment
        std::vector<Vertex> currentPath;
        currentPath.push_back(segments[i].start);
        currentPath.push_back(segments[i].end);
        graph[i].visited = true;
        
        // Try to extend the path forward
        int currentNode = i;
        bool extended = true;
        
        while (extended) {
            extended = false;
            
            // Try each connection
            for (int nextNodeIdx : graph[currentNode].connections) {
                int nextNode = std::abs(nextNodeIdx) - (nextNodeIdx < 0 ? 1 : 0);
                bool reversed = (nextNodeIdx < 0);
                
                if (!graph[nextNode].visited) {
                    // Add the next segment to the path
                    if (!reversed) {
                        // Normal orientation
                        currentPath.push_back(segments[nextNode].end);
                    } else {
                        // Reversed orientation
                        currentPath.push_back(segments[nextNode].start);
                    }
                    
                    graph[nextNode].visited = true;
                    currentNode = nextNode;
                    extended = true;
                    break;
                }
            }
        }
        
        // Check if the path is closed (first point == last point)
        if (currentPath.size() > 2 && distance(currentPath.front(), currentPath.back()) < tolerance) {
            // Close the loop exactly by setting the last point equal to the first
            currentPath.back() = currentPath.front();
        }
        
        paths.push_back(currentPath);
    }
    
    return paths;
}

// Compute the area of a polygon in 3D by projecting onto the dominant plane
float computePolygonArea(const std::vector<Vertex>& polygon) {
    if (polygon.size() < 3) {
        return 0.0f;
    }
    
    // Determine dominant plane (largest normal component)
    Vec3 normal = {0, 0, 0};
    
    // Calculate an approximate normal using the first triangle
    Vec3 v1 = {
        polygon[1].x - polygon[0].x,
        polygon[1].y - polygon[0].y,
        polygon[1].z - polygon[0].z
    };
    
    Vec3 v2 = {
        polygon[2].x - polygon[0].x,
        polygon[2].y - polygon[0].y,
        polygon[2].z - polygon[0].z
    };
    
    // Cross product to get normal
    normal.x = v1.y * v2.z - v1.z * v2.y;
    normal.y = v1.z * v2.x - v1.x * v2.z;
    normal.z = v1.x * v2.y - v1.y * v2.x;
    
    // Find dominant axis
    float absX = std::abs(normal.x);
    float absY = std::abs(normal.y);
    float absZ = std::abs(normal.z);
    
    int dominantAxis;
    if (absX >= absY && absX >= absZ) {
        dominantAxis = 0; // X
    } else if (absY >= absX && absY >= absZ) {
        dominantAxis = 1; // Y
    } else {
        dominantAxis = 2; // Z
    }
    
    // Project onto the dominant plane and compute area using shoelace formula
    float area = 0.0f;
    for (size_t i = 0; i < polygon.size(); i++) {
        size_t j = (i + 1) % polygon.size();
        
        float coord1i, coord2i, coord1j, coord2j;
        
        // Select the two coordinates based on the dominant axis
        if (dominantAxis == 0) { // Project onto YZ plane
            coord1i = polygon[i].y;
            coord2i = polygon[i].z;
            coord1j = polygon[j].y;
            coord2j = polygon[j].z;
        } else if (dominantAxis == 1) { // Project onto XZ plane
            coord1i = polygon[i].x;
            coord2i = polygon[i].z;
            coord1j = polygon[j].x;
            coord2j = polygon[j].z;
        } else { // Project onto XY plane
            coord1i = polygon[i].x;
            coord2i = polygon[i].y;
            coord1j = polygon[j].x;
            coord2j = polygon[j].y;
        }
        
        area += (coord1i * coord2j - coord1j * coord2i);
    }
    
    return std::abs(area) / 2.0f;
}

// Determine if a closed path is an outer boundary (clockwise) or hole (counterclockwise)
bool isOuterBoundary(const std::vector<Vertex>& path, const Vec3& planeNormal) {
    if (path.size() < 3) {
        return false;
    }
    
    // Calculate the signed area to determine winding direction
    float signedArea = 0.0f;
    
    // Find dominant plane for projection
    float absX = std::abs(planeNormal.x);
    float absY = std::abs(planeNormal.y);
    float absZ = std::abs(planeNormal.z);
    
    int dominantAxis;
    if (absX >= absY && absX >= absZ) {
        dominantAxis = 0; // YZ plane
    } else if (absY >= absX && absY >= absZ) {
        dominantAxis = 1; // XZ plane
    } else {
        dominantAxis = 2; // XY plane
    }
    
    // Project and calculate signed area using shoelace formula
    for (size_t i = 0; i < path.size(); i++) {
        size_t j = (i + 1) % path.size();
        
        float x1, y1, x2, y2;
        
        // Select the two coordinates based on the dominant axis
        if (dominantAxis == 0) { // Project onto YZ plane
            x1 = path[i].y;
            y1 = path[i].z;
            x2 = path[j].y;
            y2 = path[j].z;
        } else if (dominantAxis == 1) { // Project onto XZ plane
            x1 = path[i].x;
            y1 = path[i].z;
            x2 = path[j].x;
            y2 = path[j].z;
        } else { // Project onto XY plane
            x1 = path[i].x;
            y1 = path[i].y;
            x2 = path[j].x;
            y2 = path[j].y;
        }
        
        signedArea += (x1 * y2 - x2 * y1);
    }
    
    // Adjust sign based on normal direction
    bool clockwise;
    if (dominantAxis == 0) {
        clockwise = (signedArea * planeNormal.x) > 0;
    } else if (dominantAxis == 1) {
        clockwise = (signedArea * planeNormal.y) > 0;
    } else {
        clockwise = (signedArea * planeNormal.z) > 0;
    }
    
    // Outer boundaries are clockwise when viewed from outside
    return clockwise;
}

// Group paths by outer boundaries and holes
struct PlaneIntersectionComponent {
    std::vector<Vertex> outerBoundary;
    std::vector<std::vector<Vertex>> holes;
};

// Calculate the importance of a cutting plane based on feature metrics
float calculatePlaneImportance(
    const Mesh& mesh,
    const CuttingPlane& plane,
    const std::vector<std::vector<Vertex>>& intersectionPaths) 
{
    if (intersectionPaths.empty()) {
        return 0.0f;
    }
    
    float importance = 0.0f;
    
    // Factor 1: Total intersection length
    float totalLength = 0.0f;
    for (const auto& path : intersectionPaths) {
        for (size_t i = 0; i < path.size() - 1; i++) {
            totalLength += distance(path[i], path[i+1]);
        }
    }
    importance += totalLength;
    
    // Factor 2: Area of intersection
    float totalArea = 0.0f;
    for (const auto& path : intersectionPaths) {
        if (path.size() >= 3) {
            totalArea += computePolygonArea(path);
        }
    }
    importance += totalArea * 2.0f;
    
    // Factor 3: Path complexity (number of vertices as a rough proxy)
    int totalVertices = 0;
    for (const auto& path : intersectionPaths) {
        totalVertices += path.size();
    }
    importance += totalVertices * 0.1f;
    
    // Factor 4: Number of distinct components
    importance += intersectionPaths.size() * 5.0f;
    
    return importance;
}

// Enhanced version to handle multiple intersection components
PlaneIntersection computeAdvancedPlaneIntersections(
    const Mesh& mesh,
    const std::vector<CuttingPlane>& planes)
{
    PlaneIntersection intersection;
    intersection.visible = false;
    
    // Check if mesh is valid
    if (mesh.vertices.empty() || mesh.indices.empty()) {
        std::cerr << "Empty mesh, cannot compute plane intersections" << std::endl;
        return intersection;
    }
    
    // For each plane
    for (size_t planeIdx = 0; planeIdx < planes.size(); planeIdx++) {
        const CuttingPlane& plane = planes[planeIdx];
        
        // Compute raw segments for this plane
        std::vector<IntersectionSegment> planeSegments;
        
        // Traverse all triangles
        for (size_t i = 0; i < mesh.indices.size(); i += 3) {
            const Vertex& v0 = mesh.vertices[mesh.indices[i]];
            const Vertex& v1 = mesh.vertices[mesh.indices[i + 1]];
            const Vertex& v2 = mesh.vertices[mesh.indices[i + 2]];
            
            IntersectionSegment segment;
            if (trianglePlaneIntersection(v0, v1, v2, plane, segment)) {
                segment.planeIndex = planeIdx;
                planeSegments.push_back(segment);
            }
        }
        
        // Connect segments into continuous paths
        std::vector<std::vector<Vertex>> paths = connectIntersectionSegments(planeSegments, 1e-4f);
        
        // Calculate importance for this plane
        float importance = calculatePlaneImportance(mesh, plane, paths);
        
        // Convert paths back to segments for rendering
        for (const auto& path : paths) {
            for (size_t i = 0; i < path.size() - 1; i++) {
                IntersectionSegment segment;
                segment.start = path[i];
                segment.end = path[i + 1];
                segment.planeIndex = planeIdx;
                
                // Add the segment to the intersection
                intersection.segments.push_back(segment);
            }
        }
    }
    
    std::cout << "Computed " << intersection.segments.size()
              << " advanced intersection segments for " << planes.size() << " planes" << std::endl;
    
    return intersection;
}

// Optimize selection of cutting planes
std::vector<CuttingPlane> optimizePlaneSelection(
    const Mesh& mesh,
    const std::vector<CuttingPlane>& candidatePlanes,
    int maxPlanes,
    float minSpacing) 
{
    // If we have fewer candidates than max, return all of them
    if (candidatePlanes.size() <= maxPlanes) {
        return candidatePlanes;
    }
    
    // Calculate importance for each plane
    std::vector<std::pair<float, int>> planeImportance;
    
    for (size_t i = 0; i < candidatePlanes.size(); i++) {
        // Compute intersection paths for this plane
        const CuttingPlane& plane = candidatePlanes[i];
        std::vector<IntersectionSegment> planeSegments;
        
        for (size_t j = 0; j < mesh.indices.size(); j += 3) {
            const Vertex& v0 = mesh.vertices[mesh.indices[j]];
            const Vertex& v1 = mesh.vertices[mesh.indices[j + 1]];
            const Vertex& v2 = mesh.vertices[mesh.indices[j + 2]];
            
            IntersectionSegment segment;
            if (trianglePlaneIntersection(v0, v1, v2, plane, segment)) {
                planeSegments.push_back(segment);
            }
        }
        
        // Connect segments into paths
        std::vector<std::vector<Vertex>> paths = connectIntersectionSegments(planeSegments, 1e-4f);
        
        // Calculate importance
        float importance = calculatePlaneImportance(mesh, plane, paths);
        planeImportance.push_back({importance, i});
    }
    
    // Sort by importance
    std::sort(planeImportance.begin(), planeImportance.end(),
             [](const auto& a, const auto& b) { return a.first > b.first; });
    
    // Select planes greedily while maintaining minimum spacing
    std::vector<CuttingPlane> selectedPlanes;
    std::vector<bool> selected(candidatePlanes.size(), false);
    
    // Always include the most important plane
    if (!planeImportance.empty()) {
        int mostImportantIdx = planeImportance[0].second;
        selectedPlanes.push_back(candidatePlanes[mostImportantIdx]);
        selected[mostImportantIdx] = true;
    }
    
    // Greedily select remaining planes
    for (size_t i = 1; i < planeImportance.size() && selectedPlanes.size() < maxPlanes; i++) {
        int currentIdx = planeImportance[i].second;
        
        // Check if this plane is too close to any selected plane
        bool tooClose = false;
        for (const auto& selectedPlane : selectedPlanes) {
            float planeDist = std::abs(candidatePlanes[currentIdx].distance - selectedPlane.distance);
            if (planeDist < minSpacing) {
                tooClose = true;
                break;
            }
        }
        
        // If not too close, select this plane
        if (!tooClose) {
            selectedPlanes.push_back(candidatePlanes[currentIdx]);
            selected[currentIdx] = true;
        }
    }
    
    // If we still don't have enough planes, add more based just on spacing
    if (selectedPlanes.size() < maxPlanes) {
        for (size_t i = 0; i < candidatePlanes.size() && selectedPlanes.size() < maxPlanes; i++) {
            if (!selected[i]) {
                bool tooClose = false;
                for (const auto& selectedPlane : selectedPlanes) {
                    float planeDist = std::abs(candidatePlanes[i].distance - selectedPlane.distance);
                    if (planeDist < minSpacing) {
                        tooClose = true;
                        break;
                    }
                }
                
                if (!tooClose) {
                    selectedPlanes.push_back(candidatePlanes[i]);
                    selected[i] = true;
                }
            }
        }
    }
    
    // Sort selected planes by distance for proper rendering
    std::sort(selectedPlanes.begin(), selectedPlanes.end(),
             [](const CuttingPlane& a, const CuttingPlane& b) {
                 return a.distance < b.distance;
             });
    
    std::cout << "Optimized plane selection from " << candidatePlanes.size() 
              << " to " << selectedPlanes.size() << " planes" << std::endl;
    
    return selectedPlanes;
}

} // namespace MC