#ifndef BITVEC_HPP
#define BITVEC_HPP

#include <memory>
#include <stdexcept>
#include "utils/varbit.h"

namespace pgvector {
namespace cpp {

// Exception types
class BitVecException : public std::runtime_error {
public:
    explicit BitVecException(const std::string& message) : std::runtime_error(message) {}
};

class DimensionMismatchException : public BitVecException {
public:
    explicit DimensionMismatchException(const std::string& message) : BitVecException(message) {}
};

// BitVec class for managing bit vector operations
class BitVec {
private:
    std::unique_ptr<VarBit, void(*)(VarBit*)> data;
    
    // Custom deleter for VarBit
    static void delete_varbit(VarBit* ptr);
    
public:
    // Constructors
    BitVec() = delete;  // Prevent default construction
    explicit BitVec(int dim);
    
    // Copy constructor and assignment operator
    BitVec(const BitVec& other);
    BitVec& operator=(const BitVec& other);
    
    // Move constructor and assignment operator
    BitVec(BitVec&& other) noexcept;
    BitVec& operator=(BitVec&& other) noexcept;
    
    // Destructor
    ~BitVec() = default;
    
    // Getters
    VarBit* get() const { return data.get(); }
    VarBit& ref() const { return *data; }
    int get_dimension() const;
    
    // Static factory methods
    static BitVec create(int dim);
    
    // Distance functions
    static double hamming_distance(const BitVec& a, const BitVec& b);
    static double jaccard_distance(const BitVec& a, const BitVec& b);
    
    // Direct VarBit distance functions (for compatibility)
    static double hamming_distance(const VarBit& a, const VarBit& b);
    static double jaccard_distance(const VarBit& a, const VarBit& b);
    
    // Validate that two bit vectors have the same dimensions
    static void validate_dimensions(const VarBit& a, const VarBit& b);
};

// C-compatible wrapper functions
extern "C" {
    // Initialize a bit vector (C-compatible wrapper)
    VarBit* InitBitVector(int dim);
    
    // Validate dimensions (C-compatible wrapper)
    void CheckDims(VarBit* a, VarBit* b);
}

} // namespace cpp
} // namespace pgvector

#endif // BITVEC_HPP
