#!/bin/bash -x 

out=output.log
if [ -e ${out} ]
then
  echo "Removing output log"
  rm ${out}
fi

var="sm1"
in="X"
state="dry"
mpirun -n 1 ./examples/train_gpr_kfold <<EOF 2>&1 | tee -a output.log
gpr_state_filename           trained_${var}.${state}.dat
summary_filename             summary_${var}.${state}.dat
number_of_samples            400
number_of_parameters         34
input_filename               ../data/inputs.${in}.${state}.dat
target_filename              ../data/targets.${in}.${var}.${state}.dat
kernel_type                  ARD_with_noise
max_number_of_iterations     1000
stepsize                     0.001
line_search_tolerance        0.001
gradient_tolerance           1e-5
num_kfolds                   4
training_sample_increment    0.33333333333
begin_increment              1
end_increment                3
signal_to_noise              0.001
isUsePreviousHyperp          0
isWriteGPRState              0
EOF
