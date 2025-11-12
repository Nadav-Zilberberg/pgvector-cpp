#!/usr/bin/env python3

import subprocess
import os
import tempfile

def test_c_compatibility_new():
    print("Testing C compatibility with new C++ implementation...")
    
    # Create a test C file that uses the compatibility interface
    test_c = """
#include "src_cpp/halfutils_compat.h"
#include <stdio.h>
#include <stdlib.h>

int main() {
    printf("Testing C compatibility with C++ implementation...\\n");
    
    // Initialize
    HalfvecInit();
    printf("✓ C initialization successful\\n");
    
    // Create some test data
    half vec1[3];
    half vec2[3];
    
    // Set some test values (simplified - using uint16 representation)
    // These are approximate float values in half format
    vec1[0] = 15360; // ~1.0f
    vec1[1] = 16384; // ~2.0f  
    vec1[2] = 16896; // ~3.0f
    
    vec2[0] = 17408; // ~4.0f
    vec2[1] = 17664; // ~5.0f
    vec2[2] = 17856; // ~6.0f
    
    // Test functions
    float l2_dist = HalfvecL2SquaredDistance(3, vec1, vec2);
    printf("C L2 squared distance: %f\\n", l2_dist);
    
    float inner_prod = HalfvecInnerProduct(3, vec1, vec2);
    printf("C Inner product: %f\\n", inner_prod);
    
    float l1_dist = HalfvecL1Distance(3, vec1, vec2);
    printf("C L1 distance: %f\\n", l1_dist);
    
    double cosine_sim = HalfvecCosineSimilarity(3, vec1, vec2);
    printf("C Cosine similarity: %f\\n", cosine_sim);
    
    printf("C compatibility test passed!\\n");
    return 0;
}
"""
    
    with tempfile.NamedTemporaryFile(mode='w', suffix='.c', delete=False) as f:
        f.write(test_c)
        c_file = f.name
    
    try:
        # Try to compile with compatibility layer
        result = subprocess.run([
            'gcc', '-I.', c_file, 'src_cpp/halfutils_compat.cpp', 'src_cpp/halfutils.cpp',
            '-o', c_file + '.out', '-lm', '-lstdc++'
        ], capture_output=True, text=True)
        
        if result.returncode == 0:
            print("✓ C compatibility compilation successful")
            # Run the test
            run_result = subprocess.run([c_file + '.out'], capture_output=True, text=True)
            print("Output:")
            print(run_result.stdout)
            if run_result.stderr:
                print("Errors:")
                print(run_result.stderr)
            return run_result.returncode == 0
        else:
            print("✗ C compatibility compilation failed")
            print("Error:", result.stderr)
            return False
            
    finally:
        # Cleanup
        if os.path.exists(c_file):
            os.unlink(c_file)
        if os.path.exists(c_file + '.out'):
            os.unlink(c_file + '.out')

if __name__ == "__main__":
    success = test_c_compatibility_new()
    if success:
        print("\\n✓ C compatibility test passed!")
    else:
        print("\\n✗ C compatibility test failed!")
