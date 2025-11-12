#!/usr/bin/env python3

import subprocess
import os
import tempfile

def test_cpp_compilation():
    print("Testing C++ halfutils compilation...")
    
    # Create a test C++ file
    test_cpp = """
#include "src_cpp/halfutils.hpp"
#include <iostream>
#include <vector>
#include <cmath>

int main() {
    try {
        std::cout << "Testing halfutils C++ implementation..." << std::endl;
        
        // Initialize
        halfutils::DefaultHalfCalculator::initialize();
        std::cout << "✓ Initialization successful" << std::endl;
        
        // Test with some sample data - convert float to half manually
        std::vector<half> vec1(3);
        std::vector<half> vec2(3);
        
        // Convert float values to half
        float values1[] = {1.0f, 2.0f, 3.0f};
        float values2[] = {4.0f, 5.0f, 6.0f};
        
        for (int i = 0; i < 3; i++) {
            vec1[i] = halfutils::DefaultHalfCalculator::float_to_half(values1[i]);
            vec2[i] = halfutils::DefaultHalfCalculator::float_to_half(values2[i]);
        }
        
        // Test L2 squared distance
        float l2_dist = halfutils::DefaultHalfCalculator::l2_squared_distance(
            3, vec1.data(), vec2.data());
        std::cout << "L2 squared distance: " << l2_dist << std::endl;
        
        // Test inner product
        float inner_prod = halfutils::DefaultHalfCalculator::inner_product(
            3, vec1.data(), vec2.data());
        std::cout << "Inner product: " << inner_prod << std::endl;
        
        // Test L1 distance
        float l1_dist = halfutils::DefaultHalfCalculator::l1_distance(
            3, vec1.data(), vec2.data());
        std::cout << "L1 distance: " << l1_dist << std::endl;
        
        // Test cosine similarity
        double cosine_sim = halfutils::DefaultHalfCalculator::cosine_similarity(
            3, vec1.data(), vec2.data());
        std::cout << "Cosine similarity: " << cosine_sim << std::endl;
        
        // Test namespace functions
        float ns_l2 = halfutils::HalfvecL2SquaredDistance(3, vec1.data(), vec2.data());
        std::cout << "Namespace L2: " << ns_l2 << std::endl;
        
        // Test exception handling
        try {
            halfutils::DefaultHalfCalculator::l2_squared_distance(0, nullptr, nullptr);
        } catch (const std::exception& e) {
            std::cout << "✓ Exception handling works: " << e.what() << std::endl;
        }
        
        std::cout << "All tests passed!" << std::endl;
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
        # Try to compile with both files
        result = subprocess.run([
            'g++', '-std=c++17', '-I.', cpp_file, 'src_cpp/halfutils.cpp',
            '-o', cpp_file + '.out', '-lm'
        ], capture_output=True, text=True)
        
        if result.returncode == 0:
            print("✓ C++ compilation successful")
            # Run the test
            run_result = subprocess.run([cpp_file + '.out'], capture_output=True, text=True)
            print("Output:")
            print(run_result.stdout)
            if run_result.stderr:
                print("Errors:")
                print(run_result.stderr)
            return run_result.returncode == 0
        else:
            print("✗ C++ compilation failed")
            print("Error:", result.stderr)
            return False
            
    finally:
        # Cleanup
        if os.path.exists(cpp_file):
            os.unlink(cpp_file)
        if os.path.exists(cpp_file + '.out'):
            os.unlink(cpp_file + '.out')

if __name__ == "__main__":
    success = test_cpp_compilation()
    if success:
        print("\\n✓ All C++ tests passed!")
    else:
        print("\\n✗ Some tests failed!")
