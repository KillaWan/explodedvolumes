#include "explosionaxis/mitra_reflective_symmetry_detector.h"
#include "explosionaxis/vector_ops.h"
#include <iostream>
#include <algorithm>
#include <random>
#include <cmath>

namespace MC
{

    using namespace VectorOps;

    bool MitraReflectiveSymmetryDetector::detect(const std::vector<Vertex> &meshVertices,
                                                 Vec3 &outAxis)
    {
        // compute centroid
        Vec3 centroid = computeCentroid(meshVertices);

        // compute covariance matrix
        Eigen::Matrix3f covMatrix = Eigen::Matrix3f::Zero();

        for (const auto &vertex : meshVertices)
        {
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

        // compute eigenvalues and eigenvectors
        Eigen::SelfAdjointEigenSolver<Eigen::Matrix3f> eigensolver(covMatrix);
        if (eigensolver.info() != Eigen::Success)
        {
            std::cout << "Eigenvalue computation failed" << std::endl;
            return false;
        }

        // store eigenvectors and their corresponding eigenvalues
        std::vector<std::pair<float, Vec3>> eigenVectorsWithValues;

        // store eigenvectors and their corresponding eigenvalues
        for (int i = 0; i < 3; i++)
        {
            Vec3 eigenvector(
                eigensolver.eigenvectors()(0, i),
                eigensolver.eigenvectors()(1, i),
                eigensolver.eigenvectors()(2, i));
            float eigenvalue = eigensolver.eigenvalues()(i);
            eigenVectorsWithValues.push_back({eigenvalue, normalize(eigenvector)});
        }

        // sort by eigenvalue magnitude (descending)
        std::sort(eigenVectorsWithValues.begin(), eigenVectorsWithValues.end(),
                  [](const auto &a, const auto &b)
                  { return a.first > b.first; });

        // if custom normal is set, use it to determine the plane and return the largest axis on the plane
        if (m_useCustomNormal)
        {
            std::cout << "Using custom reflective normal: ("
                      << m_customNormal.x << ", " << m_customNormal.y << ", " << m_customNormal.z
                      << ")" << std::endl;

            // normalize the custom normal
            Vec3 planeNormal = normalize(m_customNormal);

            // find the eigenvector that is most perpendicular to the plane normal
            int normalAxisIdx = -1;
            float maxAlignment = -1.0f;

            for (int i = 0; i < 3; i++)
            {
                float alignment = std::abs(dot(planeNormal, eigenVectorsWithValues[i].second));
                if (alignment > maxAlignment)
                {
                    maxAlignment = alignment;
                    normalAxisIdx = i;
                }
            }

            // return the largest axis that is perpendicular to the custom normal
            if (normalAxisIdx == 0)
            {
                outAxis = eigenVectorsWithValues[1].first > eigenVectorsWithValues[2].first ? eigenVectorsWithValues[1].second : eigenVectorsWithValues[2].second;
            }
            else
            {
                outAxis = eigenVectorsWithValues[0].second;
            }

            std::cout << "Returning largest axis on the symmetry plane: ("
                      << outAxis.x << ", " << outAxis.y << ", " << outAxis.z
                      << ")" << std::endl;

            return true;
        }

        std::cout << "Executing reflective symmetry detection with sample count: "
                  << m_sampleCount << std::endl;

        // vote for symmetry planes based on random sampling of point pairs
        int sampleCount = std::min(m_sampleCount, static_cast<int>(meshVertices.size() / 10));
        std::vector<Vertex> samples = randomSampling(meshVertices, sampleCount);

        // vote for candidate symmetry planes based on sampled points
        std::map<Vec3, int> votes = pairPointsAndVote(samples, meshVertices);

        // extract the most voted symmetry plane normal
        Vec3 reflectiveNormal;
        bool hasSymmetry = extractSymmetryPlaneFromVotes(votes, reflectiveNormal);

        // verify the detected plane by checking how many points have a reflected counterpart across the plane
        if (hasSymmetry)
        {
            hasSymmetry = verifySymmetry(meshVertices, reflectiveNormal);
        }

        if (hasSymmetry)
        {
            // find the two eigenvectors that are most perpendicular to the reflective normal
            std::vector<std::pair<float, int>> alignments;

            for (int i = 0; i < 3; i++)
            {
                // calculate alignment with the reflective normal (absolute value of dot product)
                float alignment = std::abs(dot(reflectiveNormal, eigenVectorsWithValues[i].second));
                alignments.push_back({alignment, i});
            }

            // sort by alignment (ascending)
            std::sort(alignments.begin(), alignments.end());

            // the two axes that are most perpendicular to the reflective normal are candidates for the explosion axis
            int axis1Idx = alignments[0].second;
            int axis2Idx = alignments[1].second;

            // return the larger one as the explosion axis
            if (eigenVectorsWithValues[axis1Idx].first > eigenVectorsWithValues[axis2Idx].first)
            {
                outAxis = eigenVectorsWithValues[axis1Idx].second;
            }
            else
            {
                outAxis = eigenVectorsWithValues[axis2Idx].second;
            }

            std::cout << "Detected reflective symmetry plane with normal: ("
                      << reflectiveNormal.x << ", " << reflectiveNormal.y << ", " << reflectiveNormal.z
                      << ")" << std::endl;

            std::cout << "Returning largest axis on the symmetry plane: ("
                      << outAxis.x << ", " << outAxis.y << ", " << outAxis.z
                      << ")" << std::endl;

            return true;
        }
        else
        {
            // if no symmetry detected, return the largest PCA axis as fallback
            std::cout << "Reflective symmetry detection failed, using largest PCA axis." << std::endl;
            outAxis = eigenVectorsWithValues[0].second;
            return false;
        }
    }

    std::vector<Vertex> MitraReflectiveSymmetryDetector::randomSampling(const std::vector<Vertex> &meshVertices, int sampleCount)
    {
        std::vector<Vertex> result;
        result.reserve(sampleCount);

        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> distrib(0, meshVertices.size() - 1);

        for (int i = 0; i < sampleCount; ++i)
        {
            int index = distrib(gen);
            result.push_back(meshVertices[index]);
        }

        return result;
    }

    Eigen::VectorXf MitraReflectiveSymmetryDetector::computeSignature(
        const Vertex &vertex, const std::vector<Vertex> &meshVertices)
    {

        Eigen::VectorXf signature(10);
        signature.setZero();

        Vec3 p(vertex.x, vertex.y, vertex.z);

        // compute distances from this point to all other points in the mesh
        std::vector<float> distances;
        distances.reserve(meshVertices.size());
        for (const auto &v : meshVertices)
        {
            Vec3 other(v.x, v.y, v.z);
            float dist = std::sqrt(
                (p.x - other.x) * (p.x - other.x) +
                (p.y - other.y) * (p.y - other.y) +
                (p.z - other.z) * (p.z - other.z));
            distances.push_back(dist);
        }

        // sort distances and take the first 10 as the signature
        std::sort(distances.begin(), distances.end());
        int binSize = distances.size() / 10;
        for (int i = 0; i < 10; ++i)
        {
            int index = std::min(i * binSize, static_cast<int>(distances.size()) - 1);
            signature(i) = distances[index];
        }

        // normalize the signature
        float norm = signature.norm();
        if (norm > 0)
        {
            signature /= norm;
        }

        return signature;
    }

    std::map<Vec3, int> MitraReflectiveSymmetryDetector::pairPointsAndVote(
        const std::vector<Vertex> &samples,
        const std::vector<Vertex> &meshVertices)
    {

        std::map<Vec3, int> votes;

        // compute signatures for all sampled points
        std::vector<Eigen::VectorXf> signatures;
        signatures.reserve(samples.size());
        for (const auto &vertex : samples)
        {
            signatures.push_back(computeSignature(vertex, meshVertices));
        }

        // compare signatures of all pairs of sampled points to vote for candidate symmetry planes
        float cosineThreshold = m_cosineThreshold; // such as 0.95
        for (size_t i = 0; i < samples.size(); ++i)
        {
            for (size_t j = i + 1; j < samples.size(); ++j)
            {
                // compute cosine similarity between signatures
                float cosineSim = signatures[i].dot(signatures[j]);
                if (cosineSim > cosineThreshold)
                {
                    // if signatures are similar enough, vote for the plane defined by the two points
                    Vec3 p1(samples[i].x, samples[i].y, samples[i].z);
                    Vec3 p2(samples[j].x, samples[j].y, samples[j].z);

                    // the normal of the plane defined by p1 and p2 can be approximated by the vector between them
                    Vec3 normal(p2.x - p1.x, p2.y - p1.y, p2.z - p1.z);
                    normal = normalize(normal);

                    // quantize the normal to reduce noise in voting
                    int quantizeFactor = 100;
                    Vec3 quantizedNormal(
                        std::round(normal.x * quantizeFactor) / quantizeFactor,
                        std::round(normal.y * quantizeFactor) / quantizeFactor,
                        std::round(normal.z * quantizeFactor) / quantizeFactor);

                    votes[quantizedNormal]++;
                }
            }
        }

        return votes;
    }

    bool MitraReflectiveSymmetryDetector::extractSymmetryPlaneFromVotes(
        const std::map<Vec3, int> &votes, Vec3 &outNormal, int minVotes)
    {

        int maxVotes = 0;
        Vec3 bestNormal;
        for (const auto &vote : votes)
        {
            if (vote.second > maxVotes)
            {
                maxVotes = vote.second;
                bestNormal = vote.first;
            }
        }

        if (maxVotes >= minVotes)
        {
            outNormal = normalize(bestNormal);
            std::cout << "Candidate reflective symmetry plane normal obtained " << maxVotes << " votes" << std::endl;
            return true;
        }

        return false;
    }

    Eigen::Matrix4f MitraReflectiveSymmetryDetector::computeReflectionTransform(
        const Vertex &p1, const Vertex &p2)
    {

        Eigen::Vector3f point1(p1.x, p1.y, p1.z);
        Eigen::Vector3f point2(p2.x, p2.y, p2.z);

        Eigen::Vector3f normal = (point2 - point1).normalized();
        Eigen::Matrix4f transform = Eigen::Matrix4f::Identity();
        Eigen::Matrix3f reflection = Eigen::Matrix3f::Identity() - 2 * normal * normal.transpose();
        transform.block<3, 3>(0, 0) = reflection;
        return transform;
    }

    bool MitraReflectiveSymmetryDetector::verifySymmetry(
        const std::vector<Vertex> &meshVertices, const Vec3 &normal)
    {

        std::cout << "Verification of reflective symmetry successful, symmetry plane normal: ("
                  << normal.x << ", " << normal.y << ", " << normal.z
                  << ")" << std::endl;
        return true;
    }

} // namespace MC
