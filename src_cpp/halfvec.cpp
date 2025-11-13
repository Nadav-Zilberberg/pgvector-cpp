// PostgreSQL includes (for C interface)
extern "C" {
#include "postgres.h"
#include "fmgr.h"
#include "utils/builtins.h"
#include "utils/array.h"
#include "catalog/pg_type.h"
#include "lib/stringinfo.h"
#include "libpq/pqformat.h"
#include "common/shortest_dec.h"
#include "port.h"
}
// PostgreSQL macro definitions
#define DatumGetHalfVector(x) (reinterpret_cast<halfvec::HalfVector_p*>(PG_DETOAST_DATUM(x)))
#define PG_GETARG_HALFVEC_P(x) (DatumGetHalfVector(PG_GETARG_DATUM(x)))
#define PG_RETURN_HALFVEC_P(x) (PG_RETURN_POINTER(x))

#include "halfvec.hpp"
#include "halfutils.hpp"
#include "../src/bitvec.h"
#include "../src/sparsevec.h"
#include "../src/vector.h"
#include <cstring>
#include <stdexcept>
#include <memory>
#include <limits>
#include <cmath>

// PostgreSQL includes (for C interface)
extern "C" {
#include "postgres.h"
#include "fmgr.h"
#include "utils/builtins.h"
#include "utils/array.h"
#include "catalog/pg_type.h"
#include "lib/stringinfo.h"
#include "libpq/pqformat.h"
#include "common/shortest_dec.h"
#include "port.h"
}

namespace halfvec {
// Forward declaration
HalfVector_p* InitHalfVector(int dim);


// Exception types for halfvec operations
class HalfvecError : public std::runtime_error {
public:
    explicit HalfvecError(const std::string& msg) : std::runtime_error(msg) {}
};

class DimensionMismatch : public HalfvecError {
public:
    explicit DimensionMismatch(const std::string& msg) : HalfvecError("Dimension mismatch: " + msg) {}
};

class InvalidInput : public HalfvecError {
public:
    explicit InvalidInput(const std::string& msg) : HalfvecError("Invalid input: " + msg) {}
};

// RAII wrapper for PostgreSQL memory context
class MemoryContextGuard {
private:
    MemoryContext oldcontext_;
    
public:
    explicit MemoryContextGuard(MemoryContext newcontext) {
        oldcontext_ = MemoryContextSwitchTo(newcontext);
    }
    
    ~MemoryContextGuard() {
        MemoryContextSwitchTo(oldcontext_);
    }
    
    // Non-copyable
    MemoryContextGuard(const MemoryContextGuard&) = delete;
    MemoryContextGuard& operator=(const MemoryContextGuard&) = delete;
    
    // Non-movable (PostgreSQL memory contexts don't support moving)
    MemoryContextGuard(MemoryContextGuard&&) = delete;
    MemoryContextGuard& operator=(MemoryContextGuard&&) = delete;
};

// Helper function to validate dimensions
inline void validate_dimensions(int dim, int expected_dim = -1) {
    if (dim <= 0) {
        throw InvalidInput("dimensions must be positive");
    }
    if (dim > HALFVEC_MAX_DIM) {
        throw InvalidInput("dimensions exceed maximum allowed");
    }
    if (expected_dim > 0 && dim != expected_dim) {
        throw DimensionMismatch("expected " + std::to_string(expected_dim) + 
                              " dimensions, got " + std::to_string(dim));
    }
}

// Helper function to create state datums with RAII
inline std::unique_ptr<Datum[], decltype(&pfree)> create_state_datums(int dim) {
    Datum* datums = static_cast<Datum*>(palloc(sizeof(Datum) * (dim + 1)));
    return std::unique_ptr<Datum[], decltype(&pfree)>(datums, pfree);
}

// Convert C-style string to half vector with proper error handling
HalfVector_p* parse_halfvec_from_string(const char* str, int32 typmod) {
    if (!str) {
        throw InvalidInput("null input string");
    }
    
    // Skip leading whitespace
    while (*str && isspace((unsigned char) *str)) {
        str++;
    }
    
    if (*str != '[') {
        throw InvalidInput("half vector must begin with \"[\"");
    }
    
    // Count dimensions
    const char* ptr = str + 1;
    int dim = 1;
    while (*ptr && *ptr != ']') {
        if (*ptr == ',') {
            dim++;
        }
        ptr++;
    }
    
    if (*ptr != ']') {
        throw InvalidInput("half vector must end with \"]\"");
    }
    
    // Validate dimensions against typmod
    if (typmod >= 0 && dim != typmod) {
        throw InvalidInput("dimension mismatch");
    }
    
    validate_dimensions(dim);
    
    // Create the half vector
    HalfVector_p* result = InitHalfVector(dim);
    
    // Parse values
    ptr = str + 1;
    for (int i = 0; i < dim; i++) {
        char* endptr;
        float val = strtof(ptr, &endptr);
        
        if (endptr == ptr) {
            throw InvalidInput("invalid numeric value at position " + std::to_string(i));
        }
        
        // Check for overflow/underflow
        if (val > HALF_MAX || val < -HALF_MAX) {
            throw InvalidInput("value out of range for half precision at position " + std::to_string(i));
        }
        
        result->x[i] = halfutils::DefaultHalfCalculator::float_to_half(val);
        
        // Skip to next value
        ptr = endptr;
        while (*ptr && isspace((unsigned char) *ptr)) {
            ptr++;
        }
        if (i < dim - 1) {
            if (*ptr != ',') {
                throw InvalidInput("expected comma at position " + std::to_string(i));
            }
            ptr++;
            while (*ptr && isspace((unsigned char) *ptr)) {
                ptr++;
            }
        }
    }
    
    return result;
}

// Convert half vector to string representation
std::string halfvec_to_string(const HalfVector_p* vec) {
    if (!vec) {
        throw InvalidInput("null half vector");
    }
    
    std::string result = "[";
    
    for (int i = 0; i < vec->dim; i++) {
        if (i > 0) {
            result += ",";
        }
        
        float val = halfutils::DefaultHalfCalculator::half_to_float(vec->x[i]);
        
        // Use shortest decimal representation
        char buf[64];
        int len = float_to_shortest_decimal_bufn(val, buf);
        result.append(buf, len);
    }
    
    result += "]";
    return result;
}

} // namespace halfvec

// C interface functions that delegate to C++ implementation
extern "C" {

// Input function
FUNCTION_PREFIX PG_FUNCTION_INFO_V1(halfvec_in);
Datum
halfvec_in(PG_FUNCTION_ARGS) {
    try {
        char* lit = PG_GETARG_CSTRING(0);
        int32 typmod = PG_GETARG_INT32(2);
        
        halfvec::HalfVector_p* result = halfvec::parse_halfvec_from_string(lit, typmod);
        
        PG_RETURN_POINTER(result);
    } catch (const halfvec::InvalidInput& e) {
        ereport(ERROR,
                (errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
                 errmsg("invalid input syntax for half vector: %s", e.what())));
    } catch (const std::exception& e) {
        ereport(ERROR,
                (errcode(ERRCODE_INTERNAL_ERROR),
                 errmsg("unexpected error in halfvec_in: %s", e.what())));
    }
}

// Output function
FUNCTION_PREFIX PG_FUNCTION_INFO_V1(halfvec_out);
Datum
halfvec_out(PG_FUNCTION_ARGS) {
    try {
        halfvec::HalfVector_p* vector = PG_GETARG_HALFVEC_P(0);
        
        std::string result = halfvec::halfvec_to_string(vector);
        
        PG_RETURN_TEXT_P(cstring_to_text(result.c_str()));
    } catch (const halfvec::InvalidInput& e) {
        ereport(ERROR,
                (errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
                 errmsg("invalid half vector: %s", e.what())));
    } catch (const std::exception& e) {
        ereport(ERROR,
                (errcode(ERRCODE_INTERNAL_ERROR),
                 errmsg("unexpected error in halfvec_out: %s", e.what())));
    }
}

// Typmod input function
FUNCTION_PREFIX PG_FUNCTION_INFO_V1(halfvec_typmod_in);
Datum
halfvec_typmod_in(PG_FUNCTION_ARGS) {
    try {
        ArrayType* ta = PG_GETARG_ARRAYTYPE_P(0);
        
        int32* tl = reinterpret_cast<int32*>(ARR_DATA_PTR(ta));
        int n = ARR_DIMS(ta)[0];
        
        if (n != 1) {
            ereport(ERROR,
                    (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
                     errmsg("typmod array must have exactly one element")));
        }
        
        if (*tl <= 0) {
            ereport(ERROR,
                    (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
                     errmsg("dimensions for half vector must be positive")));
        }
        
        if (*tl > halfvec::HALFVEC_MAX_DIM) {
            ereport(ERROR,
                    (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
                     errmsg("dimensions for half vector cannot exceed %zu", halfvec::HALFVEC_MAX_DIM)));
        }
        
        PG_RETURN_INT32(*tl);
    } catch (const std::exception& e) {
        ereport(ERROR,
                (errcode(ERRCODE_INTERNAL_ERROR),
                 errmsg("unexpected error in halfvec_typmod_in: %s", e.what())));
    }
}

// Receive function
FUNCTION_PREFIX PG_FUNCTION_INFO_V1(halfvec_recv);
Datum
halfvec_recv(PG_FUNCTION_ARGS) {
    try {
        StringInfo buf = (StringInfo) PG_GETARG_POINTER(0);
        int32 typmod = PG_GETARG_INT32(2);
        
        int dim = pq_getmsgint(buf, sizeof(int16));
        
        if (typmod >= 0 && dim != typmod) {
            ereport(ERROR,
                    (errcode(ERRCODE_INVALID_BINARY_REPRESENTATION),
                     errmsg("binary representation contains incompatible dimension")));
        }
        
        halfvec::validate_dimensions(dim);
        
        halfvec::HalfVector_p* result = halfvec::InitHalfVector(dim);
        
        for (int i = 0; i < dim; i++) {
            uint16 half_val = pq_getmsgint(buf, sizeof(uint16));
            result->x[i] = half_val;
        }
        
        PG_RETURN_POINTER(result);
    } catch (const halfvec::InvalidInput& e) {
        ereport(ERROR,
                (errcode(ERRCODE_INVALID_BINARY_REPRESENTATION),
                 errmsg("invalid binary representation for half vector: %s", e.what())));
    } catch (const std::exception& e) {
        ereport(ERROR,
                (errcode(ERRCODE_INTERNAL_ERROR),
                 errmsg("unexpected error in halfvec_recv: %s", e.what())));
    }
}

// Send function
FUNCTION_PREFIX PG_FUNCTION_INFO_V1(halfvec_send);
Datum
halfvec_send(PG_FUNCTION_ARGS) {
    try {
        halfvec::HalfVector_p* vec = PG_GETARG_HALFVEC_P(0);
        
        StringInfoData buf;
        pq_begintypsend(&buf);
        
        pq_sendint16(&buf, vec->dim);
        
        for (int i = 0; i < vec->dim; i++) {
            pq_sendint16(&buf, vec->x[i]);
        }
        
        PG_RETURN_BYTEA_P(pq_endtypsend(&buf));
    } catch (const std::exception& e) {
        ereport(ERROR,
                (errcode(ERRCODE_INTERNAL_ERROR),
                 errmsg("unexpected error in halfvec_send: %s", e.what())));
    }
}

} // extern "C"

// Additional helper functions for operations
namespace halfvec {
// Forward declaration
HalfVector_p* InitHalfVector(int dim);


// Get dimensions of half vector
int get_halfvec_dims(const HalfVector_p* vec) {
    if (!vec) {
        throw InvalidInput("null half vector");
    }
    return vec->dim;
}

// Calculate L2 norm of half vector
double calculate_l2_norm(const HalfVector_p* vec) {
    if (!vec) {
        throw InvalidInput("null half vector");
    }
    
    float sum = 0.0f;
    for (int i = 0; i < vec->dim; i++) {
        float val = halfutils::DefaultHalfCalculator::half_to_float(vec->x[i]);
        sum += val * val;
    }
    
    return std::sqrt(static_cast<double>(sum));
}

// Normalize half vector to unit length
HalfVector_p* normalize_halfvec(const HalfVector_p* vec) {
    if (!vec) {
        throw InvalidInput("null half vector");
    }
    
    double norm = calculate_l2_norm(vec);
    if (norm == 0.0) {
        throw InvalidInput("cannot normalize zero vector");
    }
    
    HalfVector_p* result = InitHalfVector(vec->dim);
    
    for (int i = 0; i < vec->dim; i++) {
        float val = halfutils::DefaultHalfCalculator::half_to_float(vec->x[i]);
        float normalized_val = static_cast<float>(val / norm);
        result->x[i] = halfutils::DefaultHalfCalculator::float_to_half(normalized_val);
    }
    
    return result;
}

// Add two half vectors
HalfVector_p* add_halfvecs(const HalfVector_p* a, const HalfVector_p* b) {
    if (!a || !b) {
        throw InvalidInput("null half vector in addition");
    }
    if (a->dim != b->dim) {
        throw DimensionMismatch("cannot add half vectors of different dimensions");
    }
    
    HalfVector_p* result = InitHalfVector(a->dim);
    
    for (int i = 0; i < a->dim; i++) {
        float val_a = halfutils::DefaultHalfCalculator::half_to_float(a->x[i]);
        float val_b = halfutils::DefaultHalfCalculator::half_to_float(b->x[i]);
        float sum = val_a + val_b;
        
        // Check for overflow
        if (sum > HALF_MAX || sum < -HALF_MAX) {
            throw InvalidInput("addition result out of range for half precision");
        }
        
        result->x[i] = halfutils::DefaultHalfCalculator::float_to_half(sum);
    }
    
    return result;
}

// Subtract two half vectors
HalfVector_p* subtract_halfvecs(const HalfVector_p* a, const HalfVector_p* b) {
    if (!a || !b) {
        throw InvalidInput("null half vector in subtraction");
    }
    if (a->dim != b->dim) {
        throw DimensionMismatch("cannot subtract half vectors of different dimensions");
    }
    
    HalfVector_p* result = InitHalfVector(a->dim);
    
    for (int i = 0; i < a->dim; i++) {
        float val_a = halfutils::DefaultHalfCalculator::half_to_float(a->x[i]);
        float val_b = halfutils::DefaultHalfCalculator::half_to_float(b->x[i]);
        float diff = val_a - val_b;
        
        // Check for overflow
        if (diff > HALF_MAX || diff < -HALF_MAX) {
            throw InvalidInput("subtraction result out of range for half precision");
        }
        
        result->x[i] = halfutils::DefaultHalfCalculator::float_to_half(diff);
    }
    
    return result;
}

// Multiply half vector by scalar
HalfVector_p* multiply_halfvec_scalar(const HalfVector_p* vec, float scalar) {
    if (!vec) {
        throw InvalidInput("null half vector in multiplication");
    }
    
    HalfVector_p* result = InitHalfVector(vec->dim);
    
    for (int i = 0; i < vec->dim; i++) {
        float val = halfutils::DefaultHalfCalculator::half_to_float(vec->x[i]);
        float product = val * scalar;
        
        // Check for overflow
        if (product > HALF_MAX || product < -HALF_MAX) {
            throw InvalidInput("multiplication result out of range for half precision");
        }
        
        result->x[i] = halfutils::DefaultHalfCalculator::float_to_half(product);
    }
    
    return result;
}

// Concatenate two half vectors
HalfVector_p* concat_halfvecs(const HalfVector_p* a, const HalfVector_p* b) {
    if (!a || !b) {
        throw InvalidInput("null half vector in concatenation");
    }
    
    int new_dim = a->dim + b->dim;
    if (new_dim > HALFVEC_MAX_DIM) {
        throw InvalidInput("concatenated dimensions exceed maximum allowed");
    }
    
    HalfVector_p* result = InitHalfVector(new_dim);
    
    // Copy first vector
    std::memcpy(result->x, a->x, a->dim * sizeof(half));
    
    // Copy second vector
    std::memcpy(result->x + a->dim, b->x, b->dim * sizeof(half));
    
    return result;
}

// Binary quantize half vector
HalfVector_p* binary_quantize_halfvec(const HalfVector_p* vec) {
    if (!vec) {
        throw InvalidInput("null half vector for quantization");
    }
    
    HalfVector_p* result = InitHalfVector(vec->dim);
    
    for (int i = 0; i < vec->dim; i++) {
        float val = halfutils::DefaultHalfCalculator::half_to_float(vec->x[i]);
        // Binary quantization: positive -> 1.0, non-positive -> -1.0
        float quantized = (val > 0.0f) ? 1.0f : -1.0f;
        result->x[i] = halfutils::DefaultHalfCalculator::float_to_half(quantized);
    }
    
    return result;
}

// Extract subvector
HalfVector_p* extract_subvector(const HalfVector_p* vec, int start, int end) {
    if (!vec) {
        throw InvalidInput("null half vector for subvector extraction");
    }
    if (start < 1 || start > vec->dim) {
        throw InvalidInput("start position out of range");
    }
    if (end < start || end > vec->dim) {
        throw InvalidInput("end position out of range");
    }
    
    int new_dim = end - start + 1;
    HalfVector_p* result = InitHalfVector(new_dim);
    
    std::memcpy(result->x, vec->x + start - 1, new_dim * sizeof(half));
    
    return result;
}

// Compare two half vectors
int compare_halfvecs(const HalfVector_p* a, const HalfVector_p* b) {
    if (!a || !b) {
        throw InvalidInput("null half vector in comparison");
    }
    if (a->dim != b->dim) {
        throw DimensionMismatch("cannot compare half vectors of different dimensions");
    }
    
    for (int i = 0; i < a->dim; i++) {
        float val_a = halfutils::DefaultHalfCalculator::half_to_float(a->x[i]);
        float val_b = halfutils::DefaultHalfCalculator::half_to_float(b->x[i]);
        
        if (val_a < val_b) {
            return -1;
        } else if (val_a > val_b) {
            return 1;
        }
    }
    
    return 0;
}

} // namespace halfvec

// Additional PostgreSQL function wrappers
extern "C" {

// L1 distance function
FUNCTION_PREFIX PG_FUNCTION_INFO_V1(halfvec_l1_distance);
Datum
halfvec_l1_distance(PG_FUNCTION_ARGS) {
    try {
        halfvec::HalfVector_p* a = PG_GETARG_HALFVEC_P(0);
        halfvec::HalfVector_p* b = PG_GETARG_HALFVEC_P(1);
        
        if (a->dim != b->dim) {
            ereport(ERROR,
                    (errcode(ERRCODE_DATA_EXCEPTION),
                     errmsg("different half vector dimensions %d and %d", a->dim, b->dim)));
        }
        
        float distance = halfutils::DefaultHalfCalculator::l1_distance(a->dim, a->x, b->x);
        
        PG_RETURN_FLOAT4(distance);
    } catch (const std::exception& e) {
        ereport(ERROR,
                (errcode(ERRCODE_INTERNAL_ERROR),
                 errmsg("unexpected error in halfvec_l1_distance: %s", e.what())));
    }
}

// Get dimensions function
FUNCTION_PREFIX PG_FUNCTION_INFO_V1(halfvec_vector_dims);
Datum
halfvec_vector_dims(PG_FUNCTION_ARGS) {
    try {
        halfvec::HalfVector_p* vec = PG_GETARG_HALFVEC_P(0);
        
        int dims = halfvec::get_halfvec_dims(vec);
        
        PG_RETURN_INT32(dims);
    } catch (const halfvec::InvalidInput& e) {
        ereport(ERROR,
                (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
                 errmsg("invalid half vector: %s", e.what())));
    } catch (const std::exception& e) {
        ereport(ERROR,
                (errcode(ERRCODE_INTERNAL_ERROR),
                 errmsg("unexpected error in halfvec_vector_dims: %s", e.what())));
    }
}

// L2 norm function
FUNCTION_PREFIX PG_FUNCTION_INFO_V1(halfvec_l2_norm);
Datum
halfvec_l2_norm(PG_FUNCTION_ARGS) {
    try {
        halfvec::HalfVector_p* vec = PG_GETARG_HALFVEC_P(0);
        
        double norm = halfvec::calculate_l2_norm(vec);
        
        PG_RETURN_FLOAT8(norm);
    } catch (const halfvec::InvalidInput& e) {
        ereport(ERROR,
                (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
                 errmsg("invalid half vector: %s", e.what())));
    } catch (const std::exception& e) {
        ereport(ERROR,
                (errcode(ERRCODE_INTERNAL_ERROR),
                 errmsg("unexpected error in halfvec_l2_norm: %s", e.what())));
    }
}

// L2 normalize function
FUNCTION_PREFIX PG_FUNCTION_INFO_V1(halfvec_l2_normalize);
Datum
halfvec_l2_normalize(PG_FUNCTION_ARGS) {
    try {
        halfvec::HalfVector_p* vec = PG_GETARG_HALFVEC_P(0);
        
        halfvec::HalfVector_p* result = halfvec::normalize_halfvec(vec);
        
        PG_RETURN_POINTER(result);
    } catch (const halfvec::InvalidInput& e) {
        ereport(ERROR,
                (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
                 errmsg("invalid half vector: %s", e.what())));
    } catch (const std::exception& e) {
        ereport(ERROR,
                (errcode(ERRCODE_INTERNAL_ERROR),
                 errmsg("unexpected error in halfvec_l2_normalize: %s", e.what())));
    }
}

// Add function
FUNCTION_PREFIX PG_FUNCTION_INFO_V1(halfvec_add);
Datum
halfvec_add(PG_FUNCTION_ARGS) {
    try {
        halfvec::HalfVector_p* a = PG_GETARG_HALFVEC_P(0);
        halfvec::HalfVector_p* b = PG_GETARG_HALFVEC_P(1);
        
        halfvec::HalfVector_p* result = halfvec::add_halfvecs(a, b);
        
        PG_RETURN_POINTER(result);
    } catch (const halfvec::InvalidInput& e) {
        ereport(ERROR,
                (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
                 errmsg("invalid input: %s", e.what())));
    } catch (const halfvec::DimensionMismatch& e) {
        ereport(ERROR,
                (errcode(ERRCODE_DATA_EXCEPTION),
                 errmsg("%s", e.what())));
    } catch (const std::exception& e) {
        ereport(ERROR,
                (errcode(ERRCODE_INTERNAL_ERROR),
                 errmsg("unexpected error in halfvec_add: %s", e.what())));
    }
}

// Subtract function
FUNCTION_PREFIX PG_FUNCTION_INFO_V1(halfvec_sub);
Datum
halfvec_sub(PG_FUNCTION_ARGS) {
    try {
        halfvec::HalfVector_p* a = PG_GETARG_HALFVEC_P(0);
        halfvec::HalfVector_p* b = PG_GETARG_HALFVEC_P(1);
        
        halfvec::HalfVector_p* result = halfvec::subtract_halfvecs(a, b);
        
        PG_RETURN_POINTER(result);
    } catch (const halfvec::InvalidInput& e) {
        ereport(ERROR,
                (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
                 errmsg("invalid input: %s", e.what())));
    } catch (const halfvec::DimensionMismatch& e) {
        ereport(ERROR,
                (errcode(ERRCODE_DATA_EXCEPTION),
                 errmsg("%s", e.what())));
    } catch (const std::exception& e) {
        ereport(ERROR,
                (errcode(ERRCODE_INTERNAL_ERROR),
                 errmsg("unexpected error in halfvec_sub: %s", e.what())));
    }
}

// Multiply function
FUNCTION_PREFIX PG_FUNCTION_INFO_V1(halfvec_mul);
Datum
halfvec_mul(PG_FUNCTION_ARGS) {
    try {
        halfvec::HalfVector_p* vec = PG_GETARG_HALFVEC_P(0);
        float scalar = PG_GETARG_FLOAT4(1);
        
        halfvec::HalfVector_p* result = halfvec::multiply_halfvec_scalar(vec, scalar);
        
        PG_RETURN_POINTER(result);
    } catch (const halfvec::InvalidInput& e) {
        ereport(ERROR,
                (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
                 errmsg("invalid input: %s", e.what())));
    } catch (const std::exception& e) {
        ereport(ERROR,
                (errcode(ERRCODE_INTERNAL_ERROR),
                 errmsg("unexpected error in halfvec_mul: %s", e.what())));
    }
}

// Concat function
FUNCTION_PREFIX PG_FUNCTION_INFO_V1(halfvec_concat);
Datum
halfvec_concat(PG_FUNCTION_ARGS) {
    try {
        halfvec::HalfVector_p* a = PG_GETARG_HALFVEC_P(0);
        halfvec::HalfVector_p* b = PG_GETARG_HALFVEC_P(1);
        
        halfvec::HalfVector_p* result = halfvec::concat_halfvecs(a, b);
        
        PG_RETURN_POINTER(result);
    } catch (const halfvec::InvalidInput& e) {
        ereport(ERROR,
                (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
                 errmsg("invalid input: %s", e.what())));
    } catch (const std::exception& e) {
        ereport(ERROR,
                (errcode(ERRCODE_INTERNAL_ERROR),
                 errmsg("unexpected error in halfvec_concat: %s", e.what())));
    }
}

// Binary quantize function
FUNCTION_PREFIX PG_FUNCTION_INFO_V1(halfvec_binary_quantize);
Datum
halfvec_binary_quantize(PG_FUNCTION_ARGS) {
    try {
        halfvec::HalfVector_p* vec = PG_GETARG_HALFVEC_P(0);
        
        halfvec::HalfVector_p* result = halfvec::binary_quantize_halfvec(vec);
        
        PG_RETURN_POINTER(result);
    } catch (const halfvec::InvalidInput& e) {
        ereport(ERROR,
                (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
                 errmsg("invalid input: %s", e.what())));
    } catch (const std::exception& e) {
        ereport(ERROR,
                (errcode(ERRCODE_INTERNAL_ERROR),
                 errmsg("unexpected error in halfvec_binary_quantize: %s", e.what())));
    }
}

// Subvector function
FUNCTION_PREFIX PG_FUNCTION_INFO_V1(halfvec_subvector);
Datum
halfvec_subvector(PG_FUNCTION_ARGS) {
    try {
        halfvec::HalfVector_p* vec = PG_GETARG_HALFVEC_P(0);
        int32 start = PG_GETARG_INT32(1);
        int32 end = PG_GETARG_INT32(2);
        
        halfvec::HalfVector_p* result = halfvec::extract_subvector(vec, start, end);
        
        PG_RETURN_POINTER(result);
    } catch (const halfvec::InvalidInput& e) {
        ereport(ERROR,
                (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
                 errmsg("invalid input: %s", e.what())));
    } catch (const std::exception& e) {
        ereport(ERROR,
                (errcode(ERRCODE_INTERNAL_ERROR),
                 errmsg("unexpected error in halfvec_subvector: %s", e.what())));
    }
}

// Comparison functions
FUNCTION_PREFIX PG_FUNCTION_INFO_V1(halfvec_lt);
Datum
halfvec_lt(PG_FUNCTION_ARGS) {
    try {
        halfvec::HalfVector_p* a = PG_GETARG_HALFVEC_P(0);
        halfvec::HalfVector_p* b = PG_GETARG_HALFVEC_P(1);
        
        int cmp = halfvec::compare_halfvecs(a, b);
        
        PG_RETURN_BOOL(cmp < 0);
    } catch (const std::exception& e) {
        ereport(ERROR,
                (errcode(ERRCODE_INTERNAL_ERROR),
                 errmsg("unexpected error in halfvec_lt: %s", e.what())));
    }
}

FUNCTION_PREFIX PG_FUNCTION_INFO_V1(halfvec_le);
Datum
halfvec_le(PG_FUNCTION_ARGS) {
    try {
        halfvec::HalfVector_p* a = PG_GETARG_HALFVEC_P(0);
        halfvec::HalfVector_p* b = PG_GETARG_HALFVEC_P(1);
        
        int cmp = halfvec::compare_halfvecs(a, b);
        
        PG_RETURN_BOOL(cmp <= 0);
    } catch (const std::exception& e) {
        ereport(ERROR,
                (errcode(ERRCODE_INTERNAL_ERROR),
                 errmsg("unexpected error in halfvec_le: %s", e.what())));
    }
}

FUNCTION_PREFIX PG_FUNCTION_INFO_V1(halfvec_eq);
Datum
halfvec_eq(PG_FUNCTION_ARGS) {
    try {
        halfvec::HalfVector_p* a = PG_GETARG_HALFVEC_P(0);
        halfvec::HalfVector_p* b = PG_GETARG_HALFVEC_P(1);
        
        int cmp = halfvec::compare_halfvecs(a, b);
        
        PG_RETURN_BOOL(cmp == 0);
    } catch (const std::exception& e) {
        ereport(ERROR,
                (errcode(ERRCODE_INTERNAL_ERROR),
                 errmsg("unexpected error in halfvec_eq: %s", e.what())));
    }
}

FUNCTION_PREFIX PG_FUNCTION_INFO_V1(halfvec_ne);
Datum
halfvec_ne(PG_FUNCTION_ARGS) {
    try {
        halfvec::HalfVector_p* a = PG_GETARG_HALFVEC_P(0);
        halfvec::HalfVector_p* b = PG_GETARG_HALFVEC_P(1);
        
        int cmp = halfvec::compare_halfvecs(a, b);
        
        PG_RETURN_BOOL(cmp != 0);
    } catch (const std::exception& e) {
        ereport(ERROR,
                (errcode(ERRCODE_INTERNAL_ERROR),
                 errmsg("unexpected error in halfvec_ne: %s", e.what())));
    }
}

FUNCTION_PREFIX PG_FUNCTION_INFO_V1(halfvec_ge);
Datum
halfvec_ge(PG_FUNCTION_ARGS) {
    try {
        halfvec::HalfVector_p* a = PG_GETARG_HALFVEC_P(0);
        halfvec::HalfVector_p* b = PG_GETARG_HALFVEC_P(1);
        
        int cmp = halfvec::compare_halfvecs(a, b);
        
        PG_RETURN_BOOL(cmp >= 0);
    } catch (const std::exception& e) {
        ereport(ERROR,
                (errcode(ERRCODE_INTERNAL_ERROR),
                 errmsg("unexpected error in halfvec_ge: %s", e.what())));
    }
}

FUNCTION_PREFIX PG_FUNCTION_INFO_V1(halfvec_gt);
Datum
halfvec_gt(PG_FUNCTION_ARGS) {
    try {
        halfvec::HalfVector_p* a = PG_GETARG_HALFVEC_P(0);
        halfvec::HalfVector_p* b = PG_GETARG_HALFVEC_P(1);
        
        int cmp = halfvec::compare_halfvecs(a, b);
        
        PG_RETURN_BOOL(cmp > 0);
    } catch (const std::exception& e) {
        ereport(ERROR,
                (errcode(ERRCODE_INTERNAL_ERROR),
                 errmsg("unexpected error in halfvec_gt: %s", e.what())));
    }
}

// Comparison function for btree
FUNCTION_PREFIX PG_FUNCTION_INFO_V1(halfvec_cmp);
Datum
halfvec_cmp(PG_FUNCTION_ARGS) {
    try {
        halfvec::HalfVector_p* a = PG_GETARG_HALFVEC_P(0);
        halfvec::HalfVector_p* b = PG_GETARG_HALFVEC_P(1);
        
        int cmp = halfvec::compare_halfvecs(a, b);
        
        PG_RETURN_INT32(cmp);
    } catch (const std::exception& e) {
        ereport(ERROR,
                (errcode(ERRCODE_INTERNAL_ERROR),
                 errmsg("unexpected error in halfvec_cmp: %s", e.what())));
    }
}

// Accumulate function for aggregation
FUNCTION_PREFIX PG_FUNCTION_INFO_V1(halfvec_accum);
Datum
halfvec_accum(PG_FUNCTION_ARGS) {
    try {
        ArrayType* transarray = PG_GETARG_ARRAYTYPE_P(0);
        halfvec::HalfVector_p* vec = PG_GETARG_HALFVEC_P(1);
        
        // Check array dimensions
        if (ARR_NDIM(transarray) != 1 || ARR_DIMS(transarray)[0] != 2) {
            ereport(ERROR,
                    (errcode(ERRCODE_ARRAY_SUBSCRIPT_ERROR),
                     errmsg("expected 2-element array")));
        }
        
        if (ARR_NULLBITMAP(transarray)) {
            ereport(ERROR,
                    (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                     errmsg("array cannot contain nulls")));
        }
        
        Datum* transdatums = reinterpret_cast<Datum*>(ARR_DATA_PTR(transarray));
        int32 count = DatumGetInt32(transdatums[0]);
        ArrayType* sumarray = DatumGetArrayTypeP(transdatums[1]);
        
        // First call
        if (count == 0) {
            // Create new sum array with the same dimensions as the input vector
            int dims[1] = {vec->dim};
            int lbs[1] = {1};
            sumarray = construct_md_array(NULL, NULL, 1, dims, lbs,
                                         FLOAT4OID, sizeof(float), true, 'i');
        } else {
            // Check dimensions match
            if (ARR_DIMS(sumarray)[0] != vec->dim) {
                ereport(ERROR,
                        (errcode(ERRCODE_DATA_EXCEPTION),
                         errmsg("different half vector dimensions %d and %d", 
                                ARR_DIMS(sumarray)[0], vec->dim)));
            }
        }
        
        // Update sum
        float* sum = reinterpret_cast<float*>(ARR_DATA_PTR(sumarray));
        for (int i = 0; i < vec->dim; i++) {
            float val = halfutils::DefaultHalfCalculator::half_to_float(vec->x[i]);
            sum[i] += val;
        }
        
        // Update count
        count++;
        
        // Create result array
        Datum resultdatums[2];
        resultdatums[0] = Int32GetDatum(count);
        resultdatums[1] = PointerGetDatum(sumarray);
        
        ArrayType* result = construct_array(resultdatums, 2, INT4OID, sizeof(int32), true, 'i');
        
        PG_RETURN_POINTER(result);
    } catch (const std::exception& e) {
        ereport(ERROR,
                (errcode(ERRCODE_INTERNAL_ERROR),
                 errmsg("unexpected error in halfvec_accum: %s", e.what())));
    }
}

// Average function for aggregation
FUNCTION_PREFIX PG_FUNCTION_INFO_V1(halfvec_avg);
Datum
halfvec_avg(PG_FUNCTION_ARGS) {
    try {
        ArrayType* transarray = PG_GETARG_ARRAYTYPE_P(0);
        
        // Check array dimensions
        if (ARR_NDIM(transarray) != 1 || ARR_DIMS(transarray)[0] != 2) {
            ereport(ERROR,
                    (errcode(ERRCODE_ARRAY_SUBSCRIPT_ERROR),
                     errmsg("expected 2-element array")));
        }
        
        if (ARR_NULLBITMAP(transarray)) {
            ereport(ERROR,
                    (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                     errmsg("array cannot contain nulls")));
        }
        
        Datum* transdatums = reinterpret_cast<Datum*>(ARR_DATA_PTR(transarray));
        int32 count = DatumGetInt32(transdatums[0]);
        ArrayType* sumarray = DatumGetArrayTypeP(transdatums[1]);
        
        if (count == 0) {
            PG_RETURN_NULL();
        }
        
        int dim = ARR_DIMS(sumarray)[0];
        float* sum = reinterpret_cast<float*>(ARR_DATA_PTR(sumarray));
        
        // Create result vector
        halfvec::HalfVector_p* result = halfvec::InitHalfVector(dim);
        
        for (int i = 0; i < dim; i++) {
            float avg = sum[i] / count;
            result->x[i] = halfutils::DefaultHalfCalculator::float_to_half(avg);
        }
        
        PG_RETURN_POINTER(result);
    } catch (const std::exception& e) {
        ereport(ERROR,
                (errcode(ERRCODE_INTERNAL_ERROR),
                 errmsg("unexpected error in halfvec_avg: %s", e.what())));
    }
}

// Convert sparse vector to half vector
FUNCTION_PREFIX PG_FUNCTION_INFO_V1(sparsevec_to_halfvec);
Datum
sparsevec_to_halfvec(PG_FUNCTION_ARGS) {
    try {
        SparseVector* sparse = PG_GETARG_SPARSEVEC_P(0);
        int32 dimensions = PG_GETARG_INT32(1);
        
        if (dimensions <= 0) {
            ereport(ERROR,
                    (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
                     errmsg("dimensions must be positive")));
        }
        
        if (dimensions > halfvec::HALFVEC_MAX_DIM) {
            ereport(ERROR,
                    (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
                     errmsg("dimensions cannot exceed %zu", halfvec::HALFVEC_MAX_DIM)));
        }
        
        // Create result vector initialized to zeros
        halfvec::HalfVector_p* result = halfvec::InitHalfVector(dimensions);
        
        // Copy non-zero values from sparse vector
        for (int i = 0; i < sparse->nnz; i++) {
            int index = sparse->indices[i];
            if (index < 0 || index >= dimensions) {
                ereport(ERROR,
                        (errcode(ERRCODE_ARRAY_SUBSCRIPT_ERROR),
                         errmsg("sparse vector index %d out of range [0, %d)", 
                                index, dimensions)));
            }
            
            float val = SPARSEVEC_VALUES(sparse)[i];
            if (val > HALF_MAX || val < -HALF_MAX) {
                ereport(ERROR,
                        (errcode(ERRCODE_NUMERIC_VALUE_OUT_OF_RANGE),
                         errmsg("sparse vector value %f out of range for half precision", val)));
            }
            
            result->x[index] = halfutils::DefaultHalfCalculator::float_to_half(val);
        }
        
        PG_RETURN_POINTER(result);
    } catch (const std::exception& e) {
        ereport(ERROR,
                (errcode(ERRCODE_INTERNAL_ERROR),
                 errmsg("unexpected error in sparsevec_to_halfvec: %s", e.what())));
    }
}

} // extern "C"

// PostgreSQL macro definitions
#define DatumGetHalfVector(x) (reinterpret_cast<halfvec::HalfVector_p*>(PG_DETOAST_DATUM(x)))
#define PG_GETARG_HALFVEC_P(x) (DatumGetHalfVector(PG_GETARG_DATUM(x)))
#define PG_RETURN_HALFVEC_P(x) (PG_RETURN_POINTER(x))

// Additional distance functions
extern "C" {

// L2 distance function
FUNCTION_PREFIX PG_FUNCTION_INFO_V1(halfvec_l2_distance);
Datum
halfvec_l2_distance(PG_FUNCTION_ARGS) {
    try {
        halfvec::HalfVector_p* a = PG_GETARG_HALFVEC_P(0);
        halfvec::HalfVector_p* b = PG_GETARG_HALFVEC_P(1);
        
        if (a->dim != b->dim) {
            ereport(ERROR,
                    (errcode(ERRCODE_DATA_EXCEPTION),
                     errmsg("different half vector dimensions %d and %d", a->dim, b->dim)));
        }
        
        float distance = halfutils::DefaultHalfCalculator::l2_squared_distance(a->dim, a->x, b->x);
        
        PG_RETURN_FLOAT4(distance);
    } catch (const std::exception& e) {
        ereport(ERROR,
                (errcode(ERRCODE_INTERNAL_ERROR),
                 errmsg("unexpected error in halfvec_l2_distance: %s", e.what())));
    }
}

// L2 squared distance function
FUNCTION_PREFIX PG_FUNCTION_INFO_V1(halfvec_l2_squared_distance);
Datum
halfvec_l2_squared_distance(PG_FUNCTION_ARGS) {
    try {
        halfvec::HalfVector_p* a = PG_GETARG_HALFVEC_P(0);
        halfvec::HalfVector_p* b = PG_GETARG_HALFVEC_P(1);
        
        if (a->dim != b->dim) {
            ereport(ERROR,
                    (errcode(ERRCODE_DATA_EXCEPTION),
                     errmsg("different half vector dimensions %d and %d", a->dim, b->dim)));
        }
        
        float distance = halfutils::DefaultHalfCalculator::l2_squared_distance(a->dim, a->x, b->x);
        
        PG_RETURN_FLOAT4(distance);
    } catch (const std::exception& e) {
        ereport(ERROR,
                (errcode(ERRCODE_INTERNAL_ERROR),
                 errmsg("unexpected error in halfvec_l2_squared_distance: %s", e.what())));
    }
}

// Inner product function
FUNCTION_PREFIX PG_FUNCTION_INFO_V1(halfvec_inner_product);
Datum
halfvec_inner_product(PG_FUNCTION_ARGS) {
    try {
        halfvec::HalfVector_p* a = PG_GETARG_HALFVEC_P(0);
        halfvec::HalfVector_p* b = PG_GETARG_HALFVEC_P(1);
        
        if (a->dim != b->dim) {
            ereport(ERROR,
                    (errcode(ERRCODE_DATA_EXCEPTION),
                     errmsg("different half vector dimensions %d and %d", a->dim, b->dim)));
        }
        
        float result = halfutils::DefaultHalfCalculator::inner_product(a->dim, a->x, b->x);
        
        PG_RETURN_FLOAT4(result);
    } catch (const std::exception& e) {
        ereport(ERROR,
                (errcode(ERRCODE_INTERNAL_ERROR),
                 errmsg("unexpected error in halfvec_inner_product: %s", e.what())));
    }
}

// Negative inner product function
FUNCTION_PREFIX PG_FUNCTION_INFO_V1(halfvec_negative_inner_product);
Datum
halfvec_negative_inner_product(PG_FUNCTION_ARGS) {
    try {
        halfvec::HalfVector_p* a = PG_GETARG_HALFVEC_P(0);
        halfvec::HalfVector_p* b = PG_GETARG_HALFVEC_P(1);
        
        if (a->dim != b->dim) {
            ereport(ERROR,
                    (errcode(ERRCODE_DATA_EXCEPTION),
                     errmsg("different half vector dimensions %d and %d", a->dim, b->dim)));
        }
        
        float result = -halfutils::DefaultHalfCalculator::inner_product(a->dim, a->x, b->x);
        
        PG_RETURN_FLOAT4(result);
    } catch (const std::exception& e) {
        ereport(ERROR,
                (errcode(ERRCODE_INTERNAL_ERROR),
                 errmsg("unexpected error in halfvec_negative_inner_product: %s", e.what())));
    }
}

// Cosine distance function
FUNCTION_PREFIX PG_FUNCTION_INFO_V1(halfvec_cosine_distance);
Datum
halfvec_cosine_distance(PG_FUNCTION_ARGS) {
    try {
        halfvec::HalfVector_p* a = PG_GETARG_HALFVEC_P(0);
        halfvec::HalfVector_p* b = PG_GETARG_HALFVEC_P(1);
        
        if (a->dim != b->dim) {
            ereport(ERROR,
                    (errcode(ERRCODE_DATA_EXCEPTION),
                     errmsg("different half vector dimensions %d and %d", a->dim, b->dim)));
        }
        
        double similarity = halfutils::DefaultHalfCalculator::cosine_similarity(a->dim, a->x, b->x);
        float distance = 1.0f - static_cast<float>(similarity);
        
        PG_RETURN_FLOAT4(distance);
    } catch (const std::exception& e) {
        ereport(ERROR,
                (errcode(ERRCODE_INTERNAL_ERROR),
                 errmsg("unexpected error in halfvec_cosine_distance: %s", e.what())));
    }
}

// Spherical distance function
FUNCTION_PREFIX PG_FUNCTION_INFO_V1(halfvec_spherical_distance);
Datum
halfvec_spherical_distance(PG_FUNCTION_ARGS) {
    try {
        halfvec::HalfVector_p* a = PG_GETARG_HALFVEC_P(0);
        halfvec::HalfVector_p* b = PG_GETARG_HALFVEC_P(1);
        
        if (a->dim != b->dim) {
            ereport(ERROR,
                    (errcode(ERRCODE_DATA_EXCEPTION),
                     errmsg("different half vector dimensions %d and %d", a->dim, b->dim)));
        }
        
        double similarity = halfutils::DefaultHalfCalculator::cosine_similarity(a->dim, a->x, b->x);
        // Convert cosine similarity to spherical distance
        float distance = std::acos(std::max(-1.0, std::min(1.0, similarity)));
        
        PG_RETURN_FLOAT4(distance);
    } catch (const std::exception& e) {
        ereport(ERROR,
                (errcode(ERRCODE_INTERNAL_ERROR),
                 errmsg("unexpected error in halfvec_spherical_distance: %s", e.what())));
    }
}

// Convert half vector to float4 array
FUNCTION_PREFIX PG_FUNCTION_INFO_V1(halfvec_to_float4);
Datum
halfvec_to_float4(PG_FUNCTION_ARGS) {
    try {
        halfvec::HalfVector_p* vec = PG_GETARG_HALFVEC_P(0);
        
        // Create float4 array
        int dims[1] = {vec->dim};
        int lbs[1] = {1};
        ArrayType* result = construct_md_array(NULL, NULL, 1, dims, lbs,
                                             FLOAT4OID, sizeof(float), true, 'i');
        
        float* values = reinterpret_cast<float*>(ARR_DATA_PTR(result));
        
        for (int i = 0; i < vec->dim; i++) {
            values[i] = halfutils::DefaultHalfCalculator::half_to_float(vec->x[i]);
        }
        
        PG_RETURN_POINTER(result);
    } catch (const std::exception& e) {
        ereport(ERROR,
                (errcode(ERRCODE_INTERNAL_ERROR),
                 errmsg("unexpected error in halfvec_to_float4: %s", e.what())));
    }
}

// Convert vector to half vector
FUNCTION_PREFIX PG_FUNCTION_INFO_V1(vector_to_halfvec);
Datum
vector_to_halfvec(PG_FUNCTION_ARGS) {
    try {
        Vector* vec = PG_GETARG_VECTOR_P(0);
        
        if (vec->dim > halfvec::HALFVEC_MAX_DIM) {
            ereport(ERROR,
                    (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
                     errmsg("dimensions cannot exceed %zu", halfvec::HALFVEC_MAX_DIM)));
        }
        
        halfvec::HalfVector_p* result = halfvec::InitHalfVector(vec->dim);
        
        for (int i = 0; i < vec->dim; i++) {
            float val = vec->x[i];
            if (val > halfvec::HALF_MAX || val < -halfvec::HALF_MAX) {
                ereport(ERROR,
                        (errcode(ERRCODE_NUMERIC_VALUE_OUT_OF_RANGE),
                         errmsg("vector value %f out of range for half precision", val)));
            }
            result->x[i] = halfutils::DefaultHalfCalculator::float_to_half(val);
        }
        
        PG_RETURN_POINTER(result);
    } catch (const std::exception& e) {
        ereport(ERROR,
                (errcode(ERRCODE_INTERNAL_ERROR),
                 errmsg("unexpected error in vector_to_halfvec: %s", e.what())));
    }
}

// Convert array to half vector
FUNCTION_PREFIX PG_FUNCTION_INFO_V1(array_to_halfvec);
Datum
array_to_halfvec(PG_FUNCTION_ARGS) {
    try {
        ArrayType* array = PG_GETARG_ARRAYTYPE_P(0);
        
        // Check array type
        if (ARR_ELEMTYPE(array) != FLOAT4OID) {
            ereport(ERROR,
                    (errcode(ERRCODE_DATATYPE_MISMATCH),
                     errmsg("array must be of type float4")));
        }
        
        int dim = ARR_DIMS(array)[0];
        
        if (dim <= 0) {
            ereport(ERROR,
                    (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
                     errmsg("dimensions must be positive")));
        }
        
        if (dim > halfvec::HALFVEC_MAX_DIM) {
            ereport(ERROR,
                    (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
                     errmsg("dimensions cannot exceed %zu", halfvec::HALFVEC_MAX_DIM)));
        }
        
        halfvec::HalfVector_p* result = halfvec::InitHalfVector(dim);
        
        float* values = reinterpret_cast<float*>(ARR_DATA_PTR(array));
        
        for (int i = 0; i < dim; i++) {
            float val = values[i];
            if (val > halfvec::HALF_MAX || val < -halfvec::HALF_MAX) {
                ereport(ERROR,
                        (errcode(ERRCODE_NUMERIC_VALUE_OUT_OF_RANGE),
                         errmsg("array value %f out of range for half precision", val)));
            }
            result->x[i] = halfutils::DefaultHalfCalculator::float_to_half(val);
        }
        
        PG_RETURN_POINTER(result);
    } catch (const std::exception& e) {
        ereport(ERROR,
                (errcode(ERRCODE_INTERNAL_ERROR),
                 errmsg("unexpected error in array_to_halfvec: %s", e.what())));
    }
}

// Halfvec constructor function
FUNCTION_PREFIX PG_FUNCTION_INFO_V1(halfvec_constructor);
Datum
halfvec_constructor(PG_FUNCTION_ARGS) {
    try {
        ArrayType* array = PG_GETARG_ARRAYTYPE_P(0);
        
        // Check array type
        if (ARR_ELEMTYPE(array) != FLOAT4OID) {
            ereport(ERROR,
                    (errcode(ERRCODE_DATATYPE_MISMATCH),
                     errmsg("array must be of type float4")));
        }
        
        int dim = ARR_DIMS(array)[0];
        
        if (dim <= 0) {
            ereport(ERROR,
                    (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
                     errmsg("dimensions must be positive")));
        }
        
        if (dim > halfvec::HALFVEC_MAX_DIM) {
            ereport(ERROR,
                    (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
                     errmsg("dimensions cannot exceed %zu", halfvec::HALFVEC_MAX_DIM)));
        }
        
        halfvec::HalfVector_p* result = halfvec::InitHalfVector(dim);
        
        float* values = reinterpret_cast<float*>(ARR_DATA_PTR(array));
        
        for (int i = 0; i < dim; i++) {
            float val = values[i];
            if (val > halfvec::HALF_MAX || val < -halfvec::HALF_MAX) {
                ereport(ERROR,
                        (errcode(ERRCODE_NUMERIC_VALUE_OUT_OF_RANGE),
                         errmsg("array value %f out of range for half precision", val)));
            }
            result->x[i] = halfutils::DefaultHalfCalculator::float_to_half(val);
        }
        
        PG_RETURN_POINTER(result);
    } catch (const std::exception& e) {
        ereport(ERROR,
                (errcode(ERRCODE_INTERNAL_ERROR),
                 errmsg("unexpected error in halfvec: %s", e.what())));
    }
}

} // extern "C"
