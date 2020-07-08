#!/bin/bash -x
file=sce.out
for d in ../soil_moisture/cal_dirs/*/ ; do
    echo "$d"
    tail -n 1 ${d}${file}
done
