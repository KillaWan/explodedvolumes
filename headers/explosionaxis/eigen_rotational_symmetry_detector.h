#ifndef MC_EIGEN_ROTATIONAL_SYMMETRY_DETECTOR_H
#define MC_EIGEN_ROTATIONAL_SYMMETRY_DETECTOR_H

#include "symmetry_detector_interface.h"
#include "vector_ops.h"
#include <Eigen/Dense>
#include <vector>

namespace MC {

// Pure Eigen-based rotational symmetry detector
class EigenRotationalSymmetryDetector : public RotationalSymmetryDetector {
public:
    EigenRotationalSymmetryDetector(float angleTolerance = 0.1f, 
                                   float distanceTolerance = 0.05f);
    ~EigenRotationalSymmetryDetector() = default;
    
    // Detect rotational symmetry according to the interface
    bool detect(const std::vector<Vertex>& meshVertices, 
                Vec3& outRotationAxis, 
                int& outSymmetryOrder) override;
                
    std::string getName() const override { return "Eigen Rotational Detector"; }

private:
    // Check if a vector is a viable rotation axis with the given symmetry order
    float evaluateSymmetryScore(const std::vector<Vertex>& vertices,
                               const Eigen::Vector3f& axis,
                               int order);
                               
    // Create a rotation matrix for the given axis and angle
    Eigen::Matrix3f createRotationMatrix(const Eigen::Vector3f& axis, float angle);
    
    // Configuration parameters
    float m_angleTolerance;
    float m_distanceTolerance;
};

} // namespace MC

#endif // MC_EIGEN_ROTATIONAL_SYMMETRY_DETECTOR_H