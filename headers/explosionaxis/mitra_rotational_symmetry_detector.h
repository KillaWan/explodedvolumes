#ifndef MC_MITRA_ROTATIONAL_SYMMETRY_DETECTOR_H
#define MC_MITRA_ROTATIONAL_SYMMETRY_DETECTOR_H

#include "symmetry_detector_interface.h"
#include "vector_ops.h"

namespace MC {

// Mitra论文中的旋转对称性检测实现
class MitraRotationalSymmetryDetector : public RotationalSymmetryDetector {
public:
    bool detect(const std::vector<Vertex>& meshVertices, 
                Vec3& outRotationAxis, 
                int& outSymmetryOrder) override;
    
    std::string getName() const override { return "Mitra Rotational Detector"; }
};

} // namespace MC

#endif // MC_ROTATIONAL_SYMMETRY_DETECTOR_H