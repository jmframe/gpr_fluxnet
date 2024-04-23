#!/discover/nobackup/jframe/anaconda3/bin/python
# This is a script to generate slurm job scripts for the calibration runs. 

import numpy as np
import os

# Delete existing job scripts
if os.path.exists('job_scripts/cal_scripts/'):
    cmd = '/bin/rm -r job_scripts/cal_scripts/'
    os.system(cmd)
cmd = 'mkdir job_scripts/cal_scripts/'
os.system(cmd)
if os.path.exists('/bin/rm soil_moisture_submit_cal_jobs.sh'):
    cmd = '/bin/rm soil_moisture_submit_cal_jobs.sh'
    os.system(cmd)

# This is the experiment type. Previously there was a case/switch choosing
# from this value, but for now I am just doing soil moisture.
exp_type = 'soil_moisture'
 
# input variables
Nprocs  = 1
 
# load site/year combinations
sites = np.genfromtxt('data/pals/Site_Years.txt', delimiter = ' ')
Ns = sites.shape[0]

# open submit script file for writing
fname = exp_type + '_submit_cal_jobs.sh'

# Write the bash script header
with open(fname, 'w') as F:
    F.write('#!/bin/bash -f\n')
 
    # Loop through all the run sites and years, and add them thp the script
    for s in range(0,Ns):

        # job script file name
        jname = './job_scripts/cal_scripts/' + exp_type + '_cal_' + str(int(sites[s,0])) + '_' + str(int(sites[s,1])) + '.slurm'
 
        # remove old copy of this job script
        #cmd = '/bin/rm -f ' + jname
        #os.system(cmd)

        # write to job script
        cmd = 'cp job_scripts/master_cal.slurm ' + jname
        os.system(cmd)

        cmd = 'sed -i "s/exp_type/' + exp_type + '/g" ' + jname
        os.system(cmd)

        cmd = 'sed -i "s/site/' + str(int(sites[s,0])) + '/g" ' + jname
        os.system(cmd)

        cmd = 'sed -i "s/year/' + str(int(sites[s,1])) + '/g" ' + jname
        os.system(cmd)

        cmd = 'sed -i "s/eletter/sm/g" ' + jname
        os.system(cmd);

        cmd = 'sed -i "s/nprocs/' + str(Nprocs) + '/g" ' + jname
        os.system(cmd)

        # write to submit script 
        F.write('sbatch ' + jname + '\n')

## --- End Script ---------------------------------------------------
