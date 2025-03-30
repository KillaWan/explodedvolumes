#ifndef MC_SYMMETRY_DETECTOR_INTERFACE_H
#define MC_SYMMETRY_DETECTOR_INTERFACE_H

#include "data.h"
#include <vector>

namespace MC {

// 旋转对称性检测器接口
class RotationalSymmetryDetector {
public:
    virtual ~RotationalSymmetryDetector() = default;
    
    // 检测旋转对称性，返回是否存在旋转对称性
    // 如果存在，通过outRotationAxis和outSymmetryOrder返回详细信息
    virtual bool detect(const std::vector<Vertex>& meshVertices, 
                        Vec3& outRotationAxis, 
                        int& outSymmetryOrder) = 0;
                        
    virtual std::string getName() const = 0;
};

// 反射对称性检测器接口
class ReflectiveSymmetryDetector {
public:
    virtual ~ReflectiveSymmetryDetector() = default;
    
    // 检测反射对称性，返回是否存在反射对称性
    // 如果存在，通过outReflectiveNormal返回反射平面法向量
    virtual bool detect(const std::vector<Vertex>& meshVertices, 
                        Vec3& outReflectiveNormal) = 0;
                        
    virtual std::string getName() const = 0;
};

} // namespace MC

#endif // MC_SYMMETRY_DETECTOR_INTERFACE_H