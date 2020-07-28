#!/bin/bash -x 

cd "/Users/cpelissi/projects/gpr_initialization/build"

mpirun -n 1 ./examples/make_predictions_sample <<EOF 2>&1 | tee output.log
gpr_state_filename  saved_gpr_state.dat  
number_of_inputs  20
input_filename valid_inputs.bin
predictions_filename prediction_targets.bin
target_covariance_filename  covariance_filename
isCovariance 0
EOF
