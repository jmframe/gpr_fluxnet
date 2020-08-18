#!/bin/bash -x
#SBATCH --job-name="da_1_2006"
#SBATCH --account=s2027
#SBATCH --time=12:00:00
#SBATCH --constraint=hasw
#SBATCH --output="soil_moisture/gpr-noah/reports/slurm_output_1_2006_%j.out"
#SBATCH --error="soil_moisture/gpr-noah/reports/slurm_error_1_2006_%j.out"
#SBATCH --ntasks=1

# memory
ulimit -s unlimited

# modules
module purge
module load comp/gcc/8.3.0  cmake/3.17.0 
module load comp/intel/19.1.0.166 
export CXX=icpc
export GSL_HOME=/usr/local/other/gsl/2.6
export GXX_NAME=/usr/local/other/gcc/8.3.0/bin/gcc
 
# linking
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/discover/nobackup/projects/lis/libs/netcdf/4.3.3.1_intel-14.0.3.174_sp3/lib

cd soil_moisture/gpr-noah/test_1_2006/

# This program is for running the gpr
# We run for an out of sample year
# 1. Run the entire simulation, but leave out 30 days worth of observation data.
# 2. Equilibrate each time we start a new set of 30 day 'forecasts'
# 3. Save the results of the 30 day predictions after each run
# 4. Move the starting test date forward each times.

echo 0 > test_day_start.txt 
./make_obs_da_30day.py

echo 0 > da_flag.txt
echo 0 > init_flag.txt # no need to equilibrate when running multiple years ahead

echo 'running noah-mp basins for siite-year'
./noah_mp.exe 0 1 1 0

for i in `seq 1 335`
do
    echo $i
    ./noah_mp.exe
    # save the output.
    ./save_da_30day_output.py gpr
    # Move the forecast day you one
    echo $i > test_day_start.txt 
    # Make the new observation data file
    ./make_obs_da_30day.py
done

#END
