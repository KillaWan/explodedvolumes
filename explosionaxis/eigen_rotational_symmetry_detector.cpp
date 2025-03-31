#include "explosionaxis/eigen_rotational_symmetry_detector.h"
#include <iostream>
#include <random>
#include <algorithm>
#include <cmath>
#include <set>

namespace MC {

using namespace VectorOps;

EigenRotationalSymmetryDetector::EigenRotationalSymmetryDetector(
    float angleTolerance, float distanceTolerance)
    : m_angleTolerance(angleTolerance),
      m_distanceTolerance(distanceTolerance) {
}

bool EigenRotationalSymmetryDetector::detect(
    const std::vector<Vertex>& meshVertices,
    Vec3& outRotationAxis,
    int& outSymmetryOrder) {
    
    if (meshVertices.empty()) {
        std::cout << "Empty mesh provided to detector" << std::endl;
        return false;
    }
    
    std::cout << "Detecting rotational symmetry using Eigen..." << std::endl;
    
    // Compute centroid
    Vec3 centroid = computeCentroid(meshVertices);
    
    // Create points matrix for PCA
    Eigen::MatrixXf points(meshVertices.size(), 3);
    for (size_t i = 0; i < meshVertices.size(); i++) {
        points(i, 0) = meshVertices[i].x - centroid.x;
        points(i, 1) = meshVertices[i].y - centroid.y;
        points(i, 2) = meshVertices[i].z - centroid.z;
    }
    
    // Compute covariance matrix
    Eigen::Matrix3f covariance = (points.transpose() * points) / static_cast<float>(points.rows());
    
    // Compute eigenvectors
    Eigen::SelfAdjointEigenSolver<Eigen::Matrix3f> eigensolver(covariance);
    
    if (eigensolver.info() != Eigen::Success) {
        std::cout << "Eigenvalue computation failed" << std::endl;
        return false;
    }
    
    // Get the three principal axes as potential rotation axes
    std::vector<Eigen::Vector3f> axesCandidates;
    axesCandidates.push_back(eigensolver.eigenvectors().col(0));
    axesCandidates.push_back(eigensolver.eigenvectors().col(1));
    axesCandidates.push_back(eigensolver.eigenvectors().col(2));
    
    // Add standard axes as candidates
    axesCandidates.push_back(Eigen::Vector3f(1, 0, 0));
    axesCandidates.push_back(Eigen::Vector3f(0, 1, 0));
    axesCandidates.push_back(Eigen::Vector3f(0, 0, 1));
    
    // Orders to check (most common)
    std::vector<int> ordersToCheck = {2, 3, 4, 5, 6, 8};
    
    float bestScore = 0.0f;
    Eigen::Vector3f bestAxis;
    int bestOrder = 0;
    
    // Evaluate each axis with each order
    for (const auto& axis : axesCandidates) {
        for (int order : ordersToCheck) {
            float score = evaluateSymmetryScore(meshVertices, axis, order);
            if (score > bestScore) {
                bestScore = score;
                bestAxis = axis;
                bestOrder = order;
            }
        }
    }
    
    // If we found a good candidate
    if (bestScore > 0.7f) { // 70% threshold
        outRotationAxis = Vec3(bestAxis(0), bestAxis(1), bestAxis(2));
        outSymmetryOrder = bestOrder;
        std::cout << "Detected rotational symmetry with axis: (" 
                  << outRotationAxis.x << ", " << outRotationAxis.y << ", " << outRotationAxis.z 
                  << "), order: " << outSymmetryOrder 
                  << ", score: " << bestScore << std::endl;
        return true;
    }
    
    std::cout << "No rotational symmetry detected" << std::endl;
    return false;
}

float EigenRotationalSymmetryDetector::evaluateSymmetryScore(
    const std::vector<Vertex>& vertices,
    const Eigen::Vector3f& axis,
    int order) {
    
    // Calculate rotation angle for this order
    float rotationAngle = 2.0f * M_PI / static_cast<float>(order);
    
    // Create the rotation matrix
    Eigen::Matrix3f rotation = createRotationMatrix(axis.normalized(), rotationAngle);
    
    // Compute centroid
    Vec3 center = computeCentroid(vertices);
    Eigen::Vector3f centroid(center.x, center.y, center.z);
    
    // Sample a subset of vertices (max 200 for efficiency)
    int sampleSize = std::min(200, static_cast<int>(vertices.size()));
    int stepSize = static_cast<int>(vertices.size()) / sampleSize;
    stepSize = std::max(1, stepSize);
    
    int matches = 0;
    int totalSampled = 0;
    
    // For each sampled vertex, check if its rotated version is close to any other vertex
    for (size_t i = 0; i < vertices.size(); i += stepSize) {
        const Vertex& vertex = vertices[i];
        
        // Create point vector relative to centroid
        Eigen::Vector3f point(
            vertex.x - centroid(0),
            vertex.y - centroid(1),
            vertex.z - centroid(2)
        );
        
        // Rotate the point
        Eigen::Vector3f rotated = rotation * point;
        
        // Add centroid back
        rotated += centroid;
        
        // Find closest vertex
        float minDistance = std::numeric_limits<float>::max();
        for (const auto& v : vertices) {
            float distance = std::sqrt(
                std::pow(rotated(0) - v.x, 2) +
                std::pow(rotated(1) - v.y, 2) +
                std::pow(rotated(2) - v.z, 2)
            );
            
            minDistance = std::min(minDistance, distance);
            
            // Early exit if we found a close enough match
            if (minDistance < m_distanceTolerance) {
                break;
            }
        }
        
        // If close enough, count as match
        if (minDistance < m_distanceTolerance) {
            matches++;
        }
        
        totalSampled++;
    }
    
    return static_cast<float>(matches) / static_cast<float>(totalSampled);
}

Eigen::Matrix3f EigenRotationalSymmetryDetector::createRotationMatrix(
    const Eigen::Vector3f& axis, float angle) {
    
    // Rodrigues' rotation formula implemented with Eigen
    Eigen::Matrix3f K;
    K << 0, -axis(2), axis(1),
         axis(2), 0, -axis(0),
         -axis(1), axis(0), 0;
    
    Eigen::Matrix3f I = Eigen::Matrix3f::Identity();
    
    return I + std::sin(angle) * K + (1 - std::cos(angle)) * (K * K);
}

} // namespace MC