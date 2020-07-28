#!/bin/bash -x
#SBATCH --job-name="1-wetOnly"
#SBATCH --account=s2027
#SBATCH --time=12:00:00
#SBATCH --constraint=hasw
#SBATCH --output="slurm.out"
#SBATCH --error="slurm.err"
#SBATCH --ntasks=2

# memory
ulimit -s unlimited

source /usr/share/modules/init/bash
#-------------------------------------------------------
# NASA  Advanced Software Technology Group
# Configure script for FEWSNET Operational run with LIS6
#-------------------------------------------------------

module purge
module load comp/intel-14.0.3.174
module load mpi/impi-5.0.3.048
module load lib/mkl-10.1.2.024
module load other/comp/gcc-4.8.1
module load other/cmake-3.8.2

#-------------
# GPR Settings
#-------------
export CXX=mpiicpc
export CC=mpiicc
export GXX_NAME=g++
export FORTRAN_HOME=/usr/local/intel/Composer/composer_xe_2013_sp1.3.174/compiler/lib/intel64
export BLAS_HOME=/usr/local/other/SLES11/BLAS/intel-14.0.3.174
export LAPACK_HOME=/usr/local/other/SLES11/lapack/3.5.0/intel-14.0.3.174/lib
export SCALAPACK_HOME=/usr/local/other/SLES11/ScaLAPACK/2.0.2/intel-14.0.3.174/lib
export GSL_HOME=/usr/local/other/SLES11/gsl/2.4/intel-14.0.3.174

dim=10
l="wet"
sitenumber=$(cat "site.txt")
ntrain=$(cat "num_wet_training_points.txt")
projdir="/discover/nobackup/jframe/gpr_fluxnet/soil_moisture/"
traindir=${projdir}"gpr-mpi/build-"${sitenumber}"/"
rundir=${projdir}"/gpr-noah/build-"${sitenumber}"/"
repdir=${traindir}"reports/"
lowrmse=999
lown=750
lowns=3
SIlow=300
SIhi=2500
PSlow=2
PShi=18

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
for n in `seq ${SIlow} 100 ${SIhi}`
do

# Run the sub-sample statistics for this particular set.
# Note that sub-sample statistics can only run with one processor.
np=1
./sub_sample_statistics_${l}.sh ${n} 2>&1 | tee ${repdir}${n}_sub.log

###########    LOOP No PSEUDO-INPUTS
for ns in `seq ${PSlow} 1 ${PShi}`
do

echo 'Running GPR for number sub inputs = '${n}
echo 'Running GPR for number pseudo inputs = '${ns}

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
