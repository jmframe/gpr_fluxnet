#!/bin/bash -x 

mpirun -n 2 ./examples/train_gpr_sample <<EOF 2>&1 | tee output.log
gpr_state_filename saved_gpr_state.dat
number_of_inputs 20
number_of_parameters 1
input_filename training_inputs.bin
target_filename training_targets.bin
kernel_type ARD_without_noise
max_number_of_iterations 1000
stepsize 0.01
line_search_tolerance 0.001
gradient_tolerance 1e-5
EOF
