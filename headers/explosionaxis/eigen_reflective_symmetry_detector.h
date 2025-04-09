#ifndef MC_EIGEN_REFLECTIVE_SYMMETRY_DETECTOR_H
#define MC_EIGEN_REFLECTIVE_SYMMETRY_DETECTOR_H

#include "symmetry_detector_interface.h"
#include "vector_ops.h"
#include <Eigen/Dense>
#include <vector>

namespace MC
{
    class EigenReflectiveSymmetryDetector : public ReflectiveSymmetryDetector
    {
    public:
        EigenReflectiveSymmetryDetector(float distanceTolerance = 0.05f);
        ~EigenReflectiveSymmetryDetector() = default;

        bool detect(const std::vector<Vertex> &meshVertices,
                    Vec3 &outReflectiveNormal) override;

        std::string getName() const override { return "Eigen Reflective Detector"; }

    private:
        float evaluateSymmetryScore(const std::vector<Vertex> &vertices,
                                    const Eigen::Vector3f &planeNormal,
                                    const Eigen::Vector3f &planePoint);

        Eigen::Vector3f reflectPoint(const Eigen::Vector3f &point,
                                     const Eigen::Vector3f &planeNormal,
                                     const Eigen::Vector3f &planePoint);

        float m_distanceTolerance;
    };
}

#endif // MC_EIGEN_REFLECTIVE_SYMMETRY_DETECTOR_H