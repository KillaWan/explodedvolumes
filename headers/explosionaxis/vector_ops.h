#ifndef VECTOR_OPS_H
#define VECTOR_OPS_H

#include <vector>
#include <cmath>
#include "data.h"

namespace MC {

// vector operation functions
namespace VectorOps {
    inline Vec3 normalize(const Vec3& v) {
        float len = std::sqrt(v.x*v.x + v.y*v.y + v.z*v.z);
        return (len < 1e-12f) ? Vec3(0,0,1) : Vec3(v.x/len, v.y/len, v.z/len);
    }

    inline float dot(const Vec3& a, const Vec3& b) {
        return a.x*b.x + a.y*b.y + a.z*b.z;
    }

    inline Vec3 cross(const Vec3& a, const Vec3& b) {
        return Vec3(a.y*b.z - a.z*b.y,
                    a.z*b.x - a.x*b.z,
                    a.x*b.y - a.y*b.x);
    }

    Vec3 computeCentroid(const std::vector<Vertex>& meshVertices);
    Vec3 projectPointOntoPlane(const Vec3& point, const Vec3& planeNormal, const Vec3& pointOnPlane);
}

} // namespace MC

#endif // VECTOR_OPS_H