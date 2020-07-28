#!/bin/bash -x 

mpirun -n 2 ./examples/gpr_fortran_sample <<EOF 2>&1 | tee output.log
fn_gpr_state        saved_gpr_state.dat
fn_training_inputs  regression_test/training_inputs_1D.bin 
fn_training_targets regression_test/training_targets_1D.bin
kernel_type         ARD_with_noise
sample_size         20
dimx                1
maxiter             1000
stepsize            0.001
tol_line            1e-05
tol_gradient        1e-03
signal_to_noise     1e+01
signal_to_kernel    1e+05
EOF
