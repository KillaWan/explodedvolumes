#ifndef MC_SIMPLE_PCL_ROTATIONAL_SYMMETRY_DETECTOR_H
#define MC_SIMPLE_PCL_ROTATIONAL_SYMMETRY_DETECTOR_H

#include "symmetry_detector_interface.h"
#include "vector_ops.h"
#include <vector>

namespace MC
{

    class SimplePCLRotationalSymmetryDetector : public RotationalSymmetryDetector
    {
    public:
        SimplePCLRotationalSymmetryDetector();
        ~SimplePCLRotationalSymmetryDetector() = default;

        void setSampleCount(int count) { m_sampleCount = count; }
        void setSymmetryOrder(int order) { m_symmetryOrder = order; }
        void useCustomAxis(bool use) { m_useCustomAxis = use; }
        void setCustomAxis(const Vec3 &axis) { m_customAxis = axis; }

        bool detect(const std::vector<Vertex> &meshVertices,
                    Vec3 &outRotationAxis,
                    int &outSymmetryOrder) override;

        std::string getName() const override { return "Simple PCL Rotational Detector"; }

    private:
        int m_sampleCount = 100;
        int m_symmetryOrder = 4;
        bool m_useCustomAxis = false;
        Vec3 m_customAxis = {0.0f, 0.0f, 1.0f};
        bool checkSymmetryAlong(const std::vector<Vertex> &meshVertices,
                                const Vec3 &axis,
                                int &symmetryOrder);
    };

}

#endif // MC_SIMPLE_PCL_ROTATIONAL_SYMMETRY_DETECTOR_H