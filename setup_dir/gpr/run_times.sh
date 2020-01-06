#!/bin/bash -x
for site in 1 2 4 5 7 11 12 13 14 16 17 18
do
    tail -n 14 build-${site}/slurm.out
done
