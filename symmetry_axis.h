#ifndef SYMMETRY_AXIS_H
#define SYMMETRY_AXIS_H

#include <vector>
#include "marching_cubes_v3.h" // Assumes Vertex, Vec3 declarations

// Symmetry type enumeration
enum class SymmetryType {
    None,           // No symmetry detected
    Rotational,     // Rotational symmetry detected
    Reflective      // Reflective/mirror symmetry detected
};

// Structure encapsulating symmetry detection results
struct SymmetryResult {
    SymmetryType type;
    int symmetryOrder;      // Order of rotational symmetry (if type = Rotational)
    Vec3 rotationAxis;      // Axis of rotational symmetry
    Vec3 reflectiveNormal;  // Normal of the reflection plane (if type = Reflective)
};

// Main function: Compute explosion axis based on symmetry detection
// Priority:
// 1) Prefer rotational symmetry (if multiple, select highest order)
// 2) If only reflective symmetry detected, project and perform 2D PCA
// 3) If no symmetry, perform 3D PCA
// 4) Optionally allow user override
Vec3 computeExplosionAxis(const std::vector<Vertex>& meshVertices, 
                         const Vec3* userDefinedAxis = nullptr);

// ----------------- Helper Functions ----------------- //

// Detect symmetry in the mesh, returns detection result
SymmetryResult detectSymmetry(const std::vector<Vertex>& meshVertices);

// Detect rotational symmetry using transform-domain voting (Mitra et al.)
// Returns the highest order rotational symmetry found (or 1 if none)
SymmetryResult detectRotationalSymmetry(const std::vector<Vertex>& meshVertices);

// Detect reflective/mirror symmetry
// Returns the normal of the reflection plane (or nullptr if none)
SymmetryResult detectReflectiveSymmetry(const std::vector<Vertex>& meshVertices);

// Perform 3D Principal Component Analysis and return the first principal component (normalized)
Vec3 pca3D(const std::vector<Vertex>& meshVertices);

// Perform 2D PCA on vertices projected onto a plane with the given normal
// Returns the first principal component (normalized) in 3D space
Vec3 pca2DOnPlane(const std::vector<Vertex>& meshVertices, const Vec3& planeNormal);

// Compute the centroid of vertices
Vec3 computeCentroid(const std::vector<Vertex>& meshVertices);

// Project a point onto a plane defined by its normal and a point on the plane
Vec3 projectPointOntoPlane(const Vec3& point, const Vec3& planeNormal, const Vec3& pointOnPlane);

// Vector operations - declared in header for inlining potential
inline Vec3 normalize(const Vec3& v);
inline float dot(const Vec3& a, const Vec3& b);
inline Vec3 cross(const Vec3& a, const Vec3& b);

#endif // SYMMETRY_AXIS_H