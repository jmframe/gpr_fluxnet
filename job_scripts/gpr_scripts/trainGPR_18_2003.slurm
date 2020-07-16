#!/bin/bash -x
#SBATCH --job-name="18-2003-spgp"
#SBATCH --account=s2027
#SBATCH --time=12:00:00
#SBATCH --constraint=hasw
#SBATCH --output="/discover/nobackup/jframe/gpr_fluxnet/soil_moisture/gpr-mpi/18_2003/slurm.out"
#SBATCH --error="/discover/nobackup/jframe/gpr_fluxnet/soil_moisture/gpr-mpi/18_2003/slurm.err"
#SBATCH --ntasks=84

# memory
ulimit -s unlimited

source /usr/share/modules/init/bash

module purge
module load comp/intel/19.1.0.166
module load mpi/impi/20.0.0.166

# Dimensions of the GPR
dwet=18
ddry=17
# Value for searching for best pseudo sets.
PSlow=3
PShi=33
dPS=1
SKlow=5
SKhi=6
dSK=1
SHlow=8
SHhi=16
dSH=2
# Stuff that will be used in each instance of training
pythonpath="/discover/nobackup/jframe/anaconda3/bin/python"
siteyear="18_2003"
projdir="/discover/nobackup/jframe/gpr_fluxnet/"
projdirsm=${projdir}"soil_moisture/"
mpidir=${projdirsm}"gpr-mpi/18_2003/"
noahtraindir=${projdirsm}"gpr-noah/train_18_2003/"
noahtestdir=${projdirsm}"gpr-noah/test_18_2003/"
repdir=${mpidir}"reports/"
# Values for finding the best gpr.
lowrmse=999
lown=750
lowns=3
lowsk=8
lowsh=1
####################    Initialize the basic noah model     ##############################################
cd ${noahtraindir}
./noah_mp.exe 0 0 0 11
####################   Then run the onestep and equilibrate that
./noah_mp.exe 1 0 0 11
####################   Make the gpr input data
cd ${projdir}
./make_gpr_training_data.py $siteyear $mpidir $noahtraindir

cd ${mpidir}

####################    Defined function for running the GPR     ##############################################
#function trainGPR np mpidir repdir ns l n dim s2k ntrain nsh nproc
function trainGPR
{
# Directory for the training code.
train_gpr_dir=${projdir}'soil_moisture/gpr-mpi/build/examples/'
# parameters for gpr
f_ss=0.001
f_lst=0.0001
f_gt=1e-5
f_mpidir=${1}
f_repdir=${2}
f_ns=${3}
f_l=${4}
f_dim=${5}
f_s2k=${6}
f_ntrain=${7}
f_nsh=${8}
f_nproc=${9}
mpirun -n ${f_nproc} ${train_gpr_dir}train_gpr_sample <<EOF 2>&1 | tee ${f_repdir}${f_ns}_${f_l}_output.log
gpr_state_filename       ${f_l}_gpr.dat
number_of_inputs         ${f_ntrain}
number_of_parameters     ${f_dim}
number_pseudo_inputs     ${f_ns}
input_filename           inputs_${f_l}.bin
target_filename          targets_${f_l}.bin
kernel_type              ARD_without_noise
approx_type              spgp
max_number_of_iterations 10040
stepsize                 ${f_ss}
line_search_tolerance    ${f_lst}
gradient_tolerance       ${f_gt}
signal_to_noise          1e1
signal_to_kernel         ${f_s2k}
max_time                 5000
isRead                   0
nShuffles                ${f_nsh}
EOF
}

###########################################################################################################
#####     DRY ONLY TRAINING     #####     DRY ONLY TRAINING     #####     DRY ONLY TRAINING     ###########
###########################################################################################################
cd ${mpidir}
# Wet/Dry specific stuff for training
dim=$ddry
l="dry"
ntrain=$(cat "num_"${l}"_training_points.txt")
nproc=84
# Reset the rmse file.
rmsefilename=${mpidir}'rmse_'${l}'.txt'
if [ -e $rmsefilename ]
then
  rm $rmsefilename
fi
echo 'rmse pseudo-inputs nShuffles sig2kernel' >> $rmsefilename
# reset the whole repdir
rm -rf ${repdir}
mkdir ${repdir}

###########    LOOP No PSEUDO-INPUTS
for ns in `seq ${PSlow} ${dPS} ${PShi}`
do
###########    LOOP SIGNAL TO KERNAL RATIO
for sk in `seq ${SKlow} ${dSK} ${SKhi}`
do
###########    LOOP SHUFFLING RESTARTS  ##
for sh in `seq ${SHlow} ${dSH} ${SHhi}`
do
echo '--------------------------------------------------------------------------'
echo 'Running GPR for number pseudo inputs, signal2kernel, nShuffle = '${ns} ${sk} ${sh}
########    MAIN FUNCTION      ########3
trainGPR ${mpidir} ${repdir} ${ns} ${l} ${dim} 1e${sk} ${ntrain} ${sh} ${nproc} 

# Copy the gpr state to the run directory
cp ${l}_gpr.dat ${noahtraindir}
# Go into run directory and run the model
cd ${noahtraindir}
# Run noah executable with options for dynamic state update
#            One step, GPR correction, Dry correction only, Wet correction only, Equilibration
./noah_mp.exe 0 1 2 0    
# Calculate the root mean squared error
ms=`python rmse.py output.dry`
# write the root mean squared error to file
echo ${ms} ${ns} ${sh} 1e${sk} >> $rmsefilename
# Go bach to the training directory
cd ${mpidir}
# Set n and ns for lowest rmse value
# Bash doesn't do floating point numbers
if [ $(bc <<< "${ms} < ${lowrmse}") -eq 1 ]
then
  echo 'Updating the low values' ${ms} ${ns} ${sh} ${sk}
  lowrmse=${ms}
  lowns=${ns}
  lowsh=${sh}
  lowsk=${sk}
fi

# End shuffle restart loop
done
# End loop signal to kernal ratio
done
# End ns loop (number of pseudo inputs)
done

#####################
# RUN WITH BEST SET #
#####################
trainGPR ${mpidir} ${repdir} ${lowns} ${l} ${dim} 1e${lowsk} ${ntrain} ${lowsh} ${nproc} 

cp ${l}_gpr.dat ${noahtraindir}
cd ${noahtraindir}
./noah_mp.exe 0 1 2 0
msdryonly=`python rmse.py output.dry`
echo 'This should be the best set:' >> $rmsefilename
echo ${msdryonly} ${lowns} ${lowsh} 1e${lowsk} >> $rmsefilename

############################################
######## Use ouput of dry only run as observation
######## then do a onestep.
./noah_mp.exe 2 0 0 10
####################   Make the gpr input data
cd ${projdir}
./make_gpr_training_data.py $siteyear $mpidir $noahtraindir

cd ${mpidir}
###########################################################################################################
#####     NOW DO THE WET TRAINING     #####     NOW DO THE WET TRAINING     #####     WET TRAINING     ####
###########################################################################################################
# Wet/Dry specific stuff for training
dim=$dwet
l="wet"
ntrain=$(cat "num_"${l}"_training_points.txt")
nproc=4
# Values for finding the best gpr.
lowrmse=999
# Reset the rmse file.
rmsefilename=${mpidir}'rmse_'${l}'.txt'
if [ -e $rmsefilename ]
then
  rm $rmsefilename
fi
echo 'rmse pseudo-inputs nShuffles sig2kernel' >> $rmsefilename
# reset the whole repdir
rm ${repdir}*

###########    LOOP No PSEUDO-INPUTS
for ns in `seq ${PSlow} ${dPS} ${PShi}`
do
###########    LOOP SIGNAL TO KERNAL RATIO
for sk in `seq ${SKlow} ${dSK} ${SKhi}`
do
###########    LOOP SHUFFLING RESTARTS  ##
for sh in `seq ${SHlow} ${dSH} ${SHhi}`
do
echo '--------------------------------------------------------------------------'
echo 'Running GPR for number pseudo inputs, signal2kernel, nShuffle = '${ns} ${sk} ${sh}

########    MAIN FUNCTION      ########3
trainGPR ${mpidir} ${repdir} ${ns} ${l} ${dim} 1e${sk} ${ntrain} ${sh} ${nproc} 

# Copy the gpr state to the run directory
cp ${l}_gpr.dat ${noahtraindir}
# Go into run directory and run the model
cd ${noahtraindir}
# Run noah executable with options for dynamic state update
#            One step, GPR correction, Dry correction only, Wet correction only, Equilibration
./noah_mp.exe 0 1 1 0    
# Calculate the root mean squared error
ms=`python rmse.py output.gpr`
# write the root mean squared error to file
echo ${ms} ${ns} ${sh} 1e${sk} >> $rmsefilename
# Go bach to the training directory
cd ${mpidir}
# Set n and ns for lowest rmse value
# Bash doesn't do floating point numbers
if [ $(bc <<< "${ms} < ${lowrmse}") -eq 1 ]
then
  echo 'Updating the low values' ${ms} ${ns} ${sk} ${sh}
  lowrmse=${ms}
  lowns=${ns}
  lowsh=${sh}
  lowsk=${sk}
fi
# End number of shuffling restarts
done
# End loop signal to kernal ratio
done
# End ns loop (number of pseudo inputs)
done

#####################
# RUN WITH BEST SET #
#####################
########    MAIN FUNCTION      ########3
trainGPR ${mpidir} ${repdir} ${lowns} ${l} ${dim} 1e${lowsk} ${ntrain} ${lowsh} ${nproc}

cp ${l}_gpr.dat ${noahtraindir}
cd ${noahtraindir}
./noah_mp.exe 0 1 1 0 
msdry=`python rmse.py output.gpr`
echo 'This should be the best set:' >> $rmsefilename
echo ${msdry} ${lowns} ${lowsh} 1e${lowsk} >> $rmsefilename

cd ${mpidir}

#clean up unneeded files
/bin/rm *_pseudo *_start train
# Finally, copy over the gpr files and test the out of sample year.
cp ${mpidir}wet_gpr.dat ${noahtestdir}wet_gpr.dat
cp ${mpidir}dry_gpr.dat ${noahtestdir}dry_gpr.dat
cd ${noahtestdir}
./noah_mp.exe 0 0 0 11
./noah_mp.exe 0 1 0 0
