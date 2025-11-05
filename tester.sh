#!/bin/bash

# Colors for terminal output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Create test results directory
mkdir -p test_results

# Function to evaluate expressions safely
evaluate_expression() {
    local expr="$1"
    # Replace ** with ^ for bc (exponentiation)
    expr=$(echo "$expr" | sed 's/\*\*/^/g')
    # Remove quotes and extra spaces
    expr=$(echo "$expr" | tr -d '"' | sed 's/ //g')
    
    # Use bc for floating point arithmetic
    echo "scale=6; $expr" | bc -l 2>/dev/null || echo "ERROR"
}

# Function to run a single test
run_test() {
    local test_num="$1"
    local input_file="tests/operations_in_${test_num}"
    local expected_file="tests/operations_out_${test_num}"
    local result_file="test_results/operations_${test_num}"
    
    # Check if test files exist
    if [[ ! -f "$input_file" ]]; then
        echo -e "${RED}❌ Test $test_num: Input file $input_file not found${NC}"
        return 1
    fi
    
    if [[ ! -f "$expected_file" ]]; then
        echo -e "${RED}❌ Test $test_num: Expected output file $expected_file not found${NC}"
        return 1
    fi
    
    # Read input from file
    local input=$(cat "$input_file" | tr -d '\n')
    local expected=$(cat "$expected_file" | tr -d '\n')
    
    echo -e "${BLUE}🔍 Running Test $test_num:${NC}"
    echo -e "   Input:    $input"
    echo -e "   Expected: $expected"
    
    # Run the program
    local program_output=$(./bin/calc "$input" 2>/dev/null)
    local program_exit_code=$?
    
    # Clean program output (remove newlines, extra spaces)
    program_output=$(echo "$program_output" | tr -d '\n' | xargs)
    
    # Calculate actual result using bc for comparison
    local actual_result=$(evaluate_expression "$input")
    
    echo -e "   Got:      $program_output"
    echo -e "   Actual:   $actual_result"
    
    # Compare results
    if [[ $program_exit_code -ne 0 ]]; then
        echo -e "${RED}❌ Test $test_num FAILED - Program crashed with exit code $program_exit_code${NC}"
        echo "Input: $input" > "$result_file"
        echo "Program Output: $program_output" >> "$result_file"
        echo "Exit Code: $program_exit_code" >> "$result_file"
        return 1
    elif [[ "$program_output" == "$expected" ]]; then
        echo -e "${GREEN}✅ Test $test_num PASSED${NC}"
        return 0
    else
        echo -e "${RED}❌ Test $test_num FAILED - Output mismatch${NC}"
        echo "Input: $input" > "$result_file"
        echo "Expected: $expected" >> "$result_file"
        echo "Got: $program_output" >> "$result_file"
        echo "Actual Calculation: $actual_result" >> "$result_file"
        return 1
    fi
}

# Function to find all test files
find_tests() {
    local tests=()
    for file in tests/operations_in_*; do
        if [[ -f "$file" ]]; then
            local test_num=$(echo "$file" | grep -o '[0-9]\+$')
            tests+=("$test_num")
        fi
    done
    printf '%s\n' "${tests[@]}" | sort -n
}

# Main execution
main() {
    echo -e "${YELLOW}🚀 Starting Calculator Tests${NC}"
    echo "================================"
    
    # Check if program exists
    if [[ ! -f "./bin/calc" ]]; then
        echo -e "${RED}❌ Error: Program ./bin/calc not found${NC}"
        echo "Please build your program first and place it at ./bin/calc"
        exit 1
    fi
    
    # Check if tests directory exists
    if [[ ! -d "tests" ]]; then
        echo -e "${RED}❌ Error: tests directory not found${NC}"
        exit 1
    fi
    
    # Find and run all tests
    local tests=($(find_tests))
    local total_tests=${#tests[@]}
    local passed_tests=0
    local failed_tests=0
    
    if [[ $total_tests -eq 0 ]]; then
        echo -e "${YELLOW}⚠️  No tests found in tests/ directory${NC}"
        echo "Test files should be named: operations_in_01, operations_in_02, etc."
        echo "With corresponding: operations_out_01, operations_out_02, etc."
        exit 1
    fi
    
    echo -e "${BLUE}📋 Found $total_tests test(s)${NC}"
    echo
    
    for test_num in "${tests[@]}"; do
        if run_test "$test_num"; then
            ((passed_tests++))
        else
            ((failed_tests++))
        fi
        echo
    done
    
    # Summary
    echo "================================"
    echo -e "${YELLOW}📊 TEST SUMMARY${NC}"
    echo -e "Total Tests: $total_tests"
    echo -e "${GREEN}Passed: $passed_tests${NC}"
    
    if [[ $failed_tests -gt 0 ]]; then
        echo -e "${RED}Failed: $failed_tests${NC}"
        echo -e "Check test_results/ directory for failed test details"
        exit 1
    else
        echo -e "${GREEN}🎉 All tests passed!${NC}"
        exit 0
    fi
}

# Run main function
main "$@"
