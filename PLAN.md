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
