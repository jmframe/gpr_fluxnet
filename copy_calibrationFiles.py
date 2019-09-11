#!/gpfsm/dulocal/sles11/other/SLES11.3/miniconda3/2019.03_py3.7/2019-05-15/bin/python
# This is a script to generate directories for 
# Equilibrating the calibrated noah-mp runs.
# Burn-in period is 10+1 times the number of years.
# we run the burn-in for the whole record, and the updated
# state values come from the last day of the previous year,
# if there is one.

import pandas as pd
import numpy as np
import shutil, errno
import os 
from io import StringIO

# This is the experiment type. Previously there was a case/switch choosing
# from this value, but for now I am just doing soil moisture. Will worry
# about other types of experiments when the time comes.
exp_type = 'soil_moisture'

# Copy these files
files2copy = ['forcing.txt', 'obs.txt', 'cal_parms.txt', 'num_times.txt', 'init.txt']

# load site/year combination
sites =	np.genfromtxt('data/pals/Site_Years.txt', delimiter = ' ')
Ns = sites.shape[0]
with np.printoptions(precision=2, suppress=True):
    print('sites')
    print(sites)
    print('Ns')
    print(Ns)
#-----  Set up runtime directories    ---------------------------
# directory for pasting over the calibration run files
wdir = exp_type + '/init_dirs' 
wdirC = wdir + '/calibrationRunFiles' 

# Delete and reinit test dirs
cmd = 'rm -r ' + wdirC
os.system(cmd)
cmd = 'mkdir ' + wdirC
os.system(cmd)

for sample in ['in', 'out']:
    for s in range(0, Ns):
        # get sites and year, need in string
        S = str(int(sites[s,0]))
        Y = str(int(sites[s,1]))
        siteyear = S + '_' + Y
        
        # Screen report
        print('Copying run files for site = %d, year = %d, ' % (sites[s,0],  sites[s,1]))
        print('siteyear: ', siteyear)

        # copy from the directory with the sample (in or out)
        wdirCopyFrom = wdir + '/run_' + siteyear + '_' + sample

        # past to this directory
        wdirPasteHere = wdirC + '/' + siteyear + '_' + sample
        cmd = 'mkdir ' + wdirPasteHere
        os.system(cmd)

        # copy files and put in new directory
        for file2copy in files2copy:
            cmd = 'cp ' + wdirCopyFrom + '/' + file2copy + ' ' + wdirPasteHere + '/' + file2copy
            os.system(cmd)

cmd = 'tar -czvf calibratedFiles.tar.gz ' + wdirC
os.system(cmd)
cmd = 'rm -r wdirC'
#os.system(cmd)
# --- End Script ---------------------------------------------------
