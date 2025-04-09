#pragma once

#include "data.h"
#include <vector>
#include <map>
#include <Eigen/Dense>

namespace MC
{
    class MitraReflectiveSymmetryDetector
    {
    public:
        MitraReflectiveSymmetryDetector()
            : m_sampleCount(100), m_useCustomNormal(false), m_cosineThreshold(0.95f)
        {
        }

        bool detect(const std::vector<Vertex> &meshVertices, Vec3 &outReflectiveNormal);
        void setSampleCount(int count) { m_sampleCount = count; }
        void useCustomNormal(bool use) { m_useCustomNormal = use; }
        void setCustomNormal(const Vec3 &normal) { m_customNormal = normal; }
        void setCosineThreshold(float threshold) { m_cosineThreshold = threshold; }

    private:
        int m_sampleCount;
        bool m_useCustomNormal;
        Vec3 m_customNormal;
        float m_cosineThreshold;

        std::vector<Vertex> randomSampling(const std::vector<Vertex> &meshVertices, int sampleCount);
        Eigen::VectorXf computeSignature(const Vertex &vertex, const std::vector<Vertex> &meshVertices);
        std::map<Vec3, int> pairPointsAndVote(const std::vector<Vertex> &samples, const std::vector<Vertex> &meshVertices);
        bool extractSymmetryPlaneFromVotes(const std::map<Vec3, int> &votes, Vec3 &outNormal, int minVotes = 10);
        bool verifySymmetry(const std::vector<Vertex> &meshVertices, const Vec3 &normal);
        Eigen::Matrix4f computeReflectionTransform(const Vertex &p1, const Vertex &p2);
    };
}
