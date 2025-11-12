# C to C++ Conversion Plan for pgvector-cpp

## Overview
This document outlines the step-by-step plan to convert all C code in the `src` directory to C++ code in the new `src_cpp` directory, while keeping the original C code intact.

## Conversion Principles

1. **Structs → Classes**: Convert all structs to classes with appropriate access modifiers
2. **C++ Features**: Utilize constexpr, new/delete, RAII, etc.
3. **Exception Handling**: Replace error reporting with C++ exceptions
4. **Namespaces**: Organize code into appropriate namespaces
5. **References**: Use references instead of pointers where appropriate
6. **Templates**: Create generic code using templates

## Dependency Analysis

### Files with NO project dependencies (convert first):
- `bitutils.h/c` - Only depends on postgres.h and halfvec.h (for defines only)
- `halfvec.h` - Only system headers
- `vector.h` - Only system headers  
- `sparsevec.h` - Only system headers

### Files with minimal dependencies:
- `bitvec.h/c` - Depends on bitutils.h, vector.h
- `halfutils.h/c` - Depends on halfvec.h
- `halfvec.c` - Depends on halfutils.h, halfvec.h, bitvec.h, vector.h, sparsevec.h
- `sparsevec.c` - Depends on sparsevec.h, halfutils.h, halfvec.h, vector.h
- `vector.c` - Depends on vector.h, bitutils.h, bitvec.h, halfutils.h, halfvec.h, sparsevec.h, hnsw.h, ivfflat.h

### Files with complex dependencies:
- `ivfflat.h/c` - Depends on vector.h
- `ivfutils.c` - Depends on ivfflat.h, bitvec.h, halfvec.h, vector.h
- `ivfbuild.c` - Depends on ivfflat.h, bitvec.h, halfvec.h, vector.h
- `ivfinsert.c` - Depends on ivfflat.h
- `ivfscan.c` - Depends on ivfflat.h
- `ivfkmeans.c` - Depends on ivfflat.h, bitvec.h, halfvec.h, vector.h
- `ivfvacuum.c` - Depends on ivfflat.h
- `hnsw.h/c` - Depends on vector.h
- `hnswutils.c` - Depends on hnsw.h, sparsevec.h
- `hnswbuild.c` - Depends on hnsw.h
- `hnswinsert.c` - Depends on hnsw.h
- `hnswscan.c` - Depends on hnsw.h
- `hnswvacuum.c` - Depends on hnsw.h

## Conversion Steps

### Phase 1: Foundation Files (No Dependencies)

#### Step 1.1: Convert `vector.h` → `src_cpp/vector.hpp`
**Dependencies**: None
**Conversion Tasks**:
- Convert `struct Vector` to `class Vector` with:
  - Private members: `vl_len_`, `dim`, `unused`
  - Public accessors: `getDim()`, `getData()`, etc.
  - Constructor: `Vector(int dim)`
  - Destructor (if needed)
- Use `constexpr` for constants like `VECTOR_MAX_DIM`
- Add namespace: `namespace pgvector { namespace cpp { ... } }`
- Convert macros to inline functions or constexpr
- Add move semantics (move constructor, move assignment)

**C++ Features to Apply**:
- Class instead of struct
- constexpr for constants
- Inline functions instead of macros
- Namespace organization

#### Step 1.2: Convert `halfvec.h` → `src_cpp/halfvec.hpp`
**Dependencies**: None (only system headers)
**Conversion Tasks**:
- Convert `struct HalfVector` to `class HalfVector`
- Use constexpr for `HALFVEC_MAX_DIM`
- Convert macros to inline functions
- Add namespace
- Template for half type if applicable

**C++ Features to Apply**:
- Class instead of struct
- constexpr
- Templates for type flexibility
- Namespace

#### Step 1.3: Convert `sparsevec.h` → `src_cpp/sparsevec.hpp`
**Dependencies**: None
**Conversion Tasks**:
- Convert `struct SparseVector` to `class SparseVector`
- Convert inline functions to class methods
- Use constexpr for constants
- Add namespace
- Consider template for value type

**C++ Features to Apply**:
- Class instead of struct
- constexpr
- Templates
- Namespace

#### Step 1.4: Convert `bitutils.h/c` → `src_cpp/bitutils.hpp/cpp`
**Dependencies**: Only postgres.h (and halfvec.h for defines)
**Conversion Tasks**:
- Convert function pointers to std::function or function objects
- Use constexpr where possible
- Add namespace
- Consider template specialization for different CPU features
- Use RAII for initialization
- Replace function pointer dispatch with virtual functions or std::function

**C++ Features to Apply**:
- std::function or function objects
- constexpr
- Templates for CPU feature dispatch
- RAII pattern
- Namespace
- Exception handling for errors

### Phase 2: Utility Files (Depend on Foundation)

#### Step 2.1: Convert `bitvec.h/c` → `src_cpp/bitvec.hpp/cpp`
**Dependencies**: bitutils.h, vector.h
**Conversion Tasks**:
- Convert to class-based approach
- Use references instead of pointers where appropriate
- Add exception handling
- Use namespace
- Template for different bit vector types if applicable

**C++ Features to Apply**:
- Classes
- References
- Exception handling
- Namespace

#### Step 2.2: Convert `halfutils.h/c` → `src_cpp/halfutils.hpp/cpp`
**Dependencies**: halfvec.h
**Conversion Tasks**:
- Convert inline functions to class methods or namespace functions
- Use constexpr for compile-time calculations
- Template for different half precision types
- Use std::function or function objects for dispatch
- Add exception handling
- RAII for initialization

**C++ Features to Apply**:
- constexpr
- Templates
- std::function
- Exception handling
- RAII
- Namespace

#### Step 2.3: Convert `halfvec.c` → `src_cpp/halfvec.cpp`
**Dependencies**: halfutils.h, halfvec.h, bitvec.h, vector.h, sparsevec.h
**Conversion Tasks**:
- Convert PostgreSQL function interface to C++ wrapper functions
- Use references instead of pointers
- Add exception handling (wrap ereport/elog)
- Use RAII for memory management where possible
- Convert static functions to namespace functions or class methods
- Use const correctness

**C++ Features to Apply**:
- References
- Exception handling
- RAII
- const correctness
- Namespace

#### Step 2.4: Convert `sparsevec.c` → `src_cpp/sparsevec.cpp`
**Dependencies**: sparsevec.h, halfutils.h, halfvec.h, vector.h
**Conversion Tasks**:
- Similar to halfvec.c conversion
- Use references
- Exception handling
- RAII
- const correctness

**C++ Features to Apply**:
- References
- Exception handling
- RAII
- const correctness
- Namespace

#### Step 2.5: Convert `vector.c` → `src_cpp/vector.cpp`
**Dependencies**: vector.h, bitutils.h, bitvec.h, halfutils.h, halfvec.h, sparsevec.h, hnsw.h, ivfflat.h
**Note**: This depends on hnsw.h and ivfflat.h, but those will be converted later. For now, create forward declarations or include the C headers temporarily.

**Conversion Tasks**:
- Convert PostgreSQL functions to C++ wrappers
- Use references
- Exception handling
- RAII
- const correctness

**C++ Features to Apply**:
- References
- Exception handling
- RAII
- const correctness
- Namespace

### Phase 3: Index Structure Files (IVFFlat)

#### Step 3.1: Convert `ivfflat.h` → `src_cpp/ivfflat.hpp`
**Dependencies**: vector.h
**Conversion Tasks**:
- Convert all structs to classes
- Use constexpr for constants
- Add namespace
- Use templates for generic index operations
- Convert macros to inline functions/constexpr

**C++ Features to Apply**:
- Classes
- constexpr
- Templates
- Namespace
- Inline functions

#### Step 3.2: Convert `ivfflat.c` → `src_cpp/ivfflat.cpp`
**Dependencies**: ivfflat.h
**Conversion Tasks**:
- Convert PostgreSQL access method interface
- Use classes for index operations
- Exception handling
- RAII
- References

**C++ Features to Apply**:
- Classes
- Exception handling
- RAII
- References
- Namespace

#### Step 3.3: Convert `ivfutils.c` → `src_cpp/ivfutils.cpp`
**Dependencies**: ivfflat.h, bitvec.h, halfvec.h, vector.h
**Conversion Tasks**:
- Convert utility functions to namespace functions or class methods
- Use references
- Exception handling
- RAII

**C++ Features to Apply**:
- References
- Exception handling
- RAII
- Namespace

#### Step 3.4: Convert `ivfbuild.c` → `src_cpp/ivfbuild.cpp`
**Dependencies**: ivfflat.h, bitvec.h, halfvec.h, vector.h
**Conversion Tasks**:
- Convert build logic to class-based approach
- Use RAII for resource management
- Exception handling
- References

**C++ Features to Apply**:
- Classes
- RAII
- Exception handling
- References
- Namespace

#### Step 3.5: Convert `ivfinsert.c` → `src_cpp/ivfinsert.cpp`
**Dependencies**: ivfflat.h
**Conversion Tasks**:
- Convert insert operations to class methods
- RAII
- Exception handling
- References

**C++ Features to Apply**:
- Classes
- RAII
- Exception handling
- References
- Namespace

#### Step 3.6: Convert `ivfscan.c` → `src_cpp/ivfscan.cpp`
**Dependencies**: ivfflat.h
**Conversion Tasks**:
- Convert scan operations to class-based approach
- RAII for scan state
- Exception handling
- References

**C++ Features to Apply**:
- Classes
- RAII
- Exception handling
- References
- Namespace

#### Step 3.7: Convert `ivfkmeans.c` → `src_cpp/ivfkmeans.cpp`
**Dependencies**: ivfflat.h, bitvec.h, halfvec.h, vector.h
**Conversion Tasks**:
- Convert k-means algorithm to class-based approach
- Use templates for different vector types
- RAII
- Exception handling
- References

**C++ Features to Apply**:
- Classes
- Templates
- RAII
- Exception handling
- References
- Namespace

#### Step 3.8: Convert `ivfvacuum.c` → `src_cpp/ivfvacuum.cpp`
**Dependencies**: ivfflat.h
**Conversion Tasks**:
- Convert vacuum operations to class methods
- RAII
- Exception handling
- References

**C++ Features to Apply**:
- Classes
- RAII
- Exception handling
- References
- Namespace

### Phase 4: Index Structure Files (HNSW)

#### Step 4.1: Convert `hnsw.h` → `src_cpp/hnsw.hpp`
**Dependencies**: vector.h
**Conversion Tasks**:
- Convert all structs to classes
- Use constexpr for constants
- Add namespace
- Use templates for graph operations
- Convert macros to inline functions/constexpr

**C++ Features to Apply**:
- Classes
- constexpr
- Templates
- Namespace
- Inline functions

#### Step 4.2: Convert `hnsw.c` → `src_cpp/hnsw.cpp`
**Dependencies**: hnsw.h
**Conversion Tasks**:
- Convert PostgreSQL access method interface
- Use classes for index operations
- Exception handling
- RAII
- References

**C++ Features to Apply**:
- Classes
- Exception handling
- RAII
- References
- Namespace

#### Step 4.3: Convert `hnswutils.c` → `src_cpp/hnswutils.cpp`
**Dependencies**: hnsw.h, sparsevec.h
**Conversion Tasks**:
- Convert utility functions to namespace functions or class methods
- Use references
- Exception handling
- RAII
- Templates for different vector types

**C++ Features to Apply**:
- References
- Exception handling
- RAII
- Templates
- Namespace

#### Step 4.4: Convert `hnswbuild.c` → `src_cpp/hnswbuild.cpp`
**Dependencies**: hnsw.h
**Conversion Tasks**:
- Convert build logic to class-based approach
- Use RAII for resource management
- Exception handling
- References
- Templates for graph construction

**C++ Features to Apply**:
- Classes
- RAII
- Exception handling
- References
- Templates
- Namespace

#### Step 4.5: Convert `hnswinsert.c` → `src_cpp/hnswinsert.cpp`
**Dependencies**: hnsw.h
**Conversion Tasks**:
- Convert insert operations to class methods
- RAII
- Exception handling
- References

**C++ Features to Apply**:
- Classes
- RAII
- Exception handling
- References
- Namespace

#### Step 4.6: Convert `hnswscan.c` → `src_cpp/hnswscan.cpp`
**Dependencies**: hnsw.h
**Conversion Tasks**:
- Convert scan operations to class-based approach
- RAII for scan state
- Exception handling
- References
- Templates for search algorithms

**C++ Features to Apply**:
- Classes
- RAII
- Exception handling
- References
- Templates
- Namespace

#### Step 4.7: Convert `hnswvacuum.c` → `src_cpp/hnswvacuum.cpp`
**Dependencies**: hnsw.h
**Conversion Tasks**:
- Convert vacuum operations to class methods
- RAII
- Exception handling
- References

**C++ Features to Apply**:
- Classes
- RAII
- Exception handling
- References
- Namespace

## Detailed Conversion Guidelines

### 1. Struct to Class Conversion

**C Code:**
```c
typedef struct Vector {
    int32 vl_len_;
    int16 dim;
    int16 unused;
    float x[FLEXIBLE_ARRAY_MEMBER];
} Vector;
```

**C++ Code:**
```cpp
namespace pgvector {
namespace cpp {

class Vector {
private:
    int32 vl_len_;
    int16 unused;
    
public:
    int16 dim;
    float x[];  // or use std::vector<float> for better C++ style
    
    // Constructor
    explicit Vector(int dim);
    
    // Destructor
    ~Vector() = default;
    
    // Accessors
    int16 getDim() const { return dim; }
    const float* getData() const { return x; }
    float* getData() { return x; }
    
    // Move semantics
    Vector(Vector&&) noexcept = default;
    Vector& operator=(Vector&&) noexcept = default;
    
    // Delete copy (PostgreSQL varlena types are typically not copied)
    Vector(const Vector&) = delete;
    Vector& operator=(const Vector&) = delete;
};

} // namespace cpp
} // namespace pgvector
```

### 2. Exception Handling

**C Code:**
```c
if (dim < 1)
    ereport(ERROR,
            (errcode(ERRCODE_DATA_EXCEPTION),
             errmsg("halfvec must have at least 1 dimension")));
```

**C++ Code:**
```cpp
namespace pgvector {
namespace cpp {
namespace exceptions {

class VectorException : public std::exception {
    // Custom exception class
};

class InvalidDimensionException : public VectorException {
    // Specific exception
};

} // namespace exceptions

inline void checkDim(int dim) {
    if (dim < 1) {
        throw exceptions::InvalidDimensionException("halfvec must have at least 1 dimension");
    }
}

} // namespace cpp
} // namespace pgvector
```

### 3. Using References Instead of Pointers

**C Code:**
```c
void processVector(Vector *vec) {
    // ...
}
```

**C++ Code:**
```cpp
void processVector(Vector& vec) {
    // ...
}

// Or for optional/nullable:
void processVector(Vector* vec) {
    if (!vec) return;
    // ...
}

// Or use std::optional:
void processVector(std::optional<std::reference_wrapper<Vector>> vec) {
    if (!vec) return;
    // ...
}
```

### 4. Using Templates

**C Code:**
```c
float calculateDistance(int dim, float *a, float *b);
```

**C++ Code:**
```cpp
template<typename T>
T calculateDistance(int dim, const T* a, const T* b) {
    // Generic implementation
}

// Specialization for different types
template<>
half calculateDistance<half>(int dim, const half* a, const half* b) {
    // Half precision specific implementation
}
```

### 5. Using constexpr

**C Code:**
```c
#define VECTOR_MAX_DIM 16000
```

**C++ Code:**
```cpp
constexpr int VECTOR_MAX_DIM = 16000;
```

### 6. RAII Pattern

**C Code:**
```c
Vector *vec = InitVector(dim);
// ... use vec ...
pfree(vec);
```

**C++ Code:**
```cpp
// Option 1: Smart pointer wrapper
std::unique_ptr<Vector, decltype(&pfree)> vec(InitVector(dim), pfree);

// Option 2: RAII wrapper class
class VectorRAII {
    Vector* vec;
public:
    explicit VectorRAII(int dim) : vec(InitVector(dim)) {}
    ~VectorRAII() { if (vec) pfree(vec); }
    Vector* get() { return vec; }
    Vector* operator->() { return vec; }
};
```

## File Organization

```
src_cpp/
├── vector.hpp
├── vector.cpp
├── halfvec.hpp
├── halfvec.cpp
├── sparsevec.hpp
├── sparsevec.cpp
├── bitvec.hpp
├── bitvec.cpp
├── bitutils.hpp
├── bitutils.cpp
├── halfutils.hpp
├── halfutils.cpp
├── ivfflat.hpp
├── ivfflat.cpp
├── ivfutils.cpp
├── ivfbuild.cpp
├── ivfinsert.cpp
├── ivfscan.cpp
├── ivfkmeans.cpp
├── ivfvacuum.cpp
├── hnsw.hpp
├── hnsw.cpp
├── hnswutils.cpp
├── hnswbuild.cpp
├── hnswinsert.cpp
├── hnswscan.cpp
└── hnswvacuum.cpp
```

## Testing Strategy

1. **Unit Tests**: Create C++ unit tests for each converted module
2. **Integration Tests**: Ensure C++ code works with PostgreSQL interface
3. **Performance Tests**: Compare C and C++ implementations
4. **Compatibility Tests**: Ensure C++ code produces same results as C code

## Notes

- PostgreSQL function interface (PG_FUNCTION_ARGS, etc.) must be preserved for compatibility
- Memory management (palloc/pfree) should be kept as-is or wrapped in RAII
- Some PostgreSQL macros and types must remain for compatibility
- Consider creating a compatibility layer for PostgreSQL-specific code

## Progress Tracking

- [ ] Phase 1: Foundation Files
  - [ ] Step 1.1: vector.h
  - [ ] Step 1.2: halfvec.h
  - [ ] Step 1.3: sparsevec.h
  - [ ] Step 1.4: bitutils.h/c
- [ ] Phase 2: Utility Files
  - [ ] Step 2.1: bitvec.h/c
  - [ ] Step 2.2: halfutils.h/c
  - [ ] Step 2.3: halfvec.c
  - [ ] Step 2.4: sparsevec.c
  - [ ] Step 2.5: vector.c
- [ ] Phase 3: IVFFlat Files
  - [ ] Step 3.1: ivfflat.h
  - [ ] Step 3.2: ivfflat.c
  - [ ] Step 3.3-3.8: All ivf*.c files
- [ ] Phase 4: HNSW Files
  - [ ] Step 4.1: hnsw.h
  - [ ] Step 4.2: hnsw.c
  - [ ] Step 4.3-4.7: All hnsw*.c files

