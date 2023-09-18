#! /bin/bash

find . -type f  \( -name "*.cc" -o -name "*.hpp" \)  -exec wc -l {} \; | tee .project_code_sum.txt
printf "\ntotal: "
awk '{ sum += $1 } END {print sum}' .project_code_sum.txt