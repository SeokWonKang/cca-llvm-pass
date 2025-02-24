#!/bin/bash
# Usage: bash RemoveComment.sh <assembly>

sed -e 's/\#removethiscomment //g' -i $1
