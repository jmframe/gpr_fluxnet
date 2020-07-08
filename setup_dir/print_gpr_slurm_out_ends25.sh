#!/bin/bash -x
file=slurm.out
for d in soil_moisture/gpr-mpi/*/ ; do
    echo "$d"
    tail -n 25 ${d}${file}
done
