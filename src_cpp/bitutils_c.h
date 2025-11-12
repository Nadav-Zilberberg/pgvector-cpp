#ifndef BITUTILS_C_H
#define BITUTILS_C_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

// Original function pointer types
typedef uint64_t (*BitHammingDistanceFunc)(uint32_t, unsigned char*, unsigned char*, uint64_t);
typedef double (*BitJaccardDistanceFunc)(uint32_t, unsigned char*, unsigned char*, uint64_t, uint64_t, uint64_t);

// Global function pointers
extern BitHammingDistanceFunc BitHammingDistance;
extern BitJaccardDistanceFunc BitJaccardDistance;

// Initialization function
void BitvecInit(void);

#ifdef __cplusplus
}
#endif

#endif // BITUTILS_C_H
