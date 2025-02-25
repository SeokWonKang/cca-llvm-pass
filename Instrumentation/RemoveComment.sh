#!/bin/bash
# Usage: bash RemoveComment.sh <assembly>

OUTPUT_PATH=0
for arg in $@
do
	if [[ $OUTPUT_PATH != 0 ]] 
	then 
		sed -e 's/\#removethiscomment //g' -i $arg
		sed -e 's/\/\/removethiscomment //g' -i $arg
		break
	fi
	if [ $arg == '-o' ]; then OUTPUT_PATH=1; fi
done
