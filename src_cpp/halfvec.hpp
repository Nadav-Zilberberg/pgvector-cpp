#ifndef HALFVEC_HPP
#define HALFVEC_HPP

#define __STDC_WANT_IEC_60559_TYPES_EXT__

#include <float.h>
#include <cstdint>
#include <cstddef>

/* We use two types of dispatching: intrinsics and target_clones */
/* TODO Move to better place */
#ifndef DISABLE_DISPATCH
/* Only enable for more recent compilers to keep build process simple */
#if defined(__x86_64__) && defined(__GNUC__) && __GNUC__ >= 9
#define USE_DISPATCH
#elif defined(__x86_64__) && defined(__clang_major__) && __clang_major__ >= 7
#define USE_DISPATCH
#elif defined(_M_AMD64) && defined(_MSC_VER) && _MSC_VER >= 1920
#define USE_DISPATCH
#endif
#endif

/* target_clones requires glibc */
#if defined(USE_DISPATCH) && defined(__gnu_linux__) && defined(__has_attribute)
/* Use separate line for portability */
#if __has_attribute(target_clones)
#define USE_TARGET_CLONES
#endif
#endif

/* Apple clang check needed for universal binaries on Mac */
#if defined(USE_DISPATCH) && (defined(HAVE__GET_CPUID) || defined(__apple_build_version__))
#define USE__GET_CPUID
#endif

#if defined(USE_DISPATCH)
#define HALFVEC_DISPATCH
#endif

/* F16C has better performance than _Float16 (on x86-64) */
#if defined(__F16C__)
#define F16C_SUPPORT
#elif defined(__FLT16_MAX__) && !defined(HALFVEC_DISPATCH) && !defined(__FreeBSD__) && (!defined(__i386__) || defined(__SSE2__))
#define FLT16_SUPPORT
#endif

namespace halfvec {

#ifdef FLT16_SUPPORT
using half = _Float16;
constexpr auto HALF_MAX = FLT16_MAX;
#else
using half = std::uint16_t;
constexpr auto HALF_MAX = 65504;
#endif

constexpr std::size_t HALFVEC_MAX_DIM = 16000;

// Template class for flexible half type support
template<typename T = half>
class HalfVector {
public:
    int32_t vl_len_;           // varlena header (do not touch directly!)
    int16_t dim;               // number of dimensions
    int16_t unused;            // reserved for future use, always zero
    T x[];                     // flexible array member

    // Constructor
    HalfVector() : vl_len_(0), dim(0), unused(0) {}

    // Inline functions replacing macros
    static std::size_t size(std::size_t dim) {
        return offsetof(HalfVector, x) + sizeof(T) * dim;
    }

    static HalfVector* from_datum(void* datum) {
        return reinterpret_cast<HalfVector*>(PG_DETOAST_DATUM(datum));
    }

    static HalfVector* from_arg(int n) {
        return from_datum(PG_GETARG_DATUM(n));
    }

    static Datum return_pointer(HalfVector* vec) {
        return PG_RETURN_POINTER(vec);
    }
};

// Type alias for default half type
using HalfVector_p = HalfVector<>;

// Function declaration
HalfVector_p* InitHalfVector(int dim);

} // namespace halfvec

// C++ versions of the C macros
#define DatumGetHalfVector(x) (halfvec::HalfVector<>::from_datum(x))
#define PG_GETARG_HALFVEC_P(x) (halfvec::HalfVector<>::from_arg(x))
#define PG_RETURN_HALFVEC_P(x) (halfvec::HalfVector<>::return_pointer(x))

#endif // HALFVEC_HPP
