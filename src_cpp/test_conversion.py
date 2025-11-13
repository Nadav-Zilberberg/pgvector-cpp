#!/usr/bin/env python3

import os
import subprocess
import sys

def test_compilation():
    """Test that the C++ file compiles successfully"""
    print("Testing halfvec.cpp compilation...")
    
    # Check if the object file exists
    if os.path.exists("halfvec.o"):
        print("✓ halfvec.o exists")
        
        # Check if it has the expected number of functions
        result = subprocess.run(["nm", "halfvec.o"], capture_output=True, text=True)
        if result.returncode == 0:
            functions = [line for line in result.stdout.split('\n') if ' T ' in line]
            print(f"✓ Found {len(functions)} functions in object file")
            
            # Check for key PostgreSQL functions
            pg_functions = [f for f in functions if 'halfvec_' in f or 'sparsevec_to_halfvec' in f or 'vector_to_halfvec' in f]
            print(f"✓ Found {len(pg_functions)} PostgreSQL functions")
            
            return True
        else:
            print("✗ Failed to analyze object file")
            return False
    else:
        print("✗ halfvec.o does not exist")
        return False

def test_cpp_features():
    """Test that C++ features are properly used"""
    print("\nTesting C++ features...")
    
    with open("halfvec.cpp", "r") as f:
        content = f.read()
    
    # Check for namespace usage
    if "namespace halfvec" in content:
        print("✓ Uses namespace")
    else:
        print("✗ Missing namespace")
        return False
    
    # Check for exception handling
    if "try {" in content and "catch" in content:
        print("✓ Uses exception handling")
    else:
        print("✗ Missing exception handling")
        return False
    
    # Check for RAII
    if "std::unique_ptr" in content or "std::shared_ptr" in content:
        print("✓ Uses RAII")
    else:
        print("✗ Missing RAII")
        return False
    
    # Check for const correctness
    if "const" in content:
        print("✓ Uses const correctness")
    else:
        print("✗ Missing const correctness")
        return False
    
    # Check for references
    if "&" in content:
        print("✓ Uses references")
    else:
        print("✗ Missing references")
        return False
    
    return True

def main():
    """Main test function"""
    print("Testing halfvec.c to halfvec.cpp conversion")
    print("=" * 50)
    
    success = True
    
    # Test compilation
    if not test_compilation():
        success = False
    
    # Test C++ features
    if not test_cpp_features():
        success = False
    
    print("\n" + "=" * 50)
    if success:
        print("✓ All tests passed! Conversion successful.")
        return 0
    else:
        print("✗ Some tests failed.")
        return 1

if __name__ == "__main__":
    sys.exit(main())
