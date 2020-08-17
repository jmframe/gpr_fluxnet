#!/bin/bash -x
#SBATCH --job-name="da_7_2005"
#SBATCH --account=s2027
#SBATCH --time=12:00:00
#SBATCH --constraint=hasw
#SBATCH --output="soil_moisture/da_dirs/reports/slurm_output_7_2005_%j.out"
#SBATCH --error="soil_moisture/da_dirs/reports/slurm_error_sitie_2005_%j.out"
#SBATCH --ntasks=1

# memory
ulimit -s unlimited

# modules
module purge
module load comp/intel/19.1.0.166
module load mpi/impi/20.0.0.166
 
# linking
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/discover/nobackup/projects/lis/libs/netcdf/4.3.3.1_intel-14.0.3.174_sp3/lib

cd soil_moisture/da_dirs/7_2005/

# This program is part of the initialization process. 
# To initialize we need to 
# 1. Run the calibrated parameters
# 2. Equilibrate (with the calibrated parameters)
#    i.e., burning in the initial states on a noah-mp run with calibrated parameters
# 3. Run the one step simulation
# 4. Run the perturbation simulations for Data Assimilation
# 5. Run the full data Assimilation

echo 0 > test_day_start.txt 
./make_obs_da_30day.py

echo 0 > da_flag.txt
echo 'running noah-mp basins for 7-2005'
./noah_mp.exe

for i in `seq 1 365`
do
    make_obs_da_30day.py
    
    # change the DA flag to run a state purturbationsimulation.
    echo -20 > da_flag.txt
    echo 0 > init_flag.txt # no need to equilibrate during data assimilation
    echo 'running the state purturbation simulation'
    ./noah_mp.exe
    
    # change the DA flag to run a state purturbationsimulation.
    echo 20 > da_flag.txt
    echo 'running the full data assimilation simulation'
    ./noah_mp.exe
    
    # save the output.
    ./save_da_30day_output.py
    
    # Move the forecast day you one
    #echo $i > test_day_start.txt 

done

# change the DA flag back to zero
echo 0 > da_flag.txt

#END
