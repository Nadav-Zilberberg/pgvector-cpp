#ifndef HALFUTILS_COMPAT_H
#define HALFUTILS_COMPAT_H

#ifdef __cplusplus
extern "C" {
#endif

// C-compatible interface for existing code
#include <stdint.h>

// Define half type for C compatibility
typedef uint16_t half;

// Function declarations (C interface)
void HalfvecInit(void);
float HalfvecL2SquaredDistance(int dim, half* ax, half* bx);
float HalfvecInnerProduct(int dim, half* ax, half* bx);
double HalfvecCosineSimilarity(int dim, half* ax, half* bx);
float HalfvecL1Distance(int dim, half* ax, half* bx);

#ifdef __cplusplus
}
#endif

#endif // HALFUTILS_COMPAT_H
