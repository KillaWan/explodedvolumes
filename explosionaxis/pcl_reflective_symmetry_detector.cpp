#include "explosionaxis/pcl_reflective_symmetry_detector.h"
#include <iostream>
#include <Eigen/Dense>
#include <cmath>

namespace MC {

using namespace VectorOps;

SimplePCLReflectiveSymmetryDetector::SimplePCLReflectiveSymmetryDetector() {
}

bool SimplePCLReflectiveSymmetryDetector::detect(
    const std::vector<Vertex>& meshVertices,
    Vec3& outReflectiveNormal) {
    
    if (meshVertices.empty()) {
        std::cout << "Empty mesh provided to detector" << std::endl;
        return false;
    }
    
    std::cout << "Detecting reflective symmetry using Simple PCL method..." << std::endl;
    
    // Compute centroid
    Vec3 centroid = computeCentroid(meshVertices);
    
    // Construct covariance matrix manually
    Eigen::Matrix3f covMatrix = Eigen::Matrix3f::Zero();
    
    for (const auto& vertex : meshVertices) {
        Vec3 v(vertex.x - centroid.x, vertex.y - centroid.y, vertex.z - centroid.z);
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
    
    covMatrix /= meshVertices.size();
    
    // Compute eigenvectors and eigenvalues
    Eigen::SelfAdjointEigenSolver<Eigen::Matrix3f> eigensolver(covMatrix);
    if (eigensolver.info() != Eigen::Success) {
        std::cout << "Eigenvalue computation failed" << std::endl;
        return false;
    }
    
    // First, try the principal planes (planes perpendicular to principal axes)
    std::vector<Vec3> planeNormals;
    planeNormals.push_back(Vec3(
        eigensolver.eigenvectors()(0, 0),
        eigensolver.eigenvectors()(1, 0),
        eigensolver.eigenvectors()(2, 0)
    ));
    planeNormals.push_back(Vec3(
        eigensolver.eigenvectors()(0, 1),
        eigensolver.eigenvectors()(1, 1),
        eigensolver.eigenvectors()(2, 1)
    ));
    planeNormals.push_back(Vec3(
        eigensolver.eigenvectors()(0, 2),
        eigensolver.eigenvectors()(1, 2),
        eigensolver.eigenvectors()(2, 2)
    ));
    
    // Normalize the plane normals
    for (auto& normal : planeNormals) {
        normal = normalize(normal);
    }
    
    // Try each plane with centroid as plane point
    for (const auto& normal : planeNormals) {
        if (checkSymmetryPlane(meshVertices, normal, centroid)) {
            outReflectiveNormal = normal;
            std::cout << "Detected reflective symmetry with normal: (" 
                      << outReflectiveNormal.x << ", " << outReflectiveNormal.y << ", " << outReflectiveNormal.z 
                      << ")" << std::endl;
            return true;
        }
    }
    
    // Try standard planes
    std::vector<Vec3> standardNormals = {
        Vec3(1, 0, 0),
        Vec3(0, 1, 0),
        Vec3(0, 0, 1)
    };
    
    for (const auto& normal : standardNormals) {
        if (checkSymmetryPlane(meshVertices, normal, centroid)) {
            outReflectiveNormal = normal;
            std::cout << "Detected reflective symmetry with standard normal: (" 
                      << outReflectiveNormal.x << ", " << outReflectiveNormal.y << ", " << outReflectiveNormal.z 
                      << ")" << std::endl;
            return true;
        }
    }
    
    std::cout << "No reflective symmetry detected" << std::endl;
    return false;
}

bool SimplePCLReflectiveSymmetryDetector::checkSymmetryPlane(
    const std::vector<Vertex>& meshVertices,
    const Vec3& planeNormal,
    const Vec3& planePoint) {
    
    int matches = 0;
    int totalTests = std::min(50, static_cast<int>(meshVertices.size()));
    float tolerance = 0.1f; // Distance tolerance for match
    
    // Sample vertices to test
    for (int i = 0; i < totalTests; i++) {
        int idx = i * meshVertices.size() / totalTests;
        const Vertex& vertex = meshVertices[idx];
        
        Vec3 point(vertex.x, vertex.y, vertex.z);
        Vec3 reflected = reflectPointAcrossPlane(point, planeNormal, planePoint);
        
        // Find closest vertex in original mesh
        float minDist = std::numeric_limits<float>::max();
        for (const auto& v : meshVertices) {
            float dist = std::sqrt(
                std::pow(reflected.x - v.x, 2) +
                std::pow(reflected.y - v.y, 2) +
                std::pow(reflected.z - v.z, 2)
            );
            
            if (dist < minDist) {
                minDist = dist;
            }
        }
        
        if (minDist < tolerance) {
            matches++;
        }
    }
    
    float matchPercentage = static_cast<float>(matches) / totalTests;
    std::cout << "Reflective symmetry check along normal ("
              << planeNormal.x << ", " << planeNormal.y << ", " << planeNormal.z
              << "): " << matches << "/" << totalTests 
              << " (" << (matchPercentage * 100) << "%)" << std::endl;
    
    // If enough matches, consider it symmetric
    return (matchPercentage > 0.7f); // 70% threshold
}

Vec3 SimplePCLReflectiveSymmetryDetector::reflectPointAcrossPlane(
    const Vec3& point,
    const Vec3& planeNormal,
    const Vec3& planePoint) {
    
    // Calculate vector from plane point to original point
    Vec3 toPoint(point.x - planePoint.x, point.y - planePoint.y, point.z - planePoint.z);
    
    // Calculate projection of this vector onto the normal
    float projection = dot(toPoint, planeNormal);
    
    // Reflect by subtracting twice the projection from the original point
    return Vec3(
        point.x - 2 * projection * planeNormal.x,
        point.y - 2 * projection * planeNormal.y,
        point.z - 2 * projection * planeNormal.z
    );
}

} // namespace MC