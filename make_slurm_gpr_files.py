#!/discover/nobackup/jframe/anaconda3/bin/python
# This is a script to generate slurm files for training the GPR for soil moisture. 

import numpy as np
import os

# This is the experiment type. Previously there was a case/switch choosing
# from this value, but for now I am just doing soil moisture. Will worry
# about other types of experiments when the time comes.
exp_type = 'soil_moisture'

# load site/year combinations
sites = np.genfromtxt('data/pals/Site_Years.txt', delimiter = ' ')
Ns = sites.shape[0]
 
# --- Set Up Runtime Directories ------------------------------------
 
# open submit script file for writing
fname = exp_type + '_submit_gpr_jobs.sh'

# Write the bash script header
with open(fname, 'w') as F:
    F.write('#!/bin/bash\n')

    # Loop through all the run sites and years, and add them thp the script
    for s in range(0,Ns):
 
        # job script file name
        jname = './job_scripts/gpr_scripts/' + 'trainGPR_' + str(int(sites[s,0])) + '_' + str(int(sites[s,1])) + '.slurm'
 
        # remove old copy of this job script
        cmd = '/bin/rm -f ' + jname
        os.system(cmd)
 
        # write to job script
        cmd = 'cp job_scripts/master_gpr.slurm ' + jname
        os.system(cmd)
 
        cmd = 'sed -i "s/siite/' + str(int(sites[s,0])) + '/g" ' + jname
        os.system(cmd)
 
        cmd = 'sed -i "s/yeear/' + str(int(sites[s,1])) + '/g" ' + jname
        os.system(cmd)

        # write to submit script 
        F.write('\n' + 'sbatch ' + jname)
 
 ## --- End Script ---------------------------------------------------
