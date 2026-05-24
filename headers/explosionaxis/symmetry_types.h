#ifndef SYMMETRY_TYPES_H
#define SYMMETRY_TYPES_H

#include "data.h"

namespace MC {

// define symmetry types
enum class SymmetryType {
    None,           // no symmetry detected
    Rotational,     // rotational symmetry detected
    Reflective      // reflective symmetry detected
};

// symmetry detection result structure
struct SymmetryResult {
    SymmetryType type;
    int symmetryOrder;      // rotational symmetry order (if type = Rotational)
    Vec3 rotationAxis;      // rotational symmetry axis
    Vec3 reflectiveNormal;  // reflective symmetry plane normal (if type = Reflective)
};

} // namespace MC

#endif // SYMMETRY_TYPES_H