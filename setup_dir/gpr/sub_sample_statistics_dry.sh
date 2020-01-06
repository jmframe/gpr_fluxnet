#!/bin/bash
projdir="/discover/nobackup/jframe/gpr_fluxnet/"
repdir=${projdir}"soil_moisture/gpr-mpi/build-1/reports/"
subdir=${projdir}"soil_moisture/gpr-noah/build-1/examples/"
ntrain=$(cat "num_dry_training_points.txt")
mpirun -n 1 ${subdir}sub_sample_statistics <<EOF 2>&1 | tee -a ${repdir}output.log
initial_input_filename   inputs_dry.bin
initial_target_filename  targets_dry.bin
final_input_filename     sub_inputs_dry.bin
final_target_filename    sub_targets_dry.bin
initial_num_samples      ${ntrain}
final_num_samples        ${1}
num_parameters           9
EOF
