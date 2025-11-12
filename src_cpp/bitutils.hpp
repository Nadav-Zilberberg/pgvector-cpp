#ifndef BITUTILS_HPP
#define BITUTILS_HPP

#include <cstdint>
#include <cstddef>
#include <functional>
#include <memory>

namespace pgvector {
namespace cpp {

// Forward declarations
class BitUtils;

// Function signatures
using BitHammingDistanceFunc = std::function<uint64_t(uint32_t, unsigned char*, unsigned char*, uint64_t)>;
using BitJaccardDistanceFunc = std::function<double(uint32_t, unsigned char*, unsigned char*, uint64_t, uint64_t, uint64_t)>;

// BitUtils class for managing bit operations
class BitUtils {
private:
    // Static instance for singleton pattern
    static std::unique_ptr<BitUtils> instance;
    
    // Function pointers for dispatch
    BitHammingDistanceFunc hamming_distance_func;
    BitJaccardDistanceFunc jaccard_distance_func;
    
    // CPU feature detection
    static constexpr bool supports_avx512_popcount();
    
    // Constructor - initializes function pointers
    BitUtils();
    
public:
    // Delete copy constructor and assignment operator
    BitUtils(const BitUtils&) = delete;
    BitUtils& operator=(const BitUtils&) = delete;
    
    // Get singleton instance
    static BitUtils& get_instance();
    
    // Distance functions
    uint64_t hamming_distance(uint32_t bytes, unsigned char* ax, unsigned char* bx, uint64_t distance);
    double jaccard_distance(uint32_t bytes, unsigned char* ax, unsigned char* bx, uint64_t ab, uint64_t aa, uint64_t bb);
    
    // Initialize function pointers (called automatically by constructor)
    void initialize();
};

// Standalone functions that delegate to the singleton
uint64_t bit_hamming_distance(uint32_t bytes, unsigned char* ax, unsigned char* bx, uint64_t distance);
double bit_jaccard_distance(uint32_t bytes, unsigned char* ax, unsigned char* bx, uint64_t ab, uint64_t aa, uint64_t bb);

// RAII initializer class
class BitUtilsInitializer {
public:
    BitUtilsInitializer();
    ~BitUtilsInitializer() = default;
};

// Global initializer instance
extern BitUtilsInitializer bit_utils_initializer;

} // namespace cpp
} // namespace pgvector

#endif // BITUTILS_HPP
