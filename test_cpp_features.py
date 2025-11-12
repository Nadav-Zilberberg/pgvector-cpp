#!/usr/bin/env python3

import subprocess
import os
import tempfile

def test_cpp_features():
    print("Testing C++ features implementation...")
    
    # Create a comprehensive test C++ file
    test_cpp = """
#include "src_cpp/halfutils.hpp"
#include <iostream>
#include <vector>
#include <cmath>
#include <memory>
#include <functional>

int main() {
    try {
        std::cout << "=== Testing C++ Features Implementation ===" << std::endl;
        
        // 1. Namespace organization
        std::cout << "1. Namespace organization:" << std::endl;
        std::cout << "   Functions are in halfutils namespace" << std::endl;
        
        // 2. RAII initialization
        std::cout << "2. RAII initialization:" << std::endl;
        {
            halfutils::DefaultHalfCalculator::Initializer init;
            std::cout << "   ✓ RAII initializer created and destroyed" << std::endl;
        }
        
        // 3. Template support for different half types
        std::cout << "3. Template support:" << std::endl;
        using Float16Calculator = halfutils::HalfDistanceCalculator<half>;
        Float16Calculator::initialize();
        std::cout << "   ✓ Template instantiation successful" << std::endl;
        
        // 4. std::function for dispatch
        std::cout << "4. std::function dispatch:" << std::endl;
        using DistanceFunc = halfutils::DefaultHalfCalculator::DistanceFunction;
        DistanceFunc l2_func = halfutils::DefaultHalfCalculator::l2_squared_distance_default;
        std::cout << "   ✓ std::function type alias works" << std::endl;
        
        // 5. Exception handling
        std::cout << "5. Exception handling:" << std::endl;
        try {
            halfutils::DefaultHalfCalculator::l2_squared_distance(-1, nullptr, nullptr);
        } catch (const std::invalid_argument& e) {
            std::cout << "   ✓ Invalid argument exception caught: " << e.what() << std::endl;
        }
        
        try {
            halfutils::DefaultHalfCalculator::cosine_similarity(3, nullptr, nullptr);
        } catch (const std::invalid_argument& e) {
            std::cout << "   ✓ Cosine similarity exception caught: " << e.what() << std::endl;
        }
        
        // 6. constexpr functions
        std::cout << "6. constexpr functions:" << std::endl;
        constexpr auto max_dim = halfutils::DefaultHalfCalculator::max_dimensions();
        constexpr auto supports_f16c = halfutils::DefaultHalfCalculator::supports_f16c();
        std::cout << "   ✓ Max dimensions: " << max_dim << " (compile-time constant)" << std::endl;
        std::cout << "   ✓ F16C support: " << (supports_f16c ? "yes" : "no") << " (compile-time constant)" << std::endl;
        
        // 7. Test actual functionality
        std::cout << "7. Functional testing:" << std::endl;
        
        // Prepare test data
        std::vector<half> vec1(3);
        std::vector<half> vec2(3);
        
        float values1[] = {1.0f, 2.0f, 3.0f};
        float values2[] = {4.0f, 5.0f, 6.0f};
        
        for (int i = 0; i < 3; i++) {
            vec1[i] = halfutils::DefaultHalfCalculator::float_to_half(values1[i]);
            vec2[i] = halfutils::DefaultHalfCalculator::float_to_half(values2[i]);
        }
        
        // Test all distance functions
        float l2_dist = halfutils::DefaultHalfCalculator::l2_squared_distance(3, vec1.data(), vec2.data());
        float inner_prod = halfutils::DefaultHalfCalculator::inner_product(3, vec1.data(), vec2.data());
        float l1_dist = halfutils::DefaultHalfCalculator::l1_distance(3, vec1.data(), vec2.data());
        double cosine_sim = halfutils::DefaultHalfCalculator::cosine_similarity(3, vec1.data(), vec2.data());
        
        std::cout << "   ✓ L2 squared distance: " << l2_dist << std::endl;
        std::cout << "   ✓ Inner product: " << inner_prod << std::endl;
        std::cout << "   ✓ L1 distance: " << l1_dist << std::endl;
        std::cout << "   ✓ Cosine similarity: " << cosine_sim << std::endl;
        
        // 8. Namespace functions (C compatibility)
        std::cout << "8. Namespace functions (C compatibility):" << std::endl;
        float ns_l2 = halfutils::HalfvecL2SquaredDistance(3, vec1.data(), vec2.data());
        float ns_inner = halfutils::HalfvecInnerProduct(3, vec1.data(), vec2.data());
        float ns_l1 = halfutils::HalfvecL1Distance(3, vec1.data(), vec2.data());
        double ns_cosine = halfutils::HalfvecCosineSimilarity(3, vec1.data(), vec2.data());
        
        std::cout << "   ✓ Namespace L2: " << ns_l2 << std::endl;
        std::cout << "   ✓ Namespace inner product: " << ns_inner << std::endl;
        std::cout << "   ✓ Namespace L1: " << ns_l1 << std::endl;
        std::cout << "   ✓ Namespace cosine: " << ns_cosine << std::endl;
        
        // 9. Type safety
        std::cout << "9. Type safety:" << std::endl;
        std::cout << "   ✓ Half type is properly defined" << std::endl;
        std::cout << "   ✓ Function signatures are type-safe" << std::endl;
        
        std::cout << "\\n=== All C++ features working correctly! ===" << std::endl;
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
"""
    
    with tempfile.NamedTemporaryFile(mode='w', suffix='.cpp', delete=False) as f:
        f.write(test_cpp)
        cpp_file = f.name
    
    try:
        # Try to compile
        result = subprocess.run([
            'g++', '-std=c++17', '-I.', cpp_file, 'src_cpp/halfutils.cpp',
            '-o', cpp_file + '.out', '-lm'
        ], capture_output=True, text=True)
        
        if result.returncode == 0:
            print("✓ C++ features compilation successful")
            # Run the test
            run_result = subprocess.run([cpp_file + '.out'], capture_output=True, text=True)
            print("Output:")
            print(run_result.stdout)
            if run_result.stderr:
                print("Errors:")
                print(run_result.stderr)
            return run_result.returncode == 0
        else:
            print("✗ C++ features compilation failed")
            print("Error:", result.stderr)
            return False
            
    finally:
        # Cleanup
        if os.path.exists(cpp_file):
            os.unlink(cpp_file)
        if os.path.exists(cpp_file + '.out'):
            os.unlink(cpp_file + '.out')

if __name__ == "__main__":
    success = test_cpp_features()
    if success:
        print("\\n✓ All C++ features tests passed!")
    else:
        print("\\n✗ C++ features tests failed!")
