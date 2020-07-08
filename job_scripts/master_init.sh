#!/bin/bash -x

# memory
ulimit -s unlimited

# modules
module purge
module load comp/intel-16.0.3.210
module load mpi/impi-18.0.3.222
 
# linking
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/discover/nobackup/projects/lis/libs/netcdf/4.3.3.1_intel-14.0.3.174_sp3/lib

cd soil_moisture/init_dirs/run_siite_year/

# This program is part of the initialization process. 
# To initialize we need to 
# 1. Run the calibrated parameters
# 2. Equilibrate (with the calibrated parameters)
#    i.e., burning in the initial states on a noah-mp run with calibrated parameters
# 3. Run the one step simulation
# 4. Run the perturbation simulations for Data Assimilation
# 5. Run the full data Assimilation

echo 'Initializing siite-year'

./noah_mp.exe

# Copy output from noah only model
cp output.out output.noah

# change the DA flag to run a one-step state update simulation.
echo -1 > da_flag.txt
echo 'running the one-step state update simulation'
./noah_mp.exe

# Copy output from onestep model
cp output.out output.onestep

# change the DA flag to run a state purturbationsimulation.
echo -20 > da_flag.txt
echo 0 > init_flag.txt # no need to equilibrate during data assimilation
echo 'running the state purturbation simulation'
./noah_mp.exe

# Match the cumulative distributions
#./match_sm_CFD.py

# change the DA flag to run a state purturbationsimulation.
echo 20 > da_flag.txt
echo 'running the full data assimilation simulation'
./noah_mp.exe

# Copy output from assimilated model
cp output.out output.da

# Invert the data transformation, so we can plot the output
#./rematch_sm_CFD.py # Need to make this file.

# copy the observation data back
#mv obs.txt obs.mch
#cp obs.orig obs.txt

# Collect and create a time series with the date, observed soil moisture and modeled soil moisture
#./save_obs_output_sm.py

# change the DA flag back to zero
echo 0 > da_flag.txt

#END
