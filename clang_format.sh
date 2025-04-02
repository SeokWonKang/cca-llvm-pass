#!/bin/bash

SCRIPT_DIR="$(readlink -f "$(dirname "${BASH_SOURCE[0]:-${(%):-%x}}")")"

for srcfile in $(ls $SCRIPT_DIR/Instrumentation/*.{hpp,cpp}) 
do
	vim +ClangFormat +wqa $srcfile
	echo update $srcfile
done
