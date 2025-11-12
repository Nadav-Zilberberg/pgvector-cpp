#!/usr/bin/env python3

import subprocess
import os

# Test if the current implementation compiles
def test_compilation():
    print("Testing current halfutils compilation...")
    
    # Try to compile a simple test
    test_c = """
#include "src/halfutils.h"
#include "src/halfvec.h"
#include <stdio.h>

int main() {
    printf("Testing halfutils initialization...\\n");
    HalfvecInit();
    printf("Halfutils initialized successfully\\n");
    return 0;
}
"""
    
    with open('/tmp/test_halfutils.c', 'w') as f:
        f.write(test_c)
    
    # Try to compile
    result = subprocess.run([
        'gcc', '-I.', '/tmp/test_halfutils.c', 'src/halfutils.c', 
        '-o', '/tmp/test_halfutils', '-lm'
    ], capture_output=True, text=True)
    
    if result.returncode == 0:
        print("✓ Compilation successful")
        # Run the test
        run_result = subprocess.run(['/tmp/test_halfutils'], capture_output=True, text=True)
        print("Output:", run_result.stdout)
        return True
    else:
        print("✗ Compilation failed")
        print("Error:", result.stderr)
        return False

if __name__ == "__main__":
    test_compilation()
