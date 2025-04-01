#ifndef MC_SIMPLE_PCL_REFLECTIVE_SYMMETRY_DETECTOR_H
#define MC_SIMPLE_PCL_REFLECTIVE_SYMMETRY_DETECTOR_H

#include "symmetry_detector_interface.h"
#include "vector_ops.h"
#include <vector>

namespace MC {

// Simple PCL-based reflective symmetry detector
class SimplePCLReflectiveSymmetryDetector : public ReflectiveSymmetryDetector {
public:
    SimplePCLReflectiveSymmetryDetector();
    ~SimplePCLReflectiveSymmetryDetector() = default;

    void setSampleCount(int count) { m_sampleCount = count; }
    void useCustomAxis(bool use) { m_useCustomAxis = use; }
    void setCustomAxis(const Vec3& axis) { m_customAxis = axis; }
    
    // Detect reflective symmetry using simple PCA-based methods
    bool detect(const std::vector<Vertex>& meshVertices, 
                Vec3& outReflectiveNormal) override;
                
    std::string getName() const override { return "Simple PCL Reflective Detector"; }

private:
    int m_sampleCount = 100;
    bool m_useCustomAxis = false;
    Vec3 m_customAxis = {0.0f, 0.0f, 1.0f};
    // Helper method to check if a plane is a symmetry plane
    bool checkSymmetryPlane(const std::vector<Vertex>& meshVertices,
                         const Vec3& planeNormal,
                         const Vec3& planePoint);
                        
    // Helper method to reflect a point across a plane
    Vec3 reflectPointAcrossPlane(const Vec3& point, 
                              const Vec3& planeNormal, 
                              const Vec3& planePoint);
};

} // namespace MC

#endif // MC_SIMPLE_PCL_REFLECTIVE_SYMMETRY_DETECTOR_H