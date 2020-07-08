#!/bin/bash -x
file=rmse_wet.txt
for d in soil_moisture/gpr-mpi/*/ ; do
    echo "$d"
    tail -n 1 ${d}${file}
done
