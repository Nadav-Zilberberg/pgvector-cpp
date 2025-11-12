#!/usr/bin/env python3

import subprocess
import os
import tempfile

def test_edge_cases():
    print("Testing edge cases...")
    
    # Create a test C++ file for edge cases
    test_cpp = """
#include "src_cpp/halfutils.hpp"
#include <iostream>
#include <vector>
#include <cmath>

int main() {
    try {
        std::cout << "Testing edge cases..." << std::endl;
        
        // Initialize
        halfutils::DefaultHalfCalculator::initialize();
        
        // Test with zero vectors
        std::vector<half> zero_vec(3);
        std::vector<half> normal_vec(3);
        
        // Convert float values to half
        float zero_values[] = {0.0f, 0.0f, 0.0f};
        float normal_values[] = {1.0f, 2.0f, 3.0f};
        
        for (int i = 0; i < 3; i++) {
            zero_vec[i] = halfutils::DefaultHalfCalculator::float_to_half(zero_values[i]);
            normal_vec[i] = halfutils::DefaultHalfCalculator::float_to_half(normal_values[i]);
        }
        
        // Test L2 distance with zero vector
        float l2_zero = halfutils::DefaultHalfCalculator::l2_squared_distance(
            3, zero_vec.data(), normal_vec.data());
        std::cout << "L2 with zero vector: " << l2_zero << std::endl;
        
        // Test inner product with zero vector
        float inner_zero = halfutils::DefaultHalfCalculator::inner_product(
            3, zero_vec.data(), normal_vec.data());
        std::cout << "Inner product with zero vector: " << inner_zero << std::endl;
        
        // Test L1 distance with zero vector
        float l1_zero = halfutils::DefaultHalfCalculator::l1_distance(
            3, zero_vec.data(), normal_vec.data());
        std::cout << "L1 with zero vector: " << l1_zero << std::endl;
        
        // Test exception for cosine similarity with zero vector
        try {
            double cosine_zero = halfutils::DefaultHalfCalculator::cosine_similarity(
                3, zero_vec.data(), normal_vec.data());
            std::cout << "Cosine with zero vector: " << cosine_zero << std::endl;
        } catch (const std::exception& e) {
            std::cout << "✓ Cosine with zero vector exception: " << e.what() << std::endl;
        }
        
        // Test exception for both zero vectors
        try {
            double cosine_both_zero = halfutils::DefaultHalfCalculator::cosine_similarity(
                3, zero_vec.data(), zero_vec.data());
            std::cout << "Cosine with both zero vectors: " << cosine_both_zero << std::endl;
        } catch (const std::exception& e) {
            std::cout << "✓ Cosine with both zero vectors exception: " << e.what() << std::endl;
        }
        
        // Test invalid dimensions
        try {
            halfutils::DefaultHalfCalculator::l2_squared_distance(-1, nullptr, nullptr);
        } catch (const std::exception& e) {
            std::cout << "✓ Invalid dimensions exception: " << e.what() << std::endl;
        }
        
        // Test null pointers
        try {
            halfutils::DefaultHalfCalculator::l2_squared_distance(3, nullptr, nullptr);
        } catch (const std::exception& e) {
            std::cout << "✓ Null pointer exception: " << e.what() << std::endl;
        }
        
        // Test RAII initializer
        std::cout << "Testing RAII initializer..." << std::endl;
        {
            halfutils::DefaultHalfCalculator::Initializer init;
            std::cout << "✓ RAII initializer works" << std::endl;
        }
        
        // Test constexpr functions
        constexpr auto max_dim = halfutils::DefaultHalfCalculator::max_dimensions();
        std::cout << "Max dimensions: " << max_dim << std::endl;
        
        constexpr auto supports_f16c = halfutils::DefaultHalfCalculator::supports_f16c();
        std::cout << "F16C support: " << (supports_f16c ? "yes" : "no") << std::endl;
        
        std::cout << "Edge case tests completed!" << std::endl;
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Unexpected error: " << e.what() << std::endl;
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
            print("✓ Edge cases compilation successful")
            # Run the test
            run_result = subprocess.run([cpp_file + '.out'], capture_output=True, text=True)
            print("Output:")
            print(run_result.stdout)
            if run_result.stderr:
                print("Errors:")
                print(run_result.stderr)
            return run_result.returncode == 0
        else:
            print("✗ Edge cases compilation failed")
            print("Error:", result.stderr)
            return False
            
    finally:
        # Cleanup
        if os.path.exists(cpp_file):
            os.unlink(cpp_file)
        if os.path.exists(cpp_file + '.out'):
            os.unlink(cpp_file + '.out')

if __name__ == "__main__":
    success = test_edge_cases()
    if success:
        print("\\n✓ Edge case tests passed!")
    else:
        print("\\n✗ Edge case tests failed!")
