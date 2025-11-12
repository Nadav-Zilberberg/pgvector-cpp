#include "bitvec.hpp"
#include "bitutils.hpp"
#include "postgres.h"
#include "utils/varbit.h"
#include "bitutils.h"  // Include for BitHammingDistance and BitJaccardDistance function pointers

#if PG_VERSION_NUM >= 160000
#include "varatt.h"
#endif

namespace pgvector {
namespace cpp {

// Custom deleter for VarBit
void BitVec::delete_varbit(VarBit* ptr) {
    if (ptr != nullptr) {
        pfree(ptr);
    }
}

// Constructor
BitVec::BitVec(int dim) : data(nullptr, delete_varbit) {
    if (dim <= 0) {
        throw BitVecException("Bit vector dimension must be positive");
    }
    
    int size = VARBITTOTALLEN(dim);
    VarBit* result = (VarBit *) palloc0(size);
    SET_VARSIZE(result, size);
    VARBITLEN(result) = dim;
    
    data.reset(result);
}

// Copy constructor
BitVec::BitVec(const BitVec& other) : data(nullptr, delete_varbit) {
    if (other.data) {
        int size = VARSIZE(other.data.get());
        VarBit* copy = (VarBit *) palloc(size);
        memcpy(copy, other.data.get(), size);
        data.reset(copy);
    }
}

// Copy assignment operator
BitVec& BitVec::operator=(const BitVec& other) {
    if (this != &other) {
        if (other.data) {
            int size = VARSIZE(other.data.get());
            VarBit* copy = (VarBit *) palloc(size);
            memcpy(copy, other.data.get(), size);
            data.reset(copy);
        } else {
            data.reset();
        }
    }
    return *this;
}

// Move constructor
BitVec::BitVec(BitVec&& other) noexcept : data(std::move(other.data)) {
    // Reset other's data to nullptr to prevent double deletion
    other.data.reset();
}

// Move assignment operator
BitVec& BitVec::operator=(BitVec&& other) noexcept {
    if (this != &other) {
        data = std::move(other.data);
        other.data.reset();
    }
    return *this;
}

// Get dimension
int BitVec::get_dimension() const {
    if (!data) {
        throw BitVecException("Bit vector is not initialized");
    }
    return VARBITLEN(data.get());
}

// Static factory method
BitVec BitVec::create(int dim) {
    return BitVec(dim);
}

// Validate dimensions
void BitVec::validate_dimensions(const VarBit& a, const VarBit& b) {
    if (VARBITLEN(&a) != VARBITLEN(&b)) {
        throw DimensionMismatchException(
            "Bit vectors have different dimensions: " + 
            std::to_string(VARBITLEN(&a)) + " vs " + std::to_string(VARBITLEN(&b))
        );
    }
}

// Hamming distance between BitVec objects
double BitVec::hamming_distance(const BitVec& a, const BitVec& b) {
    if (!a.data || !b.data) {
        throw BitVecException("One or both bit vectors are not initialized");
    }
    return hamming_distance(*a.data, *b.data);
}

// Jaccard distance between BitVec objects
double BitVec::jaccard_distance(const BitVec& a, const BitVec& b) {
    if (!a.data || !b.data) {
        throw BitVecException("One or both bit vectors are not initialized");
    }
    return jaccard_distance(*a.data, *b.data);
}

// Hamming distance between VarBit objects
double BitVec::hamming_distance(const VarBit& a, const VarBit& b) {
    validate_dimensions(a, b);
    
    uint32_t bytes = VARBITBYTES(&a);
    unsigned char* bits_a = VARBITS(&a);
    unsigned char* bits_b = VARBITS(&b);
    
    // Use the C function pointer for Hamming distance
    return static_cast<double>(BitHammingDistance(bytes, bits_a, bits_b, 0));
}

// Jaccard distance between VarBit objects
double BitVec::jaccard_distance(const VarBit& a, const VarBit& b) {
    validate_dimensions(a, b);
    
    uint32_t bytes = VARBITBYTES(&a);
    unsigned char* bits_a = VARBITS(&a);
    unsigned char* bits_b = VARBITS(&b);
    
    // Use the C function pointer for Jaccard distance
    return BitJaccardDistance(bytes, bits_a, bits_b, 0, 0, 0);
}

// C-compatible wrapper functions
extern "C" {

VarBit* InitBitVector(int dim) {
    try {
        auto bitvec = BitVec::create(dim);
        // Release ownership to return raw pointer
        VarBit* result = bitvec.get();
        bitvec.data.release();  // Prevent double deletion
        return result;
    } catch (const std::exception& e) {
        ereport(ERROR,
                (errcode(ERRCODE_INTERNAL_ERROR),
                 errmsg("Failed to initialize bit vector: %s", e.what())));
        return nullptr;  // Never reached, but required for compilation
    }
}

void CheckDims(VarBit* a, VarBit* b) {
    try {
        BitVec::validate_dimensions(*a, *b);
    } catch (const DimensionMismatchException& e) {
        ereport(ERROR,
                (errcode(ERRCODE_DATA_EXCEPTION),
                 errmsg("%s", e.what())));
    } catch (const std::exception& e) {
        ereport(ERROR,
                (errcode(ERRCODE_INTERNAL_ERROR),
                 errmsg("Failed to validate dimensions: %s", e.what())));
    }
}

} // extern "C"

} // namespace cpp
} // namespace pgvector
