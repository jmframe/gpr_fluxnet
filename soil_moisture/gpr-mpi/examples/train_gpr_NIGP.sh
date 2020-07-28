#!/bin/bash -x 

mpirun -n 2 ./examples/train_gpr_NIGP <<EOF 2>&1 | tee output.log
gpr_state_filename       saved_gpr_state_dry.dat
nigp_state_filename      nigp_saved_gpr_state_dry.dat
number_of_inputs         100
number_of_parameters     10
input_filename           inputs_dry.bin
target_filename          targets_dry.bin
kernel_type              ARD_with_noise
max_number_of_iterations 1000
stepsize                 0.001
line_search_tolerance    0.001
gradient_tolerance       1e-5
signal_to_noise          1e2
signal_to_kernel         1e6
max_time                 6000
isRead                   1
num_refinements          5
h2a                      5e-1
EOF
