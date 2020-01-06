#!/bin/bash -x
#SBATCH --job-name="cal_14_2003"
#SBATCH --account=s2027
#SBATCH --time=12:00:00
#SBATCH --constraint=hasw
#SBATCH --output="soil_moisture/cal_dirs/reports/slurm_output_14_2003_%j.out"
#SBATCH --error="soil_moisture/cal_dirs/reports/slurm_error_14_2003_%j.out"
#SBATCH --ntasks=2

# memory
ulimit -s unlimited

# modules
module purge
module load comp/intel-16.0.3.210
module load mpi/impi-18.0.3.222
 
# linking
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/discover/nobackup/projects/lis/libs/netcdf/4.3.3.1_intel-14.0.3.174_sp3/lib

# location
cd soil_moisture/cal_dirs/run_14_2003

# start cleanup script
#sh periodic_cleanup.sh &

# start job
./ostrich.exe

# done
exit 0



