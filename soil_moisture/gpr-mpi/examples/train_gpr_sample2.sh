#!/bin/bash -x 
if [ -e output.log]
then
  echo "Removing output log"
  rm output.log
fi

var="sm1"
in="X"
state="wet"
mpirun -n 1 ./examples/train_gpr_sample2 <<EOF 2>&1 | tee -a output.log
gpr_state_filename trained_${var}.${state}.dat
number_of_inputs 850
number_of_parameters 35
input_filename  /gpfsm/dnb02/cpelissi/LIS/run_10k/inputs.${in}.${state}.dat
target_filename /gpfsm/dnb02/cpelissi/LIS/run_10k/targets.${in}.${var}.${state}.dat
kernel_type ARD_with_noise
max_number_of_iterations 1000
stepsize 0.001
line_search_tolerance 0.001
gradient_tolerance 1e-5
fraction_training   0.882353
fraction_validation 0.117647
fraction_test       0.00
training_sample_increment 0.04
start_increment 5
max_increments  20 
signal_to_noise 0.001
isUsePreviousHyperp 0
EOF
#mpirun -n 1 ./examples/train_gpr_sample2 <<EOF 2>&1 | tee -a output.log
#gpr_state_filename trained_${var}.${state}.dat
#number_of_inputs 8000
#number_of_parameters 34
#input_filename  /gpfsm/dnb02/cpelissi/LIS/run_10k/inputs.${in}.${state}.dat
#target_filename /gpfsm/dnb02/cpelissi/LIS/run_10k/targets.${in}.${var}.${state}.dat
#kernel_type ARD_with_noise
#max_number_of_iterations 1000
#stepsize 0.001
#line_search_tolerance 0.001
#gradient_tolerance 1e-5
#fraction_training   0.25
#fraction_validation 0.05
#fraction_test       0.00
#training_sample_increment 0.05
#start_increment 0
#max_increments  1 
#signal_to_noise 0.001
#isUsePreviousHyperp 0
#EOF
