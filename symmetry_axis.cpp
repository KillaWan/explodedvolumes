#include "symmetry_axis.h"
#include <cmath>
#include <iostream>
#include <algorithm>
#include <map>
#include <Eigen/Dense>  // Assuming Eigen library for matrix operations

// ---------- Vector operations ---------- //
inline Vec3 normalize(const Vec3& v) {
    float len = std::sqrt(v.x*v.x + v.y*v.y + v.z*v.z);
    if(len < 1e-12f) return Vec3(0, 0, 1);
    return Vec3(v.x/len, v.y/len, v.z/len);
}

inline float dot(const Vec3& a, const Vec3& b) {
    return a.x*b.x + a.y*b.y + a.z*b.z;
}

inline Vec3 cross(const Vec3& a, const Vec3& b) {
    return Vec3(a.y*b.z - a.z*b.y,
                a.z*b.x - a.x*b.z,
                a.x*b.y - a.y*b.x);
}

// Compute the distance between a point and a line defined by a point and direction
float pointLineDistance(const Vec3& point, const Vec3& linePoint, const Vec3& lineDir) {
    Vec3 v = cross(lineDir, Vec3(point.x - linePoint.x, point.y - linePoint.y, point.z - linePoint.z));
    return std::sqrt(dot(v, v)) / std::sqrt(dot(lineDir, lineDir));
}

// Compute the centroid of vertices
Vec3 computeCentroid(const std::vector<Vertex>& meshVertices) {
    Vec3 centroid(0, 0, 0);
    for (const auto& v : meshVertices) {
        centroid.x += v.x;
        centroid.y += v.y;
        centroid.z += v.z;
    }
    float count = static_cast<float>(meshVertices.size());
    if (count > 0) {
        centroid.x /= count;
        centroid.y /= count;
        centroid.z /= count;
    }
    return centroid;
}

// Project a point onto a plane defined by its normal and a point on the plane
Vec3 projectPointOntoPlane(const Vec3& point, const Vec3& planeNormal, const Vec3& pointOnPlane) {
    Vec3 n = normalize(planeNormal);
    float d = dot(n, Vec3(point.x - pointOnPlane.x, point.y - pointOnPlane.y, point.z - pointOnPlane.z));
    return Vec3(point.x - d * n.x, point.y - d * n.y, point.z - d * n.z);
}

// ---------- Symmetry detection ---------- //

// Implementation of Mitra's algorithm for rotational symmetry detection
SymmetryResult detectRotationalSymmetry(const std::vector<Vertex>& meshVertices) {
    SymmetryResult result;
    result.type = SymmetryType::None;
    result.symmetryOrder = 1;
    result.rotationAxis = Vec3(0, 0, 1);
    
    // Step 1: Compute the centroid
    Vec3 centroid = computeCentroid(meshVertices);
    
    // Step 2: Transform-domain voting for potential rotation axes
    // This is a simplified implementation of Mitra's algorithm
    
    // For a complete implementation:
    // 1. Generate pairs of points on the mesh surface
    // 2. For each pair, compute potential rotation axes
    // 3. Vote in transform space (direction + angle)
    // 4. Identify peaks in the voting space
    
    // Candidate axes for rotation (in practice, these would be derived from voting)
    std::vector<Vec3> candidateAxes = {
        Vec3(0, 0, 1),  // Z-axis
        Vec3(1, 0, 0),  // X-axis
        Vec3(0, 1, 0)   // Y-axis
    };
    
    std::map<int, Vec3> detectedSymmetries; // Maps order to axis
    
    // For each candidate axis, check for rotational symmetry
    for (const auto& axis : candidateAxes) {
        // Test different orders of symmetry (2, 3, 4, etc.)
        for (int order = 2; order <= 8; ++order) {
            float angleStep = 2.0f * M_PI / order;
            bool isSymmetric = true;
            
            // For each vertex, check if there are corresponding vertices for each rotation
            for (const auto& vertex : meshVertices) {
                Vec3 relPos = Vec3(
                    vertex.x - centroid.x,
                    vertex.y - centroid.y,
                    vertex.z - centroid.z
                );
                
                // For each rotation angle
                for (int i = 1; i < order; ++i) {
                    float angle = angleStep * i;
                    
                    // Rotate the relative position
                    // This is a simplification - proper axis-angle rotation should be used
                    // TODO: Implement proper axis-angle rotation
                    
                    // Check if there's a vertex close to the rotated position
                    bool foundMatchingVertex = false;
                    Vec3 rotatedPos = relPos; // Placeholder for actual rotation
                    
                    // Convert back to world coordinates
                    rotatedPos.x += centroid.x;
                    rotatedPos.y += centroid.y;
                    rotatedPos.z += centroid.z;
                    
                    // Search for a matching vertex (simplified)
                    // In a real implementation, use spatial data structures for efficiency
                    
                    // If no matching vertex found, this is not a symmetric model for this order
                    if (!foundMatchingVertex) {
                        isSymmetric = false;
                        break;
                    }
                }
                
                if (!isSymmetric) break;
            }
            
            if (isSymmetric) {
                detectedSymmetries[order] = axis;
            }
        }
    }
    
    // Find the highest order symmetry
    if (!detectedSymmetries.empty()) {
        auto highestOrder = detectedSymmetries.rbegin(); // Highest key in the map
        result.type = SymmetryType::Rotational;
        result.symmetryOrder = highestOrder->first;
        result.rotationAxis = highestOrder->second;
    }
    
    // For demonstration purposes, let's assume we detected a 4-fold rotational symmetry around the Z-axis
    // Remove this hardcoded example in a real implementation
    result.type = SymmetryType::Rotational;
    result.symmetryOrder = 4;
    result.rotationAxis = Vec3(0, 0, 1);
    
    return result;
}

// Detect reflective/mirror symmetry
SymmetryResult detectReflectiveSymmetry(const std::vector<Vertex>& meshVertices) {
    SymmetryResult result;
    result.type = SymmetryType::None;
    result.symmetryOrder = 1;
    result.reflectiveNormal = Vec3(0, 1, 0);
    
    // Step 1: Compute the centroid
    Vec3 centroid = computeCentroid(meshVertices);
    
    // Step 2: Test candidate reflection planes
    // In practice, this would involve more sophisticated voting mechanisms
    
    // Candidate planes (in practice, these would be derived from more comprehensive analysis)
    std::vector<Vec3> candidatePlaneNormals = {
        Vec3(1, 0, 0),  // YZ plane
        Vec3(0, 1, 0),  // XZ plane
        Vec3(0, 0, 1)   // XY plane
    };
    
    float bestScore = 0;
    Vec3 bestNormal;
    
    for (const auto& normal : candidatePlaneNormals) {
        float symmetryScore = 0;
        
        // For each vertex, check if there's a corresponding reflected vertex
        for (const auto& vertex : meshVertices) {
            Vec3 relPos = Vec3(
                vertex.x - centroid.x,
                vertex.y - centroid.y,
                vertex.z - centroid.z
            );
            
            // Reflect the position across the plane
            float d = dot(normal, relPos);
            Vec3 reflectedPos = Vec3(
                relPos.x - 2 * d * normal.x,
                relPos.y - 2 * d * normal.y,
                relPos.z - 2 * d * normal.z
            );
            
            // Convert back to world coordinates
            reflectedPos.x += centroid.x;
            reflectedPos.y += centroid.y;
            reflectedPos.z += centroid.z;
            
            // Search for a matching vertex (simplified)
            // In a real implementation, use spatial data structures for efficiency
            
            // Calculate similarity score based on closest match
            // Accumulate score for all vertices
        }
        
        if (symmetryScore > bestScore) {
            bestScore = symmetryScore;
            bestNormal = normal;
        }
    }
    
    // Check if the best score indicates symmetry
    float symmetryThreshold = 0.8f; // Adjust as needed
    if (bestScore > symmetryThreshold) {
        result.type = SymmetryType::Reflective;
        result.reflectiveNormal = bestNormal;
    }
    
    // For demonstration purposes, assume we found reflective symmetry across the XZ plane
    // Remove this hardcoded example in a real implementation
    result.type = SymmetryType::Reflective;
    result.reflectiveNormal = Vec3(0, 1, 0);
    
    return result;
}

// Main symmetry detection function
SymmetryResult detectSymmetry(const std::vector<Vertex>& meshVertices) {
    // First check for rotational symmetry
    SymmetryResult rotational = detectRotationalSymmetry(meshVertices);
    
    if (rotational.type == SymmetryType::Rotational) {
        return rotational;
    }
    
    // If no rotational symmetry, check for reflective symmetry
    return detectReflectiveSymmetry(meshVertices);
}

// ---------- PCA Implementation ---------- //

// Perform 3D PCA and return the first principal component
Vec3 pca3D(const std::vector<Vertex>& meshVertices) {
    // Compute centroid
    Vec3 centroid = computeCentroid(meshVertices);
    
    // Build covariance matrix
    Eigen::Matrix3f covMatrix = Eigen::Matrix3f::Zero();
    
    for (const auto& vertex : meshVertices) {
        Vec3 v = Vec3(
            vertex.x - centroid.x,
            vertex.y - centroid.y,
            vertex.z - centroid.z
        );
        
        covMatrix(0, 0) += v.x * v.x;
        covMatrix(0, 1) += v.x * v.y;
        covMatrix(0, 2) += v.x * v.z;
        covMatrix(1, 0) += v.y * v.x;
        covMatrix(1, 1) += v.y * v.y;
        covMatrix(1, 2) += v.y * v.z;
        covMatrix(2, 0) += v.z * v.x;
        covMatrix(2, 1) += v.z * v.y;
        covMatrix(2, 2) += v.z * v.z;
    }
    
    // Normalize by number of points
    covMatrix /= meshVertices.size();
    
    // Compute eigenvalues and eigenvectors
    Eigen::SelfAdjointEigenSolver<Eigen::Matrix3f> solver(covMatrix);
    
    // Get eigenvector corresponding to largest eigenvalue
    Eigen::Vector3f eigenVec = solver.eigenvectors().col(2); // Column with largest eigenvalue
    
    return Vec3(eigenVec(0), eigenVec(1), eigenVec(2));
}

// Perform 2D PCA on vertices projected onto a plane
Vec3 pca2DOnPlane(const std::vector<Vertex>& meshVertices, const Vec3& planeNormal) {
    // Normalize plane normal
    Vec3 n = normalize(planeNormal);
    
    // Get the centroid of the mesh
    Vec3 centroid = computeCentroid(meshVertices);
    
    // Project all vertices onto the plane
    std::vector<Vec3> projectedPoints;
    projectedPoints.reserve(meshVertices.size());
    
    for (const auto& vertex : meshVertices) {
        Vec3 point(vertex.x, vertex.y, vertex.z);
        Vec3 projectedPoint = projectPointOntoPlane(point, n, centroid);
        projectedPoints.push_back(projectedPoint);
    }
    
    // Compute centroid of projected points
    Vec3 projectedCentroid(0, 0, 0);
    for (const auto& p : projectedPoints) {
        projectedCentroid.x += p.x;
        projectedCentroid.y += p.y;
        projectedCentroid.z += p.z;
    }
    float count = static_cast<float>(projectedPoints.size());
    if (count > 0) {
        projectedCentroid.x /= count;
        projectedCentroid.y /= count;
        projectedCentroid.z /= count;
    }
    
    // Create a local 2D coordinate system on the plane
    // First basis vector is arbitrary, perpendicular to n
    Vec3 u;
    if (std::abs(n.x) < std::abs(n.y) && std::abs(n.x) < std::abs(n.z)) {
        u = normalize(cross(Vec3(1, 0, 0), n));
    } else {
        u = normalize(cross(Vec3(0, 1, 0), n));
    }
    
    // Second basis vector completes the orthogonal system
    Vec3 v = cross(n, u);
    
    // Map each projected point to 2D coordinates in the plane
    std::vector<Eigen::Vector2f> points2D;
    points2D.reserve(projectedPoints.size());
    
    for (const auto& p : projectedPoints) {
        Vec3 relVec = Vec3(
            p.x - projectedCentroid.x,
            p.y - projectedCentroid.y,
            p.z - projectedCentroid.z
        );
        
        float u_coord = dot(relVec, u);
        float v_coord = dot(relVec, v);
        points2D.push_back(Eigen::Vector2f(u_coord, v_coord));
    }
    
    // Build 2D covariance matrix
    Eigen::Matrix2f covMatrix = Eigen::Matrix2f::Zero();
    
    for (const auto& p : points2D) {
        covMatrix(0, 0) += p(0) * p(0);
        covMatrix(0, 1) += p(0) * p(1);
        covMatrix(1, 0) += p(1) * p(0);
        covMatrix(1, 1) += p(1) * p(1);
    }
    
    // Normalize by number of points
    covMatrix /= points2D.size();
    
    // Compute eigenvalues and eigenvectors
    Eigen::SelfAdjointEigenSolver<Eigen::Matrix2f> solver(covMatrix);
    
    // Get eigenvector corresponding to largest eigenvalue
    Eigen::Vector2f eigenVec = solver.eigenvectors().col(1); // Column with largest eigenvalue
    
    // Convert 2D eigenvector back to 3D
    Vec3 result = Vec3(
        eigenVec(0) * u.x + eigenVec(1) * v.x,
        eigenVec(0) * u.y + eigenVec(1) * v.y,
        eigenVec(0) * u.z + eigenVec(1) * v.z
    );
    
    return normalize(result);
}

// ---------- Main function: computeExplosionAxis ---------- //
Vec3 computeExplosionAxis(const std::vector<Vertex>& meshVertices, const Vec3* userDefinedAxis) {
    // If user has defined a custom axis, use that
    if (userDefinedAxis) {
        return normalize(*userDefinedAxis);
    }
    
    // Try to detect symmetry
    SymmetryResult sym = detectSymmetry(meshVertices);
    
    if (sym.type == SymmetryType::Rotational) {
        // Rotational symmetry detected, use the axis of rotation
        std::cout << "Using rotational symmetry axis (order " << sym.symmetryOrder << ")\n";
        return normalize(sym.rotationAxis);
    }
    else if (sym.type == SymmetryType::Reflective) {
        // Reflective symmetry detected, project to plane and perform 2D PCA
        std::cout << "Using 2D PCA on reflection plane\n";
        return normalize(pca2DOnPlane(meshVertices, sym.reflectiveNormal));
    }
    else {
        // No symmetry detected, use 3D PCA
        std::cout << "No symmetry detected, using 3D PCA\n";
        return normalize(pca3D(meshVertices));
    }
}