#ifndef MC_EIGEN_REFLECTIVE_SYMMETRY_DETECTOR_H
#define MC_EIGEN_REFLECTIVE_SYMMETRY_DETECTOR_H

#include "symmetry_detector_interface.h"
#include "vector_ops.h"
#include <Eigen/Dense>
#include <vector>

namespace MC {

// Pure Eigen-based reflective symmetry detector
class EigenReflectiveSymmetryDetector : public ReflectiveSymmetryDetector {
public:
    EigenReflectiveSymmetryDetector(float distanceTolerance = 0.05f);
    ~EigenReflectiveSymmetryDetector() = default;
    
    // Detect reflective symmetry according to the interface
    bool detect(const std::vector<Vertex>& meshVertices, 
                Vec3& outReflectiveNormal) override;
                
    std::string getName() const override { return "Eigen Reflective Detector"; }

private:
    // Check if a plane is a symmetry plane
    float evaluateSymmetryScore(const std::vector<Vertex>& vertices,
                               const Eigen::Vector3f& planeNormal,
                               const Eigen::Vector3f& planePoint);
                               
    // Reflect a point across a plane
    Eigen::Vector3f reflectPoint(const Eigen::Vector3f& point,
                                const Eigen::Vector3f& planeNormal,
                                const Eigen::Vector3f& planePoint);
    
    // Configuration parameters
    float m_distanceTolerance;
};

} // namespace MC

#endif // MC_EIGEN_REFLECTIVE_SYMMETRY_DETECTOR_H