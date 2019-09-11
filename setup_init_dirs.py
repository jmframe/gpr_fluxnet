#!/gpfsm/dulocal/sles11/other/SLES11.3/miniconda3/2019.03_py3.7/2019-05-15/bin/python
# This is a script to generate directories for 
# Equilibrating the calibrated noah-mp runs.
# Burn-in period is 10+1 times the number of years.
# we run the burn-in for the whole record, and the updated
# state values come from the last day of the previous year,
# if there is one.

import numpy as np
import shutil, errno
import os
import sys
from io import StringIO

# Since this program will override the initialization files, 
# be careful running it. Check to make sure you meant to run.
continue_setup = str(input('Are you sure you want to run this setup? \
It may over override existing initialization files!\n'))
if not continue_setup in ['yes', 'Yes', 'Y', 'y', 'continue', 'Continue', 'affirmative']:
    print('Exiting program based on your answer. \n')
    sys.exit()
# Since this program will override the initialization files, 
# be careful running it. Check to make sure you meant to run.
continue_setup = str(input('Are you sure that you have generated the correct \
Forcing and Observation data files?\n \
You should have ran makeFiles.py in /data/pals/site_data\n'))
if not continue_setup in ['yes', 'Yes', 'Y', 'y', 'continue', 'Continue', 'affirmative']:
    print('Exiting program based on your answer. \n')
    sys.exit()

# --- Experiment Setup ---------------------------------------------
print('setting up initialization directories')
# This is the experiment type. Previously there was a case/switch choosing
# from this value, but for now I am just doing soil moisture. Will worry
# about other types of experiments when the time comes.
exp_type = 'soil_moisture'

# load site/year combination
sites = np.array([1, 2, 4, 5, 7, 11, 12, 13, 14, 16, 17, 18])
Ns = sites.shape[0]

#-----  Set up runtime directories    ---------------------------
# Delete and reinit test dirs
cmd = 'rm -r ' + exp_type + '/init_dirs'
os.system(cmd)
cmd = 'mkdir ' + exp_type + '/init_dirs'
os.system(cmd)

for s in range(0, Ns):

    # get sites and year, need in string
    S = str(int(sites[s]))
    
    # Screen report
    print('Setting up run directory for site = ' + S)

    # working directory
    wdir = exp_type + '/init_dirs/run_'+ S

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

    # executables
    cmd = 'ln -s ../../../model_code/noah_mp.exe ' + wdir + '/noah_mp.exe'
    os.system(cmd)

    # copy the calibrated parameter NAMES
    cmd = 'cp setup_dir/cal_parms.tpl ' + wdir + '/cal_parms.tpl'
    os.system(cmd);

    # generic noah-mp input files
    cmd = 'cp ./setup_dir/da_flag_cal.txt ' + wdir + '/da_flag.txt'
    os.system(cmd)
    cmd = 'cp ./setup_dir/init.txt ' + wdir
    os.system(cmd)
    cmd = 'cp ./setup_dir/tbot.txt ' + wdir
    os.system(cmd)

    # Large data files. Forcing and observation.
    cmd = 'cp soil_moisture/cal_dirs/run_'+S+'/forcing.txt ' + wdir + '/forcing.txt'
    os.system(cmd)
    cmd = 'cp soil_moisture/cal_dirs/run_'+S+'/obs.txt ' + wdir + '/obs.orig'
    os.system(cmd)

    # site-specific noah-mp input files
    cmd = 'cp data/parms/extract_parms/site_data/parms_' + S + '.txt ' + wdir + '/parms.txt'
    os.system(cmd)
    cmd = 'cp data/parms/extract_parms/site_data/time_parms_' + S + '.txt ' + wdir + '/time_parms.txt'
    os.system(cmd) 

    # get calibrated parameter VALUES from sce.out
    cal_dir = exp_type + '/cal_dirs/run_' + S + '/sce.out'
    f_read = open(cal_dir, "r")
    cal_params = f_read.readlines()[-1]
    f_read.close()
    cal_params = np.genfromtxt(StringIO(cal_params), dtype=None)
    fname = wdir + '/cal_parms.txt'
    with np.printoptions(precision=7, suppress=True):
        with open(fname, 'w') as F:
            for v in range(1, cal_params.shape[0]):
                F.write(str(cal_params[v]))
                F.write('\n')

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
    cmd = 'cp initialize/site_data/soil_init_' + S + '.txt ' + wdir + '/soil_init.orig'
    os.system(cmd)
    cmd = 'cp initialize/site_data/plant_init_' + S + '.txt ' + wdir + '/plant_init.orig'
    os.system(cmd)
    # Python scripts to run the initialization
    cmd = 'cp setup_dir/initialize.sh ' + wdir + '/initialize.sh' 
    os.system(cmd)
    cmd = 'cp setup_dir/save_obs_output_sm.py ' + wdir + '/save_obs_output_sm.py' 
    os.system(cmd)
    cmd = 'cp setup_dir/plot_init_sm.py ' + wdir + '/plot_init_sm.py' 
    os.system(cmd)
    cmd = 'cp setup_dir/plot_SM_assimilated.py ' + wdir + '/plot_SM_assimilated.py' 
    os.system(cmd)
    cmd = 'cp setup_dir/match_sm_CFD.py ' + wdir + '/match_sm_CFD.py' 
    os.system(cmd)
    cmd = 'cp setup_dir/rematch_sm_CFD.py ' + wdir + '/rematch_sm_CFD.py' 
    os.system(cmd)
    # Copy Data Assimilation Files
    cmd = 'cp setup_dir/sig_sm.txt ' + wdir + '/sig_sm.txt' 
    os.system(cmd)
    cmd = 'cp setup_dir/Nlag.txt ' + wdir + '/Nlag.txt' 
    os.system(cmd)
    cmd = 'cp setup_dir/obs_cov.txt ' + wdir + '/obs_cov.txt' 
    os.system(cmd)


# --- End Script ---------------------------------------------------
