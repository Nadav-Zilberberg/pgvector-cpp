#!/usr/bin/env python3

import subprocess
import os
import tempfile

def test_c_compatibility():
    print("Testing C compatibility...")
    
    # Create a test C file that uses the original interface
    test_c = """
#include "src/halfutils.h"
#include "src/halfvec.h"
#include <stdio.h>
#include <stdlib.h>

int main() {
    printf("Testing C compatibility...\\n");
    
    // Initialize
    HalfvecInit();
    printf("✓ C initialization successful\\n");
    
    // Create some test data
    HalfVector *vec1 = malloc(sizeof(HalfVector) + 3 * sizeof(half));
    HalfVector *vec2 = malloc(sizeof(HalfVector) + 3 * sizeof(half));
    
    vec1->dim = 3;
    vec2->dim = 3;
    
    // Set some test values (simplified)
    vec1->x[0] = 1.0f;
    vec1->x[1] = 2.0f;
    vec1->x[2] = 3.0f;
    
    vec2->x[0] = 4.0f;
    vec2->x[1] = 5.0f;
    vec2->x[2] = 6.0f;
    
    // Test functions
    float l2_dist = HalfvecL2SquaredDistance(3, vec1->x, vec2->x);
    printf("C L2 squared distance: %f\\n", l2_dist);
    
    float inner_prod = HalfvecInnerProduct(3, vec1->x, vec2->x);
    printf("C Inner product: %f\\n", inner_prod);
    
    float l1_dist = HalfvecL1Distance(3, vec1->x, vec2->x);
    printf("C L1 distance: %f\\n", l1_dist);
    
    double cosine_sim = HalfvecCosineSimilarity(3, vec1->x, vec2->x);
    printf("C Cosine similarity: %f\\n", cosine_sim);
    
    free(vec1);
    free(vec2);
    
    printf("C compatibility test passed!\\n");
    return 0;
}
"""
    
    with tempfile.NamedTemporaryFile(mode='w', suffix='.c', delete=False) as f:
        f.write(test_c)
        c_file = f.name
    
    try:
        # Try to compile with original C files
        result = subprocess.run([
            'gcc', '-I.', c_file, 'src/halfutils.c',
            '-o', c_file + '.out', '-lm'
        ], capture_output=True, text=True)
        
        if result.returncode == 0:
            print("✓ C compilation successful")
            # Run the test
            run_result = subprocess.run([c_file + '.out'], capture_output=True, text=True)
            print("Output:")
            print(run_result.stdout)
            if run_result.stderr:
                print("Errors:")
                print(run_result.stderr)
            return run_result.returncode == 0
        else:
            print("✗ C compilation failed")
            print("Error:", result.stderr)
            return False
            
    finally:
        # Cleanup
        if os.path.exists(c_file):
            os.unlink(c_file)
        if os.path.exists(c_file + '.out'):
            os.unlink(c_file + '.out')

if __name__ == "__main__":
    success = test_c_compatibility()
    if success:
        print("\\n✓ C compatibility test passed!")
    else:
        print("\\n✗ C compatibility test failed!")
