#!/bin/bash

mpirun -n 1 ./examples/sub_sample_statistics <<EOF 2>&1 | tee -a output.log
initial_input_filename   inInputs
initial_target_filename  inTargets
final_input_filename     fiInputs
final_target_filename    fiTargets
initial_num_samples      100
final_num_samples        10
num_parameters           2
EOF
