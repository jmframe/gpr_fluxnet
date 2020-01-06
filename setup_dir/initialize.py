#!/gpfsm/dulocal/sles11/other/SLES11.3/miniconda3/2019.03_py3.7/2019-05-15/bin/python

# This program is part of the initialization process. 
# To initialize we need to 
# 1. Run the calibrated parameters
# 2. Equilibrate (with the calibrated parameters)
#    i.e., burning in the initial states on a noah-mp run with calibrated parameters

# This script will go in each directory, instead of looping through them all.
# Simply because I am not sure how to call the noah_mp.exe from outside the folder

import os
import numpy as np

S = open('site.txt', 'r')
S = S.readline()
print('Initializing site: '+S)
# to check for convergence, introduce sm_old
sm_old = np.zeros(4)
diff=0

# Print the initial States
with open('soil_init.orig', 'r') as SM:
    sm = np.genfromtxt(SM)
print('initial initial soil moisture states')
print(sm)
with open('plant_init.orig', 'r') as PL:
    pl = np.genfromtxt(PL)
print('initial initial plant states')
print(pl)
# Copy the initial states
cmd = 'cp soil_init.orig soil_init.txt'
os.system(cmd)
cmd = 'cp plant_init.orig plant_init.txt'
os.system(cmd)
# copy over the observation data, because we'll transform it for DA
cmd = 'cp obs.orig obs.txt'
os.system(cmd)


sm1 = np.genfromtxt('soil_init.txt')
pl1 = np.genfromtxt('plant_init.txt')
# Loop through the number of times to do the burn-in, 
# We are doing 10+1 times for each year.
for irun in range(1, 11):
    print('for site ' + S + '\nThis is burn-in run ' + str(irun))
    os.system('./noah_mp.exe')

    # Collect initial state
    O = np.genfromtxt('output.out', delimiter = None)
    # Get the last line of the document
    endX = O.shape[0]-1
    sm = O[endX,10:14]
    pl = O[endX,14:18]
    with open('soil_init.txt', 'w') as SM:
        for ism in range(0, 4):
            SM.write(str(sm[ism]))
            SM.write('\n')
    with open('plant_init.txt', 'w') as PL:
        for ipl in range(0, 4):
            PL.write(str(pl[ipl]))
            PL.write('\n')
    for i in range(0,sm.shape[0]):
        diff = diff + np.abs(sm[i] - sm_old[i])
    if diff == 0:    
        print('the soil moisture states have converged')
        break
    diff = 0
    sm_old = sm

print('the soil moisture initial states changed from:')
print(sm1)
print('to:')
print(sm)

print('the plant initial states changed from:')
print(pl1)
print('to:')
print(pl)

# Copy output from noah only model
cmd = ('cp output.out output.noah')
os.system(cmd)

print('running the one-step state update simulation')
# change the DA flag to run a one-step state update simulation.
with open('da_flag.txt', 'w') as daflgFile:
    daflgFile.write(str(-1))
# Run the one-step state update simulation
os.system('./noah_mp.exe')
# Copy output from onestep model
cmd = 'cp output.out output.onestep'
os.system(cmd)

print('running the state purturbation simulation')
# change the DA flag to run a state purturbationsimulation.
with open('da_flag.txt', 'w') as daflgFile:
    daflgFile.write(str(-20))

# Run the one-step state update simulation
os.system('./noah_mp.exe')

# Match the cumulative distributions
import match_sm_CFD

# change the DA flag to run a state purturbationsimulation.
with open('da_flag.txt', 'w') as daflgFile:
    daflgFile.write(str(20))

print('running the full data assimilation simulation')
# Run the one-step state update simulation
os.system('./noah_mp.exe')
# Copy output from assimilated model
cmd = 'cp output.out output.da'
os.system(cmd)

# Invert the data transformation, so we can plot the output
import rematch_sm_CFD # Need to make this file.

# copy the observation data back
cmd = 'mv obs.txt obs.mch'
os.system(cmd)
cmd = 'cp obs.orig obs.txt'
os.system(cmd)

# Collect and create a time series with the date, observed soil moisture and modeled soil moisture
import save_obs_output_sm
#import plot_init_sm
#END
