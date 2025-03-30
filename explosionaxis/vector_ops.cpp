#include "explosionaxis/vector_ops.h"

namespace MC {
namespace VectorOps {

Vec3 computeCentroid(const std::vector<Vertex>& meshVertices) {
    Vec3 centroid(0, 0, 0);
    if (meshVertices.empty()) return centroid;
    
    for (const auto& vertex : meshVertices) {
        centroid.x += vertex.x;
        centroid.y += vertex.y;
        centroid.z += vertex.z;
    }
    
    float count = static_cast<float>(meshVertices.size());
    centroid.x /= count;
    centroid.y /= count;
    centroid.z /= count;
    
    return centroid;
}

Vec3 projectPointOntoPlane(const Vec3& point, const Vec3& planeNormal, const Vec3& pointOnPlane) {
    Vec3 v(point.x - pointOnPlane.x, point.y - pointOnPlane.y, point.z - pointOnPlane.z);
    float dist = dot(v, planeNormal);
    return Vec3(
        point.x - dist * planeNormal.x,
        point.y - dist * planeNormal.y,
        point.z - dist * planeNormal.z
    );
}

} // namespace VectorOps
} // namespace MC