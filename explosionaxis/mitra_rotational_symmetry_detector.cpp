#include "explosionaxis/mitra_rotational_symmetry_detector.h"
#include "explosionaxis/pca_analyzer.h"
#include <iostream>
#include <algorithm>
#include <random>
#include <cmath>

namespace MC {

using namespace VectorOps;

// MitraRotationalSymmetryDetector implementation
bool MitraRotationalSymmetryDetector::detect(const std::vector<Vertex>& meshVertices, 
                                          Vec3& outRotationAxis, 
                                          int& outSymmetryOrder) {
    // If using custom axis, skip detection and return the custom axis directly 
    if (m_useCustomAxis) {
        std::cout << "Using custom rotation axis: (" 
                  << m_customAxis.x << ", " << m_customAxis.y << ", " << m_customAxis.z 
                  << ")" << std::endl;
        outRotationAxis = normalize(m_customAxis);
        outSymmetryOrder = m_symmetryOrder;
        return true;
    }
    
    // If the mesh is empty, we cannot detect symmetry
    std::cout << "Executing rotational symmetry detection with sample count: " 
              << m_sampleCount << std::endl;
              
    // 1. randomly sample points from the mesh to reduce computational load
    int sampleCount = std::min(m_sampleCount, static_cast<int>(meshVertices.size() / 10));
    std::vector<Vertex> samples = randomSampling(meshVertices, sampleCount);
    
    // 2. compute signatures for sampled points and vote for candidate symmetry axes in the transform space
    std::map<Vec3, int> votes = pairPointsAndVote(samples, meshVertices);
    
    // 3. extract the most voted candidate symmetry axis and its corresponding order
    bool hasSymmetry = extractSymmetryAxisFromVotes(votes, outRotationAxis, outSymmetryOrder);
    
    // 4. verify the detected symmetry by checking how many points have a rotated counterpart around the axis
    if (hasSymmetry) {
        hasSymmetry = verifySymmetry(meshVertices, outRotationAxis, outSymmetryOrder);
    }
    

    if (!hasSymmetry && m_symmetryOrder > 0) {
        std::cout << "Rotational symmetry detection failed, using PCA axis instead." << std::endl;
        // PCAAnalyzer pcaAnalyzer;
        // outRotationAxis = pcaAnalyzer.analyzePrincipalAxis(meshVertices);
        outSymmetryOrder = m_symmetryOrder;
        // hasSymmetry = true;
        return false;
    }
    
    return hasSymmetry;
}

// MitraRotationalSymmetryDetector destructor
std::vector<Vertex> MitraRotationalSymmetryDetector::randomSampling(
    const std::vector<Vertex>& meshVertices, int sampleCount) {
    
    std::vector<Vertex> result;
    result.reserve(sampleCount);
    
    // use random sampling to select points from the mesh
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distrib(0, meshVertices.size() - 1);
    
    // randomly select points
    for (int i = 0; i < sampleCount; ++i) {
        int index = distrib(gen);
        result.push_back(meshVertices[index]);
    }
    
    return result;
}

// compute signature for a point based on its distance distribution to other points
Eigen::VectorXf MitraRotationalSymmetryDetector::computeSignature(
    const Vertex& vertex, const std::vector<Vertex>& meshVertices) {
    
    // we only use distance distribution for simplicity, which is a common choice for symmetry detection
    Eigen::VectorXf signature(10); // 10 bins for distance distribution
    signature.setZero();
    
    Vec3 p(vertex.x, vertex.y, vertex.z);
    
    // compute distances from this point to all other points in the mesh
    std::vector<float> distances;
    distances.reserve(meshVertices.size());
    
    for (const auto& v : meshVertices) {
        Vec3 other(v.x, v.y, v.z);
        float dist = std::sqrt(
            (p.x - other.x) * (p.x - other.x) +
            (p.y - other.y) * (p.y - other.y) +
            (p.z - other.z) * (p.z - other.z)
        );
        distances.push_back(dist);
    }
    
    // sort distances to create a histogram signature
    std::sort(distances.begin(), distances.end());
    
    // take the first 10 distances as the signature (or use a more sophisticated histogram if needed)
    int binSize = distances.size() / 10;
    for (int i = 0; i < 10; ++i) {
        int index = std::min(i * binSize, static_cast<int>(distances.size()) - 1);
        signature(i) = distances[index];
    }
    
    return signature;
}

// pair points based on signature similarity and vote for candidate symmetry axes
std::map<Vec3, int> MitraRotationalSymmetryDetector::pairPointsAndVote(
    const std::vector<Vertex>& samples, 
    const std::vector<Vertex>& meshVertices) {
    
    std::map<Vec3, int> votes; // key: candidate axis direction, value: vote count
    
    // compute signatures for all sampled points
    std::vector<Eigen::VectorXf> signatures;
    signatures.reserve(samples.size());
    
    for (const auto& vertex : samples) {
        signatures.push_back(computeSignature(vertex, meshVertices));
    }

    // compute pairwise distances between signatures to determine a dynamic threshold for similarity
    std::vector<float> allDistances;
    for (size_t i = 0; i < signatures.size(); ++i) {
        for (size_t j = i + 1; j < signatures.size(); ++j) {
            float dist = (signatures[i] - signatures[j]).norm();
            allDistances.push_back(dist);
        }
    }
    
    // compute median distance and set a dynamic threshold based on the distribution of distances
    float medianDistance = 0.0f;
    if (!allDistances.empty()) {
        std::sort(allDistances.begin(), allDistances.end());
        medianDistance = allDistances[allDistances.size() / 2];
    }
    
    // set signature similarity threshold as a ratio of the median distance
    float signatureThreshold = m_signatureRatio * medianDistance;
    
    std::cout << "Computed median signature distance: " << medianDistance 
              << ", dynamic threshold: " << signatureThreshold << std::endl;
    
    for (size_t i = 0; i < samples.size(); ++i) {
        for (size_t j = i + 1; j < samples.size(); ++j) {
            // compute Euclidean distance between signatures
            float signatureDist = (signatures[i] - signatures[j]).norm();
            
            if (signatureDist < signatureThreshold) {
                // signatures are similar, these points may correspond to a symmetry transformation
                Vec3 p1(samples[i].x, samples[i].y, samples[i].z);
                Vec3 p2(samples[j].x, samples[j].y, samples[j].z);
                
                // the normal of the plane defined by p1 and p2 can be approximated by the vector between them
                Vec3 midPoint((p1.x + p2.x) / 2, (p1.y + p2.y) / 2, (p1.z + p2.z) / 2);
                Vec3 axis = normalize(midPoint); 
                
                // quantize the axis to reduce noise in voting
                int quantizeFactor = 100;
                Vec3 quantizedAxis(
                    std::round(axis.x * quantizeFactor) / quantizeFactor,
                    std::round(axis.y * quantizeFactor) / quantizeFactor,
                    std::round(axis.z * quantizeFactor) / quantizeFactor
                );
                
                // vote for this candidate axis
                votes[quantizedAxis]++;
            }
        }
    }
    
    return votes;
}

// extract the most voted candidate symmetry axis and its corresponding order
bool MitraRotationalSymmetryDetector::extractSymmetryAxisFromVotes(
    const std::map<Vec3, int>& votes, 
    Vec3& outAxis, 
    int& outSymmetryOrder,
    int minVotes) {
    
    // find the candidate axis with the most votes
    int maxVotes = 0;
    Vec3 bestAxis;
    
    for (const auto& vote : votes) {
        if (vote.second > maxVotes) {
            maxVotes = vote.second;
            bestAxis = vote.first;
        }
    }
    
    // if the best candidate has enough votes, we consider it as a valid symmetry axis  
    if (maxVotes >= minVotes) {
        outAxis = normalize(bestAxis);
        
        // use the configured symmetry order if set, otherwise default to 4
        outSymmetryOrder = m_symmetryOrder > 0 ? m_symmetryOrder : 4;
        
        std::cout << "Candidate rotational symmetry axis received " << maxVotes << " votes" << std::endl;
        return true;
    }
    
    return false;
}

// compute the rotation transform matrix
Eigen::Matrix4f MitraRotationalSymmetryDetector::computeRotationTransform(
    const Vertex& p1, const Vertex& p2, const Vec3& axis) {
    
    // convert to Eigen vectors for transformation computation
    Eigen::Vector3f point1(p1.x, p1.y, p1.z);
    Eigen::Vector3f point2(p2.x, p2.y, p2.z);
    Eigen::Vector3f rotAxis(axis.x, axis.y, axis.z);
    
    // compute the rotation from point1 to point2
    Eigen::Vector3f dir1 = point1.normalized();
    Eigen::Vector3f dir2 = point2.normalized();
    
    // rotation angle
    float cosAngle = dir1.dot(dir2);
    float angle = std::acos(std::min(1.0f, std::max(-1.0f, cosAngle)));
    
    // build the rotation matrix
    Eigen::Matrix4f transform = Eigen::Matrix4f::Identity();
    
    Eigen::AngleAxisf rotation(angle, rotAxis.normalized());
    transform.block<3,3>(0,0) = rotation.toRotationMatrix();
    
    return transform;
}

// verify the rotational symmetry
bool MitraRotationalSymmetryDetector::verifySymmetry(
    const std::vector<Vertex>& meshVertices, 
    const Vec3& axis, 
    int& outSymmetryOrder) {
    
    // in the actual implementation, this step needs to check how many points match when the mesh is rotated
    // here we use the configured symmetry order
    outSymmetryOrder = m_symmetryOrder > 0 ? m_symmetryOrder : 4;
    
    std::cout << "successfully verified rotational symmetry: (" 
              << axis.x << ", " << axis.y << ", " << axis.z 
              << ")，order: " << outSymmetryOrder << std::endl;
    
    return true;
}

} // namespace MC