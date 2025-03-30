#ifndef MC_MITRA_REFLECTIVE_SYMMETRY_DETECTOR_H
#define MC_MITRA_REFLECTIVE_SYMMETRY_DETECTOR_H

#include "symmetry_detector_interface.h"
#include "vector_ops.h"

namespace MC {

// Mitra论文中的反射对称性检测实现
class MitraReflectiveSymmetryDetector : public ReflectiveSymmetryDetector {
public:
    bool detect(const std::vector<Vertex>& meshVertices, 
                Vec3& outReflectiveNormal) override;
    
    std::string getName() const override { return "Mitra Reflective Detector"; }
};

} // namespace MC

#endif // MC_REFLECTIVE_SYMMETRY_DETECTOR_H