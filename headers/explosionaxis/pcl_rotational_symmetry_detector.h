#ifndef MC_SIMPLE_PCL_ROTATIONAL_SYMMETRY_DETECTOR_H
#define MC_SIMPLE_PCL_ROTATIONAL_SYMMETRY_DETECTOR_H

#include "symmetry_detector_interface.h"
#include "vector_ops.h"
#include <vector>

namespace MC {

// Simple PCL-based rotational symmetry detector
class SimplePCLRotationalSymmetryDetector : public RotationalSymmetryDetector {
public:
    SimplePCLRotationalSymmetryDetector();
    ~SimplePCLRotationalSymmetryDetector() = default;
    
    // Detect rotational symmetry using PCL's PCA
    bool detect(const std::vector<Vertex>& meshVertices, 
                Vec3& outRotationAxis, 
                int& outSymmetryOrder) override;
                
    std::string getName() const override { return "Simple PCL Rotational Detector"; }

private:
    // Helper methods
    bool checkSymmetryAlong(const std::vector<Vertex>& meshVertices, 
                          const Vec3& axis, 
                          int& symmetryOrder);
};

} // namespace MC

#endif // MC_SIMPLE_PCL_ROTATIONAL_SYMMETRY_DETECTOR_H