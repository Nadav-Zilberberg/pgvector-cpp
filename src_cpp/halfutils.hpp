#ifndef HALFUTILS_HPP
#define HALFUTILS_HPP

#include <cstddef>
#include <functional>
#include <stdexcept>
#include <memory>
#include <cmath>
#include <cstdint>
#include <algorithm>
#include <numeric>

// Define half type based on available support
#ifdef FLT16_SUPPORT
#include <float.h>
using half = _Float16;
constexpr auto HALF_MAX = FLT16_MAX;
#else
using half = std::uint16_t;
constexpr auto HALF_MAX = 65504;
#endif

namespace halfutils {

// Exception types
class HalfutilsError : public std::runtime_error {
public:
    explicit HalfutilsError(const std::string& msg) : std::runtime_error(msg) {}
};

class UnsupportedOperation : public HalfutilsError {
public:
    explicit UnsupportedOperation(const std::string& msg) : HalfutilsError("Unsupported: " + msg) {}
};

// Constants
constexpr std::size_t HALFVEC_MAX_DIM = 16000;

// Template for different half precision types
template<typename T = half>
class HalfDistanceCalculator {
public:
    using HalfType = T;
    using DistanceFunction = std::function<float(int dim, HalfType* ax, HalfType* bx)>;
    using DoubleDistanceFunction = std::function<double(int dim, HalfType* ax, HalfType* bx)>;

private:
    // Function pointers for dispatch
    static DistanceFunction l2_squared_distance_func_;
    static DistanceFunction inner_product_func_;
    static DoubleDistanceFunction cosine_similarity_func_;
    static DistanceFunction l1_distance_func_;
    
    // RAII initialization flag
    static bool initialized_;
    
    // CPU feature detection
    static bool has_f16c_support_;
    
    // Private constructor for RAII
    HalfDistanceCalculator() = delete;
    
public:
    // RAII initialization class
    class Initializer {
    public:
        Initializer();
        ~Initializer();
        
        // Non-copyable
        Initializer(const Initializer&) = delete;
        Initializer& operator=(const Initializer&) = delete;
        
        // Movable
        Initializer(Initializer&&) = default;
        Initializer& operator=(Initializer&&) = default;
    };
    
    // Static initialization method
    static void initialize();
    static bool is_initialized() { return initialized_; }
    
    // Distance calculation functions
    static float l2_squared_distance(int dim, HalfType* ax, HalfType* bx);
    static float inner_product(int dim, HalfType* ax, HalfType* bx);
    static double cosine_similarity(int dim, HalfType* ax, HalfType* bx);
    static float l1_distance(int dim, HalfType* ax, HalfType* bx);
    
    // Default implementations (portable)
    static float l2_squared_distance_default(int dim, HalfType* ax, HalfType* bx);
    static float inner_product_default(int dim, HalfType* ax, HalfType* bx);
    static double cosine_similarity_default(int dim, HalfType* ax, HalfType* bx);
    static float l1_distance_default(int dim, HalfType* ax, HalfType* bx);
    
#ifdef F16C_SUPPORT
    // F16C optimized implementations
    static float l2_squared_distance_f16c(int dim, HalfType* ax, HalfType* bx);
    static float inner_product_f16c(int dim, HalfType* ax, HalfType* bx);
    static double cosine_similarity_f16c(int dim, HalfType* ax, HalfType* bx);
    static float l1_distance_f16c(int dim, HalfType* ax, HalfType* bx);
#endif
    
    // Utility functions
    static constexpr bool supports_f16c() {
#ifdef F16C_SUPPORT
        return has_f16c_support_;
#else
        return false;
#endif
    }
    
    static constexpr std::size_t max_dimensions() {
        return HALFVEC_MAX_DIM;
    }
    
    // Validate dimensions
    static void validate_dimensions(int dim) {
        if (dim <= 0) {
            throw std::invalid_argument("Dimensions must be positive");
        }
        if (dim > static_cast<int>(max_dimensions())) {
            throw std::invalid_argument("Dimensions exceed maximum allowed");
        }
    }
    
    // Helper function to convert half to float
    static constexpr float half_to_float(HalfType h) {
#ifdef FLT16_SUPPORT
        return static_cast<float>(h);
#else
        // Manual conversion for uint16 representation
        return half_to_float_manual(h);
#endif
    }
    
    // Helper function to convert float to half
    static constexpr HalfType float_to_half(float f) {
#ifdef FLT16_SUPPORT
        return static_cast<HalfType>(f);
#else
        // Manual conversion for uint16 representation
        return float_to_half_manual(f);
#endif
    }
    
private:
    // Manual half to float conversion for uint16
    static float half_to_float_manual(uint16_t h);
    
    // Manual float to half conversion for uint16
    static uint16_t float_to_half_manual(float f);
    
    // CPU feature detection
    static void detect_cpu_features();
};

// Type alias for default calculator
using DefaultHalfCalculator = HalfDistanceCalculator<>;

// Global initialization function (for C compatibility)
void HalfvecInit();

// Namespace-level functions that delegate to the template class
inline float HalfvecL2SquaredDistance(int dim, half* ax, half* bx) {
    return DefaultHalfCalculator::l2_squared_distance(dim, ax, bx);
}

inline float HalfvecInnerProduct(int dim, half* ax, half* bx) {
    return DefaultHalfCalculator::inner_product(dim, ax, bx);
}

inline double HalfvecCosineSimilarity(int dim, half* ax, half* bx) {
    return DefaultHalfCalculator::cosine_similarity(dim, ax, bx);
}

inline float HalfvecL1Distance(int dim, half* ax, half* bx) {
    return DefaultHalfCalculator::l1_distance(dim, ax, bx);
}

} // namespace halfutils

#endif // HALFUTILS_HPP
