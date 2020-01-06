#!/bin/bash -x
for site in 1 2 4 5 7 11 12 13 14 16 17 18
do
cd build-${site}
sbatch spgp_train.slurm
cd ../
done
