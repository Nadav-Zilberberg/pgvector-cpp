#ifndef VECTOR_HPP
#define VECTOR_HPP

#include <cstddef>
#include <cstring>
#include <algorithm>
#include <stdexcept>

namespace pgvector {
namespace cpp {

// Constants
constexpr int VECTOR_MAX_DIM = 16000;

// Forward declaration for size calculation
class Vector;

// Size calculation function (forward declaration)
constexpr std::size_t vector_size(int dim);

class Vector {
private:
    int32_t vl_len_;    // varlena header (do not touch directly!)
    int16_t dim;        // number of dimensions
    int16_t unused;     // reserved for future use, always zero
    float* x;           // flexible array member - using pointer for C++

public:
    // Constructor
    explicit Vector(int dimensions) : vl_len_(0), dim(dimensions), unused(0) {
        if (dim <= 0) {
            throw std::invalid_argument("Vector dimensions must be positive");
        }
        if (dim > VECTOR_MAX_DIM) {
            throw std::invalid_argument("Vector dimensions cannot exceed " + std::to_string(VECTOR_MAX_DIM));
        }
        // Note: In actual implementation, memory allocation would be handled
        // by the PostgreSQL memory context system (palloc/pfree)
        x = nullptr; // Will be set by memory allocation function
    }

    // Destructor
    ~Vector() = default;

    // Copy constructor
    Vector(const Vector& other) : vl_len_(other.vl_len_), dim(other.dim), unused(other.unused) {
        // Note: Actual implementation would need to copy the flexible array data
        x = other.x; // Simplified for now
    }

    // Move constructor
    Vector(Vector&& other) noexcept : vl_len_(other.vl_len_), dim(other.dim), unused(other.unused), x(other.x) {
        other.vl_len_ = 0;
        other.dim = 0;
        other.x = nullptr;
    }

    // Copy assignment operator
    Vector& operator=(const Vector& other) {
        if (this != &other) {
            vl_len_ = other.vl_len_;
            dim = other.dim;
            unused = other.unused;
            x = other.x; // Simplified for now
        }
        return *this;
    }

    // Move assignment operator
    Vector& operator=(Vector&& other) noexcept {
        if (this != &other) {
            vl_len_ = other.vl_len_;
            dim = other.dim;
            unused = other.unused;
            x = other.x;
            
            other.vl_len_ = 0;
            other.dim = 0;
            other.x = nullptr;
        }
        return *this;
    }

    // Accessor methods
    int getDim() const { return dim; }
    float* getData() { return x; }
    const float* getData() const { return x; }
    int32_t getVlLen() const { return vl_len_; }
    int16_t getUnused() const { return unused; }

    // Mutator methods
    void setVlLen(int32_t len) { vl_len_ = len; }
    void setData(float* data) { x = data; }
    void setUnused(int16_t val) { unused = val; }

    // Utility methods
    float& operator[](int index) {
        if (index < 0 || index >= dim) {
            throw std::out_of_range("Vector index out of range");
        }
        return x[index];
    }

    const float& operator[](int index) const {
        if (index < 0 || index >= dim) {
            throw std::out_of_range("Vector index out of range");
        }
        return x[index];
    }

    // Comparison operators for internal use
    bool operator==(const Vector& other) const {
        if (dim != other.dim) return false;
        for (int i = 0; i < dim; ++i) {
            if (x[i] != other.x[i]) return false;
        }
        return true;
    }

    bool operator!=(const Vector& other) const {
        return !(*this == other);
    }

    // Size calculation
    std::size_t size() const {
        return vector_size(dim);
    }
};

// Size calculation function (definition after Vector class)
constexpr std::size_t vector_size(int dim) {
    return sizeof(Vector) + sizeof(float) * dim;
}

// Inline functions to replace macros
inline Vector* datum_get_vector(void* datum) {
    return static_cast<Vector*>(datum);
}

inline Vector* pg_getarg_vector_p(int n) {
    // This would be implemented based on PostgreSQL's PG_GETARG_DATUM macro
    // For now, returning a placeholder
    (void)n; // Suppress unused parameter warning
    return nullptr;
}

inline void pg_return_vector_p(Vector* vector) {
    // This would be implemented based on PostgreSQL's PG_RETURN_POINTER macro
    // For now, just a placeholder
    (void)vector; // Suppress unused parameter warning
}

// Function declarations (to be implemented in corresponding .cpp file)
Vector* init_vector(int dim);
void print_vector(const char* msg, Vector* vector);
int vector_cmp_internal(Vector* a, Vector* b);

} // namespace cpp
} // namespace pgvector

#endif // VECTOR_HPP
