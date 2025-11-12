#include "src_cpp/sparsevec.hpp"
#include <iostream>
#include <stdexcept>

int main() {
    try {
        // Test constants
        std::cout << "SPARSEVEC_MAX_DIM: " << pgvector::cpp::SPARSEVEC_MAX_DIM << std::endl;
        std::cout << "SPARSEVEC_MAX_NNZ: " << pgvector::cpp::SPARSEVEC_MAX_NNZ << std::endl;

        // Test size calculation
        constexpr int32_t test_nnz = 5;
        size_t size = pgvector::cpp::SparseVector::size(test_nnz);
        std::cout << "Size for nnz=" << test_nnz << ": " << size << " bytes" << std::endl;

        // Test class creation (we can't actually instantiate it without PostgreSQL memory context)
        // but we can verify the class definition compiles correctly
        std::cout << "SparseVector class definition compiled successfully" << std::endl;

        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
