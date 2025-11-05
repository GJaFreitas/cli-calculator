#!/usr/bin/env python3

import os
import subprocess
import glob
import re
from pathlib import Path

class FixedTestRunner:
    def __init__(self):
        self.colors = {
            'RED': '\033[0;31m',
            'GREEN': '\033[0;32m',
            'YELLOW': '\033[1;33m',
            'BLUE': '\033[0;34m',
            'NC': '\033[0m'
        }
        
        Path("test_results").mkdir(exist_ok=True)
    
    def color_print(self, color, message):
        print(f"{self.colors[color]}{message}{self.colors['NC']}")
    
    def run_test(self, test_num):
        """Run a single test with proper argument handling"""
        input_file = f"tests/operations_in_{test_num:02d}"
        expected_file = f"tests/operations_out_{test_num:02d}"
        result_file = f"test_results/operations_{test_num:02d}"
        
        # Check if test files exist
        if not os.path.exists(input_file):
            self.color_print('RED', f"❌ Test {test_num:02d}: Input file {input_file} not found")
            return False
        
        if not os.path.exists(expected_file):
            self.color_print('RED', f"❌ Test {test_num:02d}: Expected output file {expected_file} not found")
            return False
        
        # Read input from file
        with open(input_file, 'r') as f:
            input_content = f.read().strip()
        
        with open(expected_file, 'r') as f:
            expected = f.read().strip()
        
        self.color_print('BLUE', f"🔍 Running Test {test_num:02d}:")
        print(f"   Input:    {input_content}")
        print(f"   Expected: {expected}")
        
        try:
            # KEY FIX: Use shell=False and pass the argument directly
            # Don't try to parse or modify the input content
            result = subprocess.run(
                ["./bin/calc", input_content],  # Pass the string as single argument
                capture_output=True,
                text=True,
                timeout=5
            )
            
            program_output = result.stdout.strip()
            
            print(f"   Got:      '{program_output}'")
            
            if program_output == expected:
                self.color_print('GREEN', f"✅ Test {test_num:02d} PASSED")
                return True
            else:
                self.color_print('RED', f"❌ Test {test_num:02d} FAILED - Output mismatch")
                with open(result_file, 'w') as f:
                    f.write(f"Input: {input_content}\n")
                    f.write(f"Expected: {expected}\n")
                    f.write(f"Got: {program_output}\n")
                return False
                
        except subprocess.TimeoutExpired:
            self.color_print('RED', f"❌ Test {test_num:02d} FAILED - Program timed out")
            return False
        except Exception as e:
            self.color_print('RED', f"❌ Test {test_num:02d} FAILED - Exception: {e}")
            return False
    
    def find_tests(self):
        """Find all test files"""
        test_files = glob.glob("tests/operations_in_*")
        tests = []
        for file in test_files:
            match = re.search(r'operations_in_(\d+)', file)
            if match:
                tests.append(int(match.group(1)))
        return sorted(tests)
    
    def run_all_tests(self):
        """Run all tests"""
        self.color_print('YELLOW', "🚀 Starting Calculator Tests")
        print("================================")
        
        # Check if program exists
        if not os.path.exists("./bin/calc"):
            self.color_print('RED', "❌ Error: Program ./bin/calc not found")
            return 1
        
        # Check if tests directory exists
        if not os.path.exists("tests"):
            self.color_print('RED', "❌ Error: tests directory not found")
            return 1
        
        # Find and run all tests
        tests = self.find_tests()
        total_tests = len(tests)
        
        if total_tests == 0:
            self.color_print('YELLOW', "⚠️  No tests found in tests/ directory")
            print("Test files should be named: operations_in_01, operations_in_02, etc.")
            print("With corresponding: operations_out_01, operations_out_02, etc.")
            return 1
        
        self.color_print('BLUE', f"📋 Found {total_tests} test(s)")
        print()
        
        passed = 0
        failed = 0
        
        for test_num in tests:
            if self.run_test(test_num):
                passed += 1
            else:
                failed += 1
            print()
        
        # Summary
        print("================================")
        self.color_print('YELLOW', "📊 TEST SUMMARY")
        print(f"Total Tests: {total_tests}")
        self.color_print('GREEN', f"Passed: {passed}")
        
        if failed > 0:
            self.color_print('RED', f"Failed: {failed}")
            self.color_print('BLUE', "Check test_results/ directory for failed test details")
            return 1
        else:
            self.color_print('GREEN', "🎉 All tests passed!")
            return 0

if __name__ == "__main__":
    runner = FixedTestRunner()
    exit_code = runner.run_all_tests()
    exit(exit_code)
