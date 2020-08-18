#!/bin/bash -x
#SBATCH --job-name="da_21_2010"
#SBATCH --account=s2027
#SBATCH --time=12:00:00
#SBATCH --constraint=hasw
#SBATCH --output="soil_moisture/da_dirs/reports/slurm_output_21_2010_%j.out"
#SBATCH --error="soil_moisture/da_dirs/reports/slurm_error_sitie_2010_%j.out"
#SBATCH --ntasks=1

# memory
ulimit -s unlimited

# modules
module purge
module load comp/intel/19.1.0.166
module load mpi/impi/20.0.0.166
 
# linking
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/discover/nobackup/projects/lis/libs/netcdf/4.3.3.1_intel-14.0.3.174_sp3/lib

cd soil_moisture/da_dirs/21_2010/

# This program is for running the data assimilation. 
# We run for an out of sample 2010
# 1. Only run data assimilation for 30 days.
# 2. Equilibrate each time we start a new set of 30 day 'forecasts'
# 3. Run the purturbation, then the DA in a loop
# 4. Save the results of the 30 day predictions after each run
# 5. Move the starting test date forward each times.

echo 0 > test_day_start.txt 
./make_obs_da_30day.py

echo 0 > da_flag.txt
echo 0 > init_flag.txt # no need to equilibrate during data assimilation

echo 'running noah-mp basins for 21-2010'
./noah_mp.exe

for i in `seq 1 335`
do
    echo $i
    
    # change the DA flag to run a state purturbationsimulation.
    echo -11 > da_flag.txt
    echo 'running the state purturbation simulation'
    ./noah_mp.exe
    
    # change the DA flag to run a state purturbationsimulation.
    echo 11 > da_flag.txt
    echo 'running the full data assimilation simulation'
    ./noah_mp.exe
    
    # save the output.
    ./save_da_30day_output.py
    
    # Move the forecast day you one
    echo $i > test_day_start.txt 

    # Make the new observation data file
    ./make_obs_da_30day.py

done

# change the DA flag back to zero
echo 0 > da_flag.txt

#END
