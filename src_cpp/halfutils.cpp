#include "halfutils.hpp"
#include <algorithm>
#include <numeric>
#include <limits>
#include <cstring>

#ifdef F16C_SUPPORT
#include <immintrin.h>

#if defined(__x86_64__) && defined(__GNUC__)
#include <cpuid.h>
#elif defined(_MSC_VER)
#include <intrin.h>
#endif

#ifdef _MSC_VER
#define TARGET_F16C
#else
#define TARGET_F16C __attribute__((target("avx,f16c,fma")))
#endif
#endif

namespace halfutils {

// Static member definitions
template<typename T>
typename HalfDistanceCalculator<T>::DistanceFunction 
    HalfDistanceCalculator<T>::l2_squared_distance_func_;

template<typename T>
typename HalfDistanceCalculator<T>::DistanceFunction 
    HalfDistanceCalculator<T>::inner_product_func_;

template<typename T>
typename HalfDistanceCalculator<T>::DoubleDistanceFunction 
    HalfDistanceCalculator<T>::cosine_similarity_func_;

template<typename T>
typename HalfDistanceCalculator<T>::DistanceFunction 
    HalfDistanceCalculator<T>::l1_distance_func_;

template<typename T>
bool HalfDistanceCalculator<T>::initialized_ = false;

template<typename T>
bool HalfDistanceCalculator<T>::has_f16c_support_ = false;

// RAII Initializer implementation
template<typename T>
HalfDistanceCalculator<T>::Initializer::Initializer() {
    HalfDistanceCalculator<T>::initialize();
}

template<typename T>
HalfDistanceCalculator<T>::Initializer::~Initializer() {
    // Cleanup if needed
}

// CPU feature detection
template<typename T>
void HalfDistanceCalculator<T>::detect_cpu_features() {
#ifdef F16C_SUPPORT
    has_f16c_support_ = false;
    
#if defined(__x86_64__) && defined(__GNUC__)
    unsigned int eax, ebx, ecx, edx;
    if (__get_cpuid(1, &eax, &ebx, &ecx, &edx)) {
        has_f16c_support_ = (ecx & bit_F16C) != 0;
    }
#elif defined(_MSC_VER)
    int cpu_info[4];
    __cpuid(cpu_info, 1);
    has_f16c_support_ = (cpu_info[2] & (1 << 29)) != 0;
#else
    has_f16c_support_ = false;
#endif
    
#else
    has_f16c_support_ = false;
#endif
}

// Initialization
template<typename T>
void HalfDistanceCalculator<T>::initialize() {
    if (initialized_) {
        return;
    }
    
    detect_cpu_features();
    
    // Set up function pointers based on CPU features
#ifdef F16C_SUPPORT
    if (has_f16c_support_) {
        l2_squared_distance_func_ = l2_squared_distance_f16c;
        inner_product_func_ = inner_product_f16c;
        cosine_similarity_func_ = cosine_similarity_f16c;
        l1_distance_func_ = l1_distance_f16c;
    } else {
#endif
        l2_squared_distance_func_ = l2_squared_distance_default;
        inner_product_func_ = inner_product_default;
        cosine_similarity_func_ = cosine_similarity_default;
        l1_distance_func_ = l1_distance_default;
#ifdef F16C_SUPPORT
    }
#endif
    
    initialized_ = true;
}

// Default implementations
template<typename T>
float HalfDistanceCalculator<T>::l2_squared_distance_default(int dim, HalfType* ax, HalfType* bx) {
    validate_dimensions(dim);
    
    if (ax == nullptr || bx == nullptr) {
        throw std::invalid_argument("Input arrays cannot be null");
    }
    
    float distance = 0.0f;
    
    for (int i = 0; i < dim; i++) {
        float a = half_to_float(ax[i]);
        float b = half_to_float(bx[i]);
        float diff = a - b;
        distance += diff * diff;
    }
    
    return distance;
}

template<typename T>
float HalfDistanceCalculator<T>::inner_product_default(int dim, HalfType* ax, HalfType* bx) {
    validate_dimensions(dim);
    
    if (ax == nullptr || bx == nullptr) {
        throw std::invalid_argument("Input arrays cannot be null");
    }
    
    float result = 0.0f;
    
    for (int i = 0; i < dim; i++) {
        float a = half_to_float(ax[i]);
        float b = half_to_float(bx[i]);
        result += a * b;
    }
    
    return result;
}

template<typename T>
double HalfDistanceCalculator<T>::cosine_similarity_default(int dim, HalfType* ax, HalfType* bx) {
    validate_dimensions(dim);
    
    if (ax == nullptr || bx == nullptr) {
        throw std::invalid_argument("Input arrays cannot be null");
    }
    
    float dot_product = 0.0f;
    float norm_a = 0.0f;
    float norm_b = 0.0f;
    
    for (int i = 0; i < dim; i++) {
        float a = half_to_float(ax[i]);
        float b = half_to_float(bx[i]);
        
        dot_product += a * b;
        norm_a += a * a;
        norm_b += b * b;
    }
    
    if (norm_a == 0.0f || norm_b == 0.0f) {
        throw std::invalid_argument("Cannot compute cosine similarity of zero vectors");
    }
    
    return static_cast<double>(dot_product) / (std::sqrt(norm_a) * std::sqrt(norm_b));
}

template<typename T>
float HalfDistanceCalculator<T>::l1_distance_default(int dim, HalfType* ax, HalfType* bx) {
    validate_dimensions(dim);
    
    if (ax == nullptr || bx == nullptr) {
        throw std::invalid_argument("Input arrays cannot be null");
    }
    
    float distance = 0.0f;
    
    for (int i = 0; i < dim; i++) {
        float a = half_to_float(ax[i]);
        float b = half_to_float(bx[i]);
        distance += std::abs(a - b);
    }
    
    return distance;
}

// Manual half to float conversion for uint16
template<typename T>
float HalfDistanceCalculator<T>::half_to_float_manual(uint16_t h) {
    // Simplified manual conversion - in a real implementation,
    // this would need to handle all the IEEE 754 edge cases
    uint16_t sign = (h >> 15) & 0x0001;
    uint16_t exponent = (h >> 10) & 0x001f;
    uint16_t mantissa = h & 0x03ff;
    
    if (exponent == 0) {
        if (mantissa == 0) {
            // Zero
            return sign ? -0.0f : 0.0f;
        } else {
            // Subnormal
            float result = mantissa / 1024.0f;
            return sign ? -result : result;
        }
    } else if (exponent == 31) {
        if (mantissa == 0) {
            // Infinity
            return sign ? -std::numeric_limits<float>::infinity() : std::numeric_limits<float>::infinity();
        } else {
            // NaN
            return std::numeric_limits<float>::quiet_NaN();
        }
    }
    
    // Normal number
    float result = (1.0f + mantissa / 1024.0f) * std::pow(2.0f, exponent - 15);
    return sign ? -result : result;
}

// Manual float to half conversion for uint16
template<typename T>
uint16_t HalfDistanceCalculator<T>::float_to_half_manual(float f) {
    // Simplified manual conversion - in a real implementation,
    // this would need to handle all the IEEE 754 edge cases
    uint32_t bits;
    std::memcpy(&bits, &f, sizeof(float));
    
    uint32_t sign = (bits >> 31) & 0x1;
    uint32_t exponent = (bits >> 23) & 0xff;
    uint32_t mantissa = bits & 0x7fffff;
    
    if (exponent == 255) {
        // Infinity or NaN
        return (sign << 15) | 0x7c00 | (mantissa ? 1 : 0);
    }
    
    // Convert exponent
    int32_t new_exponent = exponent - 127 + 15;
    
    if (new_exponent >= 31) {
        // Overflow to infinity
        return (sign << 15) | 0x7c00;
    } else if (new_exponent <= 0) {
        // Underflow to zero
        return (sign << 15);
    }
    
    // Convert mantissa (round to 10 bits)
    uint16_t new_mantissa = mantissa >> 13;
    
    // Round if necessary
    if (mantissa & 0x1000) {
        new_mantissa++;
        if (new_mantissa == 0x400) {
            // Mantissa overflow
            new_mantissa = 0;
            new_exponent++;
            if (new_exponent == 31) {
                return (sign << 15) | 0x7c00; // Infinity
            }
        }
    }
    
    return (sign << 15) | (new_exponent << 10) | new_mantissa;
}

#ifdef F16C_SUPPORT
// F16C optimized implementations
template<typename T>
float HalfDistanceCalculator<T>::l2_squared_distance_f16c(int dim, HalfType* ax, HalfType* bx) {
    validate_dimensions(dim);
    
    if (ax == nullptr || bx == nullptr) {
        throw std::invalid_argument("Input arrays cannot be null");
    }
    
    float distance = 0.0f;
    int i = 0;
    
    // Process 8 elements at a time using AVX
    for (; i + 8 <= dim; i += 8) {
        __m128i a = _mm_loadu_si128((__m128i*)(ax + i));
        __m128i b = _mm_loadu_si128((__m128i*)(bx + i));
        
        __m256 a_f32 = _mm256_cvtph_ps(a);
        __m256 b_f32 = _mm256_cvtph_ps(b);
        
        __m256 diff = _mm256_sub_ps(a_f32, b_f32);
        __m256 squared = _mm256_mul_ps(diff, diff);
        
        // Horizontal sum
        __m128 low = _mm256_castps256_ps128(squared);
        __m128 high = _mm256_extractf128_ps(squared, 1);
        __m128 sum = _mm_add_ps(low, high);
        sum = _mm_hadd_ps(sum, sum);
        sum = _mm_hadd_ps(sum, sum);
        
        distance += _mm_cvtss_f32(sum);
    }
    
    // Handle remaining elements
    for (; i < dim; i++) {
        float a = half_to_float(ax[i]);
        float b = half_to_float(bx[i]);
        float diff = a - b;
        distance += diff * diff;
    }
    
    return distance;
}

template<typename T>
float HalfDistanceCalculator<T>::inner_product_f16c(int dim, HalfType* ax, HalfType* bx) {
    validate_dimensions(dim);
    
    if (ax == nullptr || bx == nullptr) {
        throw std::invalid_argument("Input arrays cannot be null");
    }
    
    float result = 0.0f;
    int i = 0;
    
    // Process 8 elements at a time using AVX
    for (; i + 8 <= dim; i += 8) {
        __m128i a = _mm_loadu_si128((__m128i*)(ax + i));
        __m128i b = _mm_loadu_si128((__m128i*)(bx + i));
        
        __m256 a_f32 = _mm256_cvtph_ps(a);
        __m256 b_f32 = _mm256_cvtph_ps(b);
        
        __m256 product = _mm256_mul_ps(a_f32, b_f32);
        
        // Horizontal sum
        __m128 low = _mm256_castps256_ps128(product);
        __m128 high = _mm256_extractf128_ps(product, 1);
        __m128 sum = _mm_add_ps(low, high);
        sum = _mm_hadd_ps(sum, sum);
        sum = _mm_hadd_ps(sum, sum);
        
        result += _mm_cvtss_f32(sum);
    }
    
    // Handle remaining elements
    for (; i < dim; i++) {
        float a = half_to_float(ax[i]);
        float b = half_to_float(bx[i]);
        result += a * b;
    }
    
    return result;
}

template<typename T>
double HalfDistanceCalculator<T>::cosine_similarity_f16c(int dim, HalfType* ax, HalfType* bx) {
    validate_dimensions(dim);
    
    if (ax == nullptr || bx == nullptr) {
        throw std::invalid_argument("Input arrays cannot be null");
    }
    
    float dot_product = 0.0f;
    float norm_a = 0.0f;
    float norm_b = 0.0f;
    int i = 0;
    
    // Process 8 elements at a time using AVX
    for (; i + 8 <= dim; i += 8) {
        __m128i a = _mm_loadu_si128((__m128i*)(ax + i));
        __m128i b = _mm_loadu_si128((__m128i*)(bx + i));
        
        __m256 a_f32 = _mm256_cvtph_ps(a);
        __m256 b_f32 = _mm256_cvtph_ps(b);
        
        __m256 dot = _mm256_mul_ps(a_f32, b_f32);
        __m256 norm_a_sq = _mm256_mul_ps(a_f32, a_f32);
        __m256 norm_b_sq = _mm256_mul_ps(b_f32, b_f32);
        
        // Horizontal sums
        __m128 dot_low = _mm256_castps256_ps128(dot);
        __m128 dot_high = _mm256_extractf128_ps(dot, 1);
        __m128 dot_sum = _mm_add_ps(dot_low, dot_high);
        dot_sum = _mm_hadd_ps(dot_sum, dot_sum);
        dot_sum = _mm_hadd_ps(dot_sum, dot_sum);
        dot_product += _mm_cvtss_f32(dot_sum);
        
        __m128 norm_a_low = _mm256_castps256_ps128(norm_a_sq);
        __m128 norm_a_high = _mm256_extractf128_ps(norm_a_sq, 1);
        __m128 norm_a_sum = _mm_add_ps(norm_a_low, norm_a_high);
        norm_a_sum = _mm_hadd_ps(norm_a_sum, norm_a_sum);
        norm_a_sum = _mm_hadd_ps(norm_a_sum, norm_a_sum);
        norm_a += _mm_cvtss_f32(norm_a_sum);
        
        __m128 norm_b_low = _mm256_castps256_ps128(norm_b_sq);
        __m128 norm_b_high = _mm256_extractf128_ps(norm_b_sq, 1);
        __m128 norm_b_sum = _mm_add_ps(norm_b_low, norm_b_high);
        norm_b_sum = _mm_hadd_ps(norm_b_sum, norm_b_sum);
        norm_b_sum = _mm_hadd_ps(norm_b_sum, norm_b_sum);
        norm_b += _mm_cvtss_f32(norm_b_sum);
    }
    
    // Handle remaining elements
    for (; i < dim; i++) {
        float a = half_to_float(ax[i]);
        float b = half_to_float(bx[i]);
        
        dot_product += a * b;
        norm_a += a * a;
        norm_b += b * b;
    }
    
    if (norm_a == 0.0f || norm_b == 0.0f) {
        throw std::invalid_argument("Cannot compute cosine similarity of zero vectors");
    }
    
    return static_cast<double>(dot_product) / (std::sqrt(norm_a) * std::sqrt(norm_b));
}

template<typename T>
float HalfDistanceCalculator<T>::l1_distance_f16c(int dim, HalfType* ax, HalfType* bx) {
    validate_dimensions(dim);
    
    if (ax == nullptr || bx == nullptr) {
        throw std::invalid_argument("Input arrays cannot be null");
    }
    
    float distance = 0.0f;
    int i = 0;
    
    // Process 8 elements at a time using AVX
    for (; i + 8 <= dim; i += 8) {
        __m128i a = _mm_loadu_si128((__m128i*)(ax + i));
        __m128i b = _mm_loadu_si128((__m128i*)(bx + i));
        
        __m256 a_f32 = _mm256_cvtph_ps(a);
        __m256 b_f32 = _mm256_cvtph_ps(b);
        
        __m256 diff = _mm256_sub_ps(a_f32, b_f32);
        __m256 abs_diff = _mm256_and_ps(diff, _mm256_castsi256_ps(_mm256_set1_epi32(0x7fffffff)));
        
        // Horizontal sum
        __m128 low = _mm256_castps256_ps128(abs_diff);
        __m128 high = _mm256_extractf128_ps(abs_diff, 1);
        __m128 sum = _mm_add_ps(low, high);
        sum = _mm_hadd_ps(sum, sum);
        sum = _mm_hadd_ps(sum, sum);
        
        distance += _mm_cvtss_f32(sum);
    }
    
    // Handle remaining elements
    for (; i < dim; i++) {
        float a = half_to_float(ax[i]);
        float b = half_to_float(bx[i]);
        distance += std::abs(a - b);
    }
    
    return distance;
}
#endif // F16C_SUPPORT

// Public interface functions
template<typename T>
float HalfDistanceCalculator<T>::l2_squared_distance(int dim, HalfType* ax, HalfType* bx) {
    if (!initialized_) {
        throw HalfutilsError("Halfutils not initialized. Call initialize() first.");
    }
    return l2_squared_distance_func_(dim, ax, bx);
}

template<typename T>
float HalfDistanceCalculator<T>::inner_product(int dim, HalfType* ax, HalfType* bx) {
    if (!initialized_) {
        throw HalfutilsError("Halfutils not initialized. Call initialize() first.");
    }
    return inner_product_func_(dim, ax, bx);
}

template<typename T>
double HalfDistanceCalculator<T>::cosine_similarity(int dim, HalfType* ax, HalfType* bx) {
    if (!initialized_) {
        throw HalfutilsError("Halfutils not initialized. Call initialize() first.");
    }
    return cosine_similarity_func_(dim, ax, bx);
}

template<typename T>
float HalfDistanceCalculator<T>::l1_distance(int dim, HalfType* ax, HalfType* bx) {
    if (!initialized_) {
        throw HalfutilsError("Halfutils not initialized. Call initialize() first.");
    }
    return l1_distance_func_(dim, ax, bx);
}

// Explicit template instantiation for default half type
template class HalfDistanceCalculator<half>;

// Global initialization function for C compatibility
void HalfvecInit() {
    DefaultHalfCalculator::initialize();
}

} // namespace halfutils
