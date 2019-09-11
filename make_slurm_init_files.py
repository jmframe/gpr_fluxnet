#!/gpfsm/dulocal/sles11/other/SLES11.3/miniconda3/2019.03_py3.7/2019-05-15/bin/python
# This is a script to generate slurm files for 
# Equilibrating the calibrated noah-mp runs.
# Burn-in period is 10+1 times the number of years.
# we run the burn-in for the whole record, and the updated
# state values come from the last day of the previous year,
# if there is one.

import numpy as np
import os

# This is the experiment type. Previously there was a case/switch choosing
# from this value, but for now I am just doing soil moisture. Will worry
# about other types of experiments when the time comes.
exp_type = 'soil_moisture'

# input variables
Nprocs  = 1

# load site/year combinations
sites = np.genfromtxt('data/pals/Site_Years.txt', delimiter = ' ')
Ns = sites.shape[0]
 
# --- Set Up Runtime Directories ------------------------------------
 
# open submit script file for writing
fname = exp_type + '_submit_init_jobs.sh'

# Write the bash script header
with open(fname, 'w') as F:
    F.write('#!/bin/bash\n')

    # Loop through all the run sites and years, and add them thp the script
    for s in range(0,Ns):
 
        # job script file name
        jname = './job_scripts/init_scripts/' + exp_type + '_init_' + str(int(sites[s,0])) + '_' + str(int(sites[s,1])) + '.slurm'
 
        # remove old copy of this job script
        cmd = '/bin/rm -f ' + jname
        os.system(cmd)
 
        # write to job script
        cmd = 'cp job_scripts/master_init.slurm ' + jname
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
        F.write('\n' + 'sbatch ' + jname)
 
 ## --- End Script ---------------------------------------------------
