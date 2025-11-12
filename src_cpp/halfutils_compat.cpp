#include "halfutils_compat.h"
#include "halfutils.hpp"

// C interface implementation that delegates to C++ implementation
extern "C" {

void HalfvecInit(void) {
    halfutils::HalfvecInit();
}

float HalfvecL2SquaredDistance(int dim, half* ax, half* bx) {
    return halfutils::HalfvecL2SquaredDistance(dim, ax, bx);
}

float HalfvecInnerProduct(int dim, half* ax, half* bx) {
    return halfutils::HalfvecInnerProduct(dim, ax, bx);
}

double HalfvecCosineSimilarity(int dim, half* ax, half* bx) {
    return halfutils::HalfvecCosineSimilarity(dim, ax, bx);
}

float HalfvecL1Distance(int dim, half* ax, half* bx) {
    return halfutils::HalfvecL1Distance(dim, ax, bx);
}

} // extern "C"
