#!/bin/sh

dir=$(cd "$(dirname $0)";pwd)
python3 ${dir}/src/main.py $@
