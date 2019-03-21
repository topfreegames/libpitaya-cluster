#!/bin/bash

if [[ -z $1 ]];
then
    echo "Pass the path of the tests executable as the first parameter"
    exit 1
fi

tests_executable=$(realpath $1)

if [[ ! -f $tests_executable ]];
then
    echo "The executable $tests_executable does not exist, have you tried building the project first?"
    exit 1
fi

tests_executable_dir=$(dirname $tests_executable)
tests_profraw=$tests_executable_dir/default.profraw

if [[ ! -f $tests_profraw ]];
then
    echo "The file $tests_profraw does not exist, have you tried running the executable? (Also make sure that the project was created with -DBUILD_TESTING=ON)"
    exit 1
fi

tests_profdata=$tests_executable_dir/default.profdata

xcrun llvm-profdata merge $tests_profraw -o $tests_profdata
xcrun llvm-cov show $tests_executable -instr-profile=$tests_profdata -format=html -output-dir=$tests_executable_dir/coverage $(pwd)/src $(pwd)/include/pitaya

echo "Generated HTML coverage report at $tests_executable_dir/coverage"

open $tests_executable_dir/coverage/index.html
