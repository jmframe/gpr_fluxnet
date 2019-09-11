#!/gpfsm/dulocal/sles11/other/SLES11.3/miniconda3/2019.03_py3.7/2019-05-15/bin/python
# This is a script to generate directories for 
# Calibrating the BLODGETT station noah-mp runs.
# I am copying this from Grey's Matlab script.
# I want to calibrate the individual years, as
# well as the whole record. I might also want to 
# test different calibration settings.

import numpy as np
import shutil, errno
import os
from os.path import dirname as up
import sys
from io import StringIO

# Since this program will override the calibration files, 
# be careful running it. Check to make sure you meant to run.
continue_setup = str(input('Are you sure you want to run this setup? \
It may over override existing calibration files!\n'))
if not continue_setup in ['yes', 'Yes', 'Y', 'y', 'continue', 'Continue', 'affirmative']:
    print('Exiting program based on your answer. \n')
    sys.exit()

# Since this program will override the calibration files, 
# be careful running it. Check to make sure you meant to run.
continue_setup = str(input('Are you sure that you have generated the correct \
Forcing and Observation data files?\n \
You should have ran makeFiles.py in /data/pals/site_data\n'))
if not continue_setup in ['yes', 'Yes', 'Y', 'y', 'continue', 'Continue', 'affirmative']:
    print('Exiting program based on your answer. \n')
    sys.exit()

# --- Experiment Setup ---------------------------------------------
print('setting up calibration directories')
# This is the experiment type. Previously there was a case/switch choosing
# from this value, but for now I am just doing soil moisture. Will worry
# about other types of experiments when the time comes.
exp_type = 'soil_moisture'

# load site/year combinations
sites = np.array([1, 2, 4, 5, 7, 11, 12, 13, 14, 16, 17, 18])
Ns = 12
with np.printoptions(precision=2, suppress=True):
    print('sites')
    print(sites)
    print('Ns')
    print(Ns)

# --- Set Up Runtime Directories ------------------------------------
# delete and reinit test dirs
cmd = 'rm -rf ' + exp_type + '/cal_dirs'
os.system(cmd)
cmd = 'mkdir ' + exp_type + '/cal_dirs'
os.system(cmd)
cmd = 'mkdir ' + exp_type + '/cal_dirs/reports'
os.system(cmd)
cmd = 'cp setup_dir/cleanCalFiles.py ' + exp_type + '/cal _dirs/'

for s in range(0, Ns):

    # get sites and year, need in string
    S = str(int(sites[s]))

    # screen report
    print('Setting up run directory for site = ' + S )

    # working directory
    wdir = exp_type + '/cal_dirs/run_' + S

    # delete runtime directory for the site
    cmd = 'rm -rf ' + wdir
    os.system(cmd)
    cmd = 'mkdir ' + wdir
    os.system(cmd)
    cmd = 'mkdir ' + wdir + '/reports'
    os.system(cmd)

    # Make a file with the site and year, 
    # since this information isn't in any of the files.
    fname = wdir + '/site.txt'
    with np.printoptions(precision=7, suppress=True):
        with open(fname, 'w') as F:
            F.write(S)
    fname = wdir + '/year.txt'

    # executables
    cmd = 'ln -s ../../../ostrich/Source/Ostrich ' + wdir + '/ostrich.exe'  
    os.system(cmd)
    cmd = 'ln -s ../../../model_code/noah_mp.exe ' + wdir + '/noah_mp.exe'  
    os.system(cmd)
    cmd = 'ln -s ../../../setup_dir/periodic_cleanup.sh ' + wdir + '/periodic_cleanup.sh'  
    os.system(cmd)

    cmd = 'cp setup_dir/ostIn.txt ' + wdir + '/ostIn.txt' 
    os.system(cmd) 
    cmd = 'cp setup_dir/cal_parms.tpl ' + wdir
    os.system(cmd) 

    # generic noah-mp input files
    cmd = 'cp ./setup_dir/da_flag_cal.txt ' + wdir + '/da_flag.txt'
    os.system(cmd)
    cmd = 'cp ./setup_dir/init.txt ' + wdir
    os.system(cmd)
    cmd = 'cp ./setup_dir/tbot.txt ' + wdir
    os.system(cmd)

    # Large data files. Forcing and observation.
    cmd = 'mv data/pals/site_data/forcing_' + S + '_train.txt ' + wdir + '/forcing.txt'  
    os.system(cmd)
    cmd = 'mv data/pals/site_data/obs_' + S + '_train.txt ' + wdir + '/obs.txt'  
    os.system(cmd)

    # site-specific noah-mp input files
    cmd = 'cp data/parms/extract_parms/site_data/parms_' + S + '.txt ' + wdir + '/parms.txt'  
    os.system(cmd)
    cmd = 'cp data/parms/extract_parms/site_data/cal_parms_' + S + '.txt ' + wdir + '/cal_parms.txt'  
    os.system(cmd)
    cmd = 'cp data/parms/extract_parms/site_data/time_parms_' + S + '.txt ' + wdir + '/time_parms.txt'
    os.system(cmd)

    # get site information
    forcing = np.genfromtxt(wdir + '/forcing.txt')
    pals_sites = np.genfromtxt('data/pals/Sites.txt')
    lat = pals_sites[int(sites[s])-1,0]
    lon = pals_sites[int(sites[s])-1,1]
    offset = pals_sites[int(sites[s])-1,2]

    # site-specific noah-mp input files - number of timesteps
    Nt = forcing.shape[0]
    fname = wdir + '/num_times.txt'
    with np.printoptions(precision=0, suppress=True):
        with open(fname, 'w') as F:
            #################################################
            ######## Might wanst to calibrate to a shorter period of time
            ######## If so, just change this to the number of time steps.
            F.write(str(Nt))

    # site-specific noah-mp input files - lat/lon
    fname = wdir + '/lat_lon.txt'
    with np.printoptions(precision=7, suppress=True):
        with open(fname, 'w') as F:
            F.write('%f\n%f' % (lat, lon)) 

    # site-specific noah-mp input files - startdate
    if offset < 0:
        startdate = 200012311200 + (12+offset)*100  # offset here is negative to the west
    else:
        startdate = 200101011200 - (12-offset)*100

    startdate = str(int(startdate));
    fname = wdir + '/startdate.txt'
    with open(fname,'w') as F:
        F.write(startdate)

    # site-specific noah-mp input files - initial states
    cmd = 'cp initialize/site_data/soil_init_' + S + '.txt ' + wdir + '/soil_init.txt'
    os.system(cmd)
    cmd = 'cp initialize/site_data/plant_init_' + S + '.txt ' + wdir + '/plant_init.txt'
    os.system(cmd)
# --- End Script ---------------------------------------------------
