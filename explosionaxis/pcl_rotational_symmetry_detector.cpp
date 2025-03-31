#include "explosionaxis/pcl_rotational_symmetry_detector.h"
#include <iostream>
#include <Eigen/Dense>
#include <cmath>

namespace MC {

using namespace VectorOps;

SimplePCLRotationalSymmetryDetector::SimplePCLRotationalSymmetryDetector() {
}

bool SimplePCLRotationalSymmetryDetector::detect(
    const std::vector<Vertex>& meshVertices,
    Vec3& outRotationAxis,
    int& outSymmetryOrder) {
    
    if (meshVertices.empty()) {
        std::cout << "Empty mesh provided to detector" << std::endl;
        return false;
    }
    
    std::cout << "Detecting rotational symmetry using Simple PCL method..." << std::endl;
    
    // Compute centroid
    Vec3 centroid = computeCentroid(meshVertices);
    
    // Construct covariance matrix manually (without relying on external PCL functions)
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
    
    // We'll try all three principal axes as potential rotation axes
    std::vector<Vec3> axisCandidate;
    axisCandidate.push_back(Vec3(
        eigensolver.eigenvectors()(0, 0),
        eigensolver.eigenvectors()(1, 0),
        eigensolver.eigenvectors()(2, 0)
    ));
    axisCandidate.push_back(Vec3(
        eigensolver.eigenvectors()(0, 1),
        eigensolver.eigenvectors()(1, 1),
        eigensolver.eigenvectors()(2, 1)
    ));
    axisCandidate.push_back(Vec3(
        eigensolver.eigenvectors()(0, 2),
        eigensolver.eigenvectors()(1, 2),
        eigensolver.eigenvectors()(2, 2)
    ));
    
    // Normalize the candidates
    for (auto& axis : axisCandidate) {
        axis = normalize(axis);
    }
    
    // Try each axis
    for (const auto& axis : axisCandidate) {
        int symmetryOrder;
        if (checkSymmetryAlong(meshVertices, axis, symmetryOrder)) {
            outRotationAxis = axis;
            outSymmetryOrder = symmetryOrder;
            std::cout << "Detected rotational symmetry with axis: (" 
                      << outRotationAxis.x << ", " << outRotationAxis.y << ", " << outRotationAxis.z 
                      << "), order: " << outSymmetryOrder << std::endl;
            return true;
        }
    }
    
    // Try standard axes
    std::vector<Vec3> standardAxes = {
        Vec3(1, 0, 0),
        Vec3(0, 1, 0),
        Vec3(0, 0, 1)
    };
    
    for (const auto& axis : standardAxes) {
        int symmetryOrder;
        if (checkSymmetryAlong(meshVertices, axis, symmetryOrder)) {
            outRotationAxis = axis;
            outSymmetryOrder = symmetryOrder;
            std::cout << "Detected rotational symmetry along standard axis: (" 
                      << outRotationAxis.x << ", " << outRotationAxis.y << ", " << outRotationAxis.z 
                      << "), order: " << outSymmetryOrder << std::endl;
            return true;
        }
    }
    
    std::cout << "No rotational symmetry detected" << std::endl;
    return false;
}

bool SimplePCLRotationalSymmetryDetector::checkSymmetryAlong(
    const std::vector<Vertex>& meshVertices,
    const Vec3& axis,
    int& symmetryOrder) {
    
    // For simplicity, we'll test common symmetry orders
    std::vector<int> ordersToCheck = {4, 6, 8, 3, 5, 2};
    
    Vec3 centroid = computeCentroid(meshVertices);
    
    for (int order : ordersToCheck) {
        float angle = 2.0f * M_PI / order;
        
        // Count how many vertices approximately match after rotation
        int matches = 0;
        int totalTests = std::min(50, static_cast<int>(meshVertices.size()));
        float tolerance = 0.1f; // Distance tolerance for match
        
        // Create rotation matrix around axis
        Eigen::Matrix3f rotation;
        float cosAngle = std::cos(angle);
        float sinAngle = std::sin(angle);
        float oneMinusCos = 1.0f - cosAngle;
        float x = axis.x, y = axis.y, z = axis.z;
        
        rotation(0, 0) = cosAngle + x * x * oneMinusCos;
        rotation(0, 1) = x * y * oneMinusCos - z * sinAngle;
        rotation(0, 2) = x * z * oneMinusCos + y * sinAngle;
        rotation(1, 0) = y * x * oneMinusCos + z * sinAngle;
        rotation(1, 1) = cosAngle + y * y * oneMinusCos;
        rotation(1, 2) = y * z * oneMinusCos - x * sinAngle;
        rotation(2, 0) = z * x * oneMinusCos - y * sinAngle;
        rotation(2, 1) = z * y * oneMinusCos + x * sinAngle;
        rotation(2, 2) = cosAngle + z * z * oneMinusCos;
        
        // Sample vertices to test
        for (int i = 0; i < totalTests; i++) {
            int idx = i * meshVertices.size() / totalTests;
            const Vertex& vertex = meshVertices[idx];
            
            // Vector from centroid to vertex
            Eigen::Vector3f point(
                vertex.x - centroid.x,
                vertex.y - centroid.y,
                vertex.z - centroid.z
            );
            
            // Rotate the point
            Eigen::Vector3f rotated = rotation * point;
            
            // Add centroid back
            rotated[0] += centroid.x;
            rotated[1] += centroid.y;
            rotated[2] += centroid.z;
            
            // Find closest vertex in original mesh
            float minDist = std::numeric_limits<float>::max();
            for (const auto& v : meshVertices) {
                float dist = std::sqrt(
                    std::pow(rotated[0] - v.x, 2) +
                    std::pow(rotated[1] - v.y, 2) +
                    std::pow(rotated[2] - v.z, 2)
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
        std::cout << "Order " << order << " symmetry check: " 
                  << matches << "/" << totalTests 
                  << " (" << (matchPercentage * 100) << "%)" << std::endl;
        
        // If enough matches, consider it symmetric
        if (matchPercentage > 0.7f) { // 70% threshold
            symmetryOrder = order;
            return true;
        }
    }
    
    return false;
}

} // namespace MC