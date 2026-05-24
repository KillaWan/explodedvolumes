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
    Vec3& outAxis) {
    
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
    
    // store eigenvectors and their corresponding eigenvalues
    std::vector<std::pair<float, Vec3>> eigenVectorsWithValues;
    
    // store eigenvectors and their corresponding eigenvalues
    for (int i = 0; i < 3; i++) {
        Vec3 eigenvector(
            eigensolver.eigenvectors()(0, i),
            eigensolver.eigenvectors()(1, i),
            eigensolver.eigenvectors()(2, i)
        );
        float eigenvalue = eigensolver.eigenvalues()(i);
        eigenVectorsWithValues.push_back({eigenvalue, normalize(eigenvector)});
    }
    
    // sort by eigenvalue magnitude (descending)
    std::sort(eigenVectorsWithValues.begin(), eigenVectorsWithValues.end(),
        [](const auto& a, const auto& b) { return a.first > b.first; });
    
    // try the eigenvectors as potential symmetry plane normals
    for (int i = 0; i < 3; i++) {
        Vec3 planeNormal = eigenVectorsWithValues[i].second;
        if (checkSymmetryPlane(meshVertices, planeNormal, centroid)) {
            // for the plane defined by this normal, we need to determine the two axes on the plane
            int normalIdx = i;
            int axis1Idx = (normalIdx + 1) % 3;
            int axis2Idx = (normalIdx + 2) % 3;
            
            // Choose the axis with the larger eigenvalue on the plane
            if (eigenVectorsWithValues[axis1Idx].first > eigenVectorsWithValues[axis2Idx].first) {
                outAxis = eigenVectorsWithValues[axis1Idx].second;
            } else {
                outAxis = eigenVectorsWithValues[axis2Idx].second;
            }
            
            std::cout << "Detected reflective symmetry plane with normal: (" 
                      << planeNormal.x << ", " << planeNormal.y << ", " << planeNormal.z 
                      << ")" << std::endl;
                      
            std::cout << "Returning larger axis on the symmetry plane: ("
                      << outAxis.x << ", " << outAxis.y << ", " << outAxis.z
                      << ")" << std::endl;
                      
            return true;
        }
    }
    
    // try standard planes
    std::vector<Vec3> standardNormals = {
        Vec3(1, 0, 0),
        Vec3(0, 1, 0),
        Vec3(0, 0, 1)
    };
    
    for (const auto& planeNormal : standardNormals) {
        if (checkSymmetryPlane(meshVertices, planeNormal, centroid)) {
            // for the standard plane, we need to determine the two axes on the plane
            // for example, if the normal is (1,0,0), then the axes on the plane are (0,1,0) and (0,0,1)
            Vec3 axis1, axis2;
            
            if (std::abs(planeNormal.x) > 0.9f) {
                // YZ plane
                axis1 = Vec3(0, 1, 0);
                axis2 = Vec3(0, 0, 1);
            } else if (std::abs(planeNormal.y) > 0.9f) {
                // XZ plane
                axis1 = Vec3(1, 0, 0);
                axis2 = Vec3(0, 0, 1);
            } else {
                // XY plane
                axis1 = Vec3(1, 0, 0);
                axis2 = Vec3(0, 1, 0);
            }
            
            // calculate the variance along each axis to determine the larger one
            float variance1 = 0.0f, variance2 = 0.0f;
            
            for (const auto& vertex : meshVertices) {
                Vec3 v(vertex.x - centroid.x, vertex.y - centroid.y, vertex.z - centroid.z);
                float proj1 = dot(v, axis1);
                float proj2 = dot(v, axis2);
                variance1 += proj1 * proj1;
                variance2 += proj2 * proj2;
            }
            
            variance1 /= meshVertices.size();
            variance2 /= meshVertices.size();
            
            // choose the axis with the larger variance as the explosion axis
            outAxis = (variance1 > variance2) ? axis1 : axis2;
            
            std::cout << "Detected reflective symmetry with standard normal: (" 
                      << planeNormal.x << ", " << planeNormal.y << ", " << planeNormal.z 
                      << ")" << std::endl;
                      
            std::cout << "Returning larger axis on the symmetry plane: ("
                      << outAxis.x << ", " << outAxis.y << ", " << outAxis.z
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
    int totalTests = std::min(m_sampleCount, static_cast<int>(meshVertices.size()));
    float tolerance = 1.0f; // Distance tolerance for match
    
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
    return (matchPercentage > 0.5f); // 70% threshold
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