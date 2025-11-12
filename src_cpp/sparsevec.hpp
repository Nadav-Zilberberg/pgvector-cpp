#ifndef SPARSEVEC_HPP
#define SPARSEVEC_HPP

#include <cstdint>
#include <cstddef>
#include <stdexcept>

namespace pgvector {
namespace cpp {

// Constants
constexpr int32_t SPARSEVEC_MAX_DIM = 1000000000;
constexpr int32_t SPARSEVEC_MAX_NNZ = 16000;

// SparseVector class
class SparseVector {
private:
    int32_t vl_len_;     // varlena header (do not touch directly!)
    int32_t dim;         // number of dimensions
    int32_t nnz;         // number of non-zero elements
    int32_t unused;      // reserved for future use, always zero
    int32_t indices[];   // indices array (flexible array member)

public:
    // Constructor
    explicit SparseVector(int32_t dimensions, int32_t non_zero_count) 
        : vl_len_(0), dim(dimensions), nnz(non_zero_count), unused(0) {}

    // Deleted copy constructor and assignment operator
    // (PostgreSQL varlena types are typically not copied)
    SparseVector(const SparseVector&) = delete;
    SparseVector& operator=(const SparseVector&) = delete;

    // Move constructor and assignment operator
    SparseVector(SparseVector&&) noexcept = default;
    SparseVector& operator=(SparseVector&&) noexcept = default;

    // Destructor
    ~SparseVector() = default;

    // Accessors
    int32_t getDim() const { return dim; }
    int32_t getNnz() const { return nnz; }
    int32_t getUnused() const { return unused; }
    
    const int32_t* getIndices() const { return indices; }
    int32_t* getIndices() { return indices; }
    
    const float* getValues() const { 
        return reinterpret_cast<const float*>(reinterpret_cast<const char*>(this) + 
                                             sizeof(SparseVector) + 
                                             (nnz * sizeof(int32_t)));
    }
    float* getValues() { 
        return reinterpret_cast<float*>(reinterpret_cast<char*>(this) + 
                                       sizeof(SparseVector) + 
                                       (nnz * sizeof(int32_t)));
    }

    // Index and value access with bounds checking
    int32_t getIndex(int32_t pos) const {
        if (pos < 0 || pos >= nnz) {
            throw std::out_of_range("Index position out of range");
        }
        return indices[pos];
    }

    float getValue(int32_t pos) const {
        if (pos < 0 || pos >= nnz) {
            throw std::out_of_range("Value position out of range");
        }
        return getValues()[pos];
    }

    void setIndex(int32_t pos, int32_t value) {
        if (pos < 0 || pos >= nnz) {
            throw std::out_of_range("Index position out of range");
        }
        indices[pos] = value;
    }

    void setValue(int32_t pos, float value) {
        if (pos < 0 || pos >= nnz) {
            throw std::out_of_range("Value position out of range");
        }
        getValues()[pos] = value;
    }

    // Helper functions
    static size_t size(int32_t nnz) {
        return sizeof(SparseVector) + (nnz * sizeof(int32_t)) + (nnz * sizeof(float));
    }

    static float* values(SparseVector* x) {
        return x->getValues();
    }

    static const float* values(const SparseVector* x) {
        return x->getValues();
    }
};

// Factory function for creating SparseVector instances
SparseVector* InitSparseVector(int32_t dim, int32_t nnz);

} // namespace cpp
} // namespace pgvector

#endif // SPARSEVEC_HPP
