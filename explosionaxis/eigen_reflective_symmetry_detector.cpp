#include "explosionaxis/eigen_reflective_symmetry_detector.h"
#include <iostream>
#include <random>
#include <algorithm>
#include <cmath>

namespace MC {

using namespace VectorOps;

EigenReflectiveSymmetryDetector::EigenReflectiveSymmetryDetector(float distanceTolerance)
    : m_distanceTolerance(distanceTolerance) {
}

bool EigenReflectiveSymmetryDetector::detect(
    const std::vector<Vertex>& meshVertices,
    Vec3& outReflectiveNormal) {
    
    if (meshVertices.empty()) {
        std::cout << "Empty mesh provided to detector" << std::endl;
        return false;
    }
    
    std::cout << "Detecting reflective symmetry using Eigen..." << std::endl;
    
    // Compute centroid
    Vec3 center = computeCentroid(meshVertices);
    Eigen::Vector3f centroid(center.x, center.y, center.z);
    
    // Create points matrix for PCA
    Eigen::MatrixXf points(meshVertices.size(), 3);
    for (size_t i = 0; i < meshVertices.size(); i++) {
        points(i, 0) = meshVertices[i].x - centroid(0);
        points(i, 1) = meshVertices[i].y - centroid(1);
        points(i, 2) = meshVertices[i].z - centroid(2);
    }
    
    // Compute covariance matrix
    Eigen::Matrix3f covariance = (points.transpose() * points) / static_cast<float>(points.rows());
    
    // Compute eigenvectors
    Eigen::SelfAdjointEigenSolver<Eigen::Matrix3f> eigensolver(covariance);
    
    if (eigensolver.info() != Eigen::Success) {
        std::cout << "Eigenvalue computation failed" << std::endl;
        return false;
    }
    
    // Get the principal axes as potential symmetry plane normals
    std::vector<Eigen::Vector3f> planeNormals;
    planeNormals.push_back(eigensolver.eigenvectors().col(0));
    planeNormals.push_back(eigensolver.eigenvectors().col(1));
    planeNormals.push_back(eigensolver.eigenvectors().col(2));
    
    // Also check standard axes
    planeNormals.push_back(Eigen::Vector3f(1, 0, 0));
    planeNormals.push_back(Eigen::Vector3f(0, 1, 0));
    planeNormals.push_back(Eigen::Vector3f(0, 0, 1));
    
    float bestScore = 0.0f;
    Eigen::Vector3f bestNormal;
    
    // Evaluate each plane through the centroid
    for (const auto& normal : planeNormals) {
        float score = evaluateSymmetryScore(meshVertices, normal.normalized(), centroid);
        if (score > bestScore) {
            bestScore = score;
            bestNormal = normal.normalized();
        }
    }
    
    // If we found a good candidate
    if (bestScore > 0.7f) { // 70% threshold
        outReflectiveNormal = Vec3(bestNormal(0), bestNormal(1), bestNormal(2));
        std::cout << "Detected reflective symmetry with normal: (" 
                  << outReflectiveNormal.x << ", " << outReflectiveNormal.y << ", " << outReflectiveNormal.z
                  << "), score: " << bestScore << std::endl;
        return true;
    }
    
    std::cout << "No reflective symmetry detected" << std::endl;
    return false;
}

float EigenReflectiveSymmetryDetector::evaluateSymmetryScore(
    const std::vector<Vertex>& vertices,
    const Eigen::Vector3f& planeNormal,
    const Eigen::Vector3f& planePoint) {
    
    // Sample a subset of vertices (max 200 for efficiency)
    int sampleSize = std::min(200, static_cast<int>(vertices.size()));
    int stepSize = static_cast<int>(vertices.size()) / sampleSize;
    stepSize = std::max(1, stepSize);
    
    int matches = 0;
    int totalSampled = 0;
    
    // For each sampled vertex, check if its reflection is close to any other vertex
    for (size_t i = 0; i < vertices.size(); i += stepSize) {
        const Vertex& vertex = vertices[i];
        
        // Create point vector
        Eigen::Vector3f point(vertex.x, vertex.y, vertex.z);
        
        // Reflect the point
        Eigen::Vector3f reflected = reflectPoint(point, planeNormal, planePoint);
        
        // Find closest vertex
        float minDistance = std::numeric_limits<float>::max();
        for (const auto& v : vertices) {
            float distance = std::sqrt(
                std::pow(reflected(0) - v.x, 2) +
                std::pow(reflected(1) - v.y, 2) +
                std::pow(reflected(2) - v.z, 2)
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

Eigen::Vector3f EigenReflectiveSymmetryDetector::reflectPoint(
    const Eigen::Vector3f& point,
    const Eigen::Vector3f& planeNormal,
    const Eigen::Vector3f& planePoint) {
    
    // Calculate reflection using the reflection formula
    Eigen::Vector3f toPoint = point - planePoint;
    float projection = toPoint.dot(planeNormal);
    
    return point - 2.0f * projection * planeNormal;
}

} // namespace MC