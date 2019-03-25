#!/bin/bash

if [[ -z $1 ]];
then
    echo "Pass the path of the tests executable as the first parameter"
    exit 1
fi

tests_executable=$(realpath $1)

shift 1

if [[ ! -f $tests_executable ]];
then
    echo "The executable $tests_executable does not exist, have you tried building the project first?"
    exit 1
fi

tests_executable_dir=$(dirname $tests_executable)
tests_profraw=$tests_executable_dir/default.profraw

# Run the executable passing the path to default.profraw to llvm
echo "Running the tests..."
LLVM_PROFILE_FILE=$tests_profraw $tests_executable $@

if [[ $? -ne 0 ]];
then
    echo "Tests failed"
    exit 1
fi

if [[ ! -f $tests_profraw ]];
then
    echo "The file $tests_profraw does not exist, make sure you are compiling with profiling enabled"
    exit 1
fi

tests_profdata=$tests_executable_dir/default.profdata

# Find all relevant files for the test coverage
relevant_include_files=$(find include -path "include/pitaya/protos" -prune -o -type f -print)

xcrun llvm-profdata merge $tests_profraw -o $tests_profdata
xcrun llvm-cov show $tests_executable -instr-profile=$tests_profdata -format=html -output-dir=$tests_executable_dir/coverage $(pwd)/src $relevant_include_files

echo "Generated HTML coverage report at $tests_executable_dir/coverage"

open $tests_executable_dir/coverage/index.html
