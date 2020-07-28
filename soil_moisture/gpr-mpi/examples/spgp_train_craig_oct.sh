#!/bin/bash -x 
dim=10
l="dry"
#year="2_2000-2004"
#year="2005"
#year="run_11_2002"
year="run_1_2006"
dir="/home/cpelissi/scratch/new_alg"
isDry=1
isWet=0

if [ -e msqe_${l}.txt ]
then
  rm msqe_${l}.txt
fi

#for n in `seq ${1} 50 ${2}`
#do
#np=`echo "${n}/400+1" | bc`
#./sub_sample_statistics_dry.sh ${n}  2>&1 | tee ${n}_sub.log
for nsh in `seq ${3} 1 ${4}`
do
for ns in `seq ${1} 1 ${2}`
do
tag=`printf %03d $ns`
mpirun -n 32 /discover/nobackup/cpelissi/calibrated_noah/blodgett/var_gpr/build/examples/train_gpr_sample <<EOF 2>&1 ${ns}_${l}_output.log
gpr_state_filename       ${l}_gpr.dat
number_of_inputs         48379
number_of_parameters     ${dim}
number_pseudo_inputs     ${ns}
input_filename           inputs_${l}.bin
target_filename          targets_${l}.bin
kernel_type              ARD_without_noise
approx_type              spgp
max_number_of_iterations 10040
stepsize                 0.001
line_search_tolerance    0.0001
gradient_tolerance       1e-4
signal_to_noise          1e3
signal_to_kernel         1e5
max_time                 5000
isRead                   0
nShuffles                ${nsh}
EOF

cp ${l}_gpr.dat ${dir}/${year}
cd ${dir}/${year}
./noah_mp.exe 0 1 ${isDry} ${isWet}
cat output.out | awk '{print $11}' > ${dir}/${tag}_${year}_test_${l}.txt
cd ${dir}
ms=`python msqe.py ${year}/tower ${tag}_${year}_test_${l}.txt`
echo ${ms} ${tag} ${n} ${j} >> msqe_${l}.txt
done
done
