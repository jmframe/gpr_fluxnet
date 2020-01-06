#!/bin/bash -x
dim=10
l="wet"
ntrain=$(cat "num_wet_training_points.txt")
projdir="/discover/nobackup/jframe/gpr_fluxnet/soil_moisture/"
traindir=${projdir}"gpr-mpi/build-1/"
rundir=${projdir}"/gpr-noah/build-1/"
repdir=${traindir}"reports/"
lowrmse=999
lown=750
lowns=3

# Reset the msqe file. This is actually rmse, might want to change
if [ -e ${traindir}msqe_${l}.txt ]
then
  rm ${traindir}msqe_${l}.txt
fi
# Reset these NaN files, maybe reset the whole repdir
if [ -e ${repdir}NaN_${l}.txt ]
then
  rm ${repdir}NaN_${l}.txt
fi

###########    LOOP No sub-inputs
for n in `seq ${1} 100 ${2}`
do

# Run the sub-sample statistics for this particular set.
# Note that sub-sample statistics can only run with one processor.
np=1
./sub_sample_statistics_${l}.sh ${n} 2>&1 | tee ${repdir}${n}_sub.log

###########    LOOP No PSEUDO-INPUTS
for ns in `seq ${3} 1 ${4}`
do
np=`echo "${n}/400+1" | bc`

tag=`printf %03d $ns`

echo 'n '${n}
echo 'ns '${ns}

mpirun -n ${np} ${traindir}examples/v1_train_gpr_sample <<EOF 2>&1 | tee ${repdir}${ns}_${l}_output.log
gpr_state_filename       ${l}_gpr.dat
number_of_inputs         ${n}
number_of_parameters     ${dim}
number_pseudo_inputs     ${ns}
input_filename           sub_inputs_${l}.bin
target_filename          sub_targets_${l}.bin
kernel_type              ARD_with_noise
approx_type              spgp
max_number_of_iterations 40000
stepsize                 0.001
line_search_tolerance    0.001
gradient_tolerance       1e-5
signal_to_noise          1e2
signal_to_kernel         1e8
max_time                 5000
isRead                   0
EOF

mpirun -n 32 ${traindir}examples/v1_train_gpr_sample <<EOF 2>&1 | tee ${repdir}${ns}_${l}_output.log
gpr_state_filename       ${l}_gpr.dat
number_of_inputs         ${ntrain}
number_of_parameters     ${dim}
number_pseudo_inputs     ${ns}
input_filename           inputs_${l}.bin
target_filename          targets_${l}.bin
kernel_type              ARD_with_noise
approx_type              spgp
max_number_of_iterations 10040
stepsize                 0.001
line_search_tolerance    0.001
gradient_tolerance       1e-4
signal_to_noise          1e1
signal_to_kernel         1e6
max_time                 5000
isRead                   1
EOF

# Copy the gpr state to the run directory
cp ${l}_gpr.dat ${rundir}
# Go into run directory and run the model
cd ${rundir}
# Run noah executable with options for dynamic state update
./noah_mp.exe
# Calculate the root mean squared error
ms=`python msqe.py`
# write the root mean squared error to file
echo ${ms} ${tag} ${n} >> ${traindir}msqe_${l}.txt
# Go bach to the training directory
cd ${traindir}
# Set n and ns for lowest rmse value
# Bash doesn't do floating point numbers
if [ $(bc <<< "${ms} < ${lowrmse}") -eq 1 ]
then
  echo 'Updating the low values' ${ms} ${tag} ${n}
  lowrmse=${ms}
  lown=${n}
  lowns=${ns}
fi

# End n loop (number of sub-samples)
done
# End ns loop (number of pseudo inputs)
done

#####################
# RUN WITH BEST SET #
#####################
np=1
./sub_sample_statistics_${l}.sh ${lown} 2>&1 | tee ${repdir}${lown}_sub_low.log
tag=`printf %03d $lowns`
np=`echo "${lown}/400+1" | bc`
mpirun -n ${np} ${traindir}examples/v1_train_gpr_sample <<EOF 2>&1 | tee ${repdir}${lowns}_${l}_output_low.log
gpr_state_filename       ${l}_gpr.dat
number_of_inputs         ${lown}
number_of_parameters     ${dim}
number_pseudo_inputs     ${lowns}
input_filename           sub_inputs_${l}.bin
target_filename          sub_targets_${l}.bin
kernel_type              ARD_with_noise
approx_type              spgp
max_number_of_iterations 40000
stepsize                 0.001
line_search_tolerance    0.001
gradient_tolerance       1e-5
signal_to_noise          1e2
signal_to_kernel         1e8
max_time                 5000
isRead                   0
EOF
mpirun -n 32 ${traindir}examples/v1_train_gpr_sample <<EOF 2>&1 | tee ${repdir}${lowns}_${l}_output_low.log
gpr_state_filename       ${l}_gpr.dat
number_of_inputs         ${ntrain}
number_of_parameters     ${dim}
number_pseudo_inputs     ${lowns}
input_filename           inputs_${l}.bin
target_filename          targets_${l}.bin
kernel_type              ARD_with_noise
approx_type              spgp
max_number_of_iterations 10040
stepsize                 0.001
line_search_tolerance    0.001
gradient_tolerance       1e-4
signal_to_noise          1e1
signal_to_kernel         1e6
max_time                 5000
isRead                   1
EOF
cp ${l}_gpr.dat ${rundir}
cd ${rundir}
./noah_mp.exe
ms=`python msqe.py`
echo 'This should be the best set:' >> ${traindir}msqe_${l}.txt
echo ${ms} ${tag} ${lown} >> ${traindir}msqe_${l}.txt
cp output.out output.gprwet
cd ${traindir}
