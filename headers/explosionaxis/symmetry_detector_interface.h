#ifndef MC_SYMMETRY_DETECTOR_INTERFACE_H
#define MC_SYMMETRY_DETECTOR_INTERFACE_H

#include "data.h"
#include <vector>

namespace MC
{

    // RotationalSymmetryDetector
    class RotationalSymmetryDetector
    {
    public:
        virtual ~RotationalSymmetryDetector() = default;
        virtual bool detect(const std::vector<Vertex> &meshVertices,
                            Vec3 &outRotationAxis,
                            int &outSymmetryOrder) = 0;

        virtual std::string getName() const = 0;
    };

    // ReflectiveSymmetryDetector
    class ReflectiveSymmetryDetector
    {
    public:
        virtual ~ReflectiveSymmetryDetector() = default;
        virtual bool detect(const std::vector<Vertex> &meshVertices,
                            Vec3 &outReflectiveNormal) = 0;

        virtual std::string getName() const = 0;
    };

}

#endif // MC_SYMMETRY_DETECTOR_INTERFACE_H