#ifndef MC_MITRA_ROTATIONAL_SYMMETRY_DETECTOR_H
#define MC_MITRA_ROTATIONAL_SYMMETRY_DETECTOR_H

#include <vector>
#include <map>
#include <Eigen/Dense>
#include "data.h"

namespace MC
{
    class MitraRotationalSymmetryDetector
    {
    public:
        MitraRotationalSymmetryDetector() = default;
        ~MitraRotationalSymmetryDetector() = default;

        bool detect(const std::vector<Vertex> &meshVertices, Vec3 &outRotationAxis, int &outSymmetryOrder);

        void setSampleCount(int count) { m_sampleCount = count; }
        int getSampleCount() const { return m_sampleCount; }

        void setSymmetryOrder(int order) { m_symmetryOrder = order; }
        int getSymmetryOrder() const { return m_symmetryOrder; }

        void useCustomAxis(bool use) { m_useCustomAxis = use; }
        bool isUsingCustomAxis() const { return m_useCustomAxis; }

        void setCustomAxis(const Vec3 &axis) { m_customAxis = axis; }
        const Vec3 &getCustomAxis() const { return m_customAxis; }

        void configure(int sampleCount, int symmetryOrder, bool useCustomAxis, const Vec3 &customAxis)
        {
            m_sampleCount = sampleCount;
            m_symmetryOrder = symmetryOrder;
            m_useCustomAxis = useCustomAxis;
            m_customAxis = customAxis;
        }

    private:
        std::vector<Vertex> randomSampling(const std::vector<Vertex> &meshVertices, int sampleCount);

        Eigen::VectorXf computeSignature(const Vertex &vertex, const std::vector<Vertex> &meshVertices);

        std::map<Vec3, int> pairPointsAndVote(const std::vector<Vertex> &samples, const std::vector<Vertex> &meshVertices);

        bool extractSymmetryAxisFromVotes(const std::map<Vec3, int> &votes, Vec3 &outAxis, int &outSymmetryOrder, int minVotes = 3);

        Eigen::Matrix4f computeRotationTransform(const Vertex &p1, const Vertex &p2, const Vec3 &axis);

        bool verifySymmetry(const std::vector<Vertex> &meshVertices, const Vec3 &axis, int &outSymmetryOrder);

        // parameters
        int m_sampleCount = 100;
        int m_symmetryOrder = 4;
        bool m_useCustomAxis = false;
        Vec3 m_customAxis = {0.0f, 0.0f, 1.0f};
        float m_signatureRatio = 0.1f;
    };
}

#endif // MC_MITRA_ROTATIONAL_SYMMETRY_DETECTOR_H