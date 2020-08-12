#!/discover/nobackup/jframe/anaconda3/bin/python
# This is a script to generate directories for Data Assimilation 

import numpy as np
import shutil, errno
import os
import sys
from io import StringIO

# Since this program will override the initialization files, 
# be careful running it. Check to make sure you meant to run.
print('this setup will override existing initialization files!')
continue_setup = str(input('Are you sure?'))
if not continue_setup in ['yes', 'Yes', 'Y', 'y', 'continue', 'Continue', 'affirmative']:
    print('Exiting program based on your answer.')
    sys.exit()
# Since this program will override the initialization files, 
# be careful running it. Check to make sure you meant to run.
print('You have generated the correct Forcing and Observation data files?') 
print('You should have ran makeFiles.py in /data/pals/site_data')
continue_setup = str(input('Are you sure?')) 
if not continue_setup in ['yes', 'Yes', 'Y', 'y', 'continue', 'Continue', 'affirmative']:
    print('Exiting program based on your answer.')
    sys.exit()

# --- Experiment Setup ---------------------------------------------
print('setting up initialization directories')
# This is the experiment type. Previously there was a case/switch choosing
# from this value, but for now I am just doing soil moisture. Will worry
# about other types of experiments when the time comes.
exp_type = 'soil_moisture'

# load site/year combination
sites =	np.genfromtxt('data/pals/Site_Years.txt', delimiter = ' ')
Ns = sites.shape[0]
with np.printoptions(precision=2, suppress=True):
    print('sites')
    print(sites)
    print('Ns')
    print(Ns)

#-----  Set up runtime directories    ---------------------------
# Delete and reinit test dirs
cmd = 'rm -r ' + exp_type + '/da_dirs'
os.system(cmd)
cmd = 'mkdir ' + exp_type + '/da_dirs'
os.system(cmd)

for s in range(0, Ns):

    # get sites and year, need in string
    S = str(int(sites[s,0]))
    Y = str(int(sites[s,1]))

    siteyear = S + '_' + Y
    
    # Screen report
    print('Setting up run directory for site = %d, year = %d, ' % (sites[s,0],  sites[s,1]))
    print('siteyear: ', siteyear)

    # working directory
    wdir = exp_type + '/da_dirs/' + siteyear

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
    with np.printoptions(precision=7, suppress=True):
        with open(fname, 'w') as F:
            F.write(Y)

    # executables
    cmd = 'ln -s ../../../model_code/noah_mp.exe ' + wdir + '/noah_mp.exe'
    os.system(cmd)

    # copy the calibrated parameter NAMES
    cmd = 'cp setup_dir/cal_parms.tpl ' + wdir + '/cal_parms.tpl'
    os.system(cmd);

    # generic noah-mp input files
    cmd = 'cp ./setup_dir/da_flag_DA.txt ' + wdir + '/da_flag.txt'
    os.system(cmd)
    cmd = 'cp ./setup_dir/init.txt ' + wdir
    os.system(cmd)
    cmd = 'cp ./setup_dir/tbot.txt ' + wdir
    os.system(cmd)

    # Large data files. Forcing and observation.
    cmd = 'ln -s ../../../data/pals/site_data/forcing_' + S + '.txt ' + wdir + '/forcing.txt'
    os.system(cmd)
    #example: /discover/nobackup/jframe/gpr_fluxnet/data/pals/site_data/obs_1_2003_train.txt
    cmd = 'ln -s ../../../data/pals/site_data/obs_' + S + '.txt ' + wdir + '/obs_full.txt'
    os.system(cmd)

    # site-specific noah-mp input files
    cmd = 'cp data/parms/extract_parms/site_data/parms_' + S + '.txt ' + wdir + '/parms.txt'
    os.system(cmd)
    cmd = 'cp data/parms/extract_parms/site_data/time_parms_' + S + '.txt ' + wdir + '/time_parms.txt'
    os.system(cmd) 

    # get calibrated parameter VALUES from proc0
    cmd = 'cp soil_moisture/cal_dirs/run_'+S+'_'+Y+'/proc0/cal_parms.txt soil_moisture/da_dirs/'+siteyear+'/cal_parms.txt'
    os.system(cmd)

    # get site information
    forcing = np.genfromtxt(wdir + '/forcing.txt')
    pals_sites = np.genfromtxt('data/pals/Sites.txt')
    lat = pals_sites[int(sites[s,0])-1,0]
    lon = pals_sites[int(sites[s,0])-1,1]
    offset = pals_sites[int(sites[s,0])-1,2]

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

    # number of iterations for equilibration
    fname = wdir + '/init_flag.txt'
    with np.printoptions(precision=0, suppress=True):
        with open(fname, 'w') as F:
            F.write(str(5))

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
    cmd = 'cp initialize/site_data/soil_init_' + siteyear + '.txt ' + wdir + '/soil_init.txt'
    os.system(cmd)
    cmd = 'cp initialize/site_data/plant_init_' + siteyear + '.txt ' + wdir + '/plant_init.txt'
    os.system(cmd)
    # Python scripts to run the initialization
#    cmd = 'cp setup_dir/initialize.slurm ' + wdir + '/initialize.slurm' 
#    os.system(cmd)
#    cmd = 'cp setup_dir/save_obs_output_sm.py ' + wdir + '/save_obs_output_sm.py' 
#    os.system(cmd)
    cmd = 'cp setup_dir/plot_init_sm.py ' + wdir + '/plot_init_sm.py' 
    os.system(cmd)
    cmd = 'cp setup_dir/make_obs_da_30day.py ' + wdir + '/make_obs_da_30day.py' 
    os.system(cmd)
    cmd = 'cp setup_dir/test_day_start.txt ' + wdir + '/test_day_start.txt' 
    os.system(cmd)
    cmd = 'cp setup_dir/save_da_30day_output.py ' + wdir + '/save_da_30day_output.py' 
    os.system(cmd)
#    cmd = 'cp setup_dir/plot_SM_assimilated.py ' + wdir + '/plot_SM_assimilated.py' 
#    os.system(cmd)
#    cmd = 'cp setup_dir/match_sm_CFD.py ' + wdir + '/match_sm_CFD.py' 
#    os.system(cmd)
#    cmd = 'cp setup_dir/rematch_sm_CFD.py ' + wdir + '/rematch_sm_CFD.py' 
#    os.system(cmd)
    # Copy Data Assimilation Files
    cmd = 'cp setup_dir/sig_sm.txt ' + wdir + '/sig_sm.txt' 
    os.system(cmd)
    cmd = 'cp setup_dir/Nlag.txt ' + wdir + '/Nlag.txt' 
    os.system(cmd)
    cmd = 'cp setup_dir/obs_cov.txt ' + wdir + '/obs_cov.txt' 
    os.system(cmd)


# --- End Script ---------------------------------------------------
