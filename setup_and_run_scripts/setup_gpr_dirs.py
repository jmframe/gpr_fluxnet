#!/discover/nobackup/jframe/anaconda3/bin/python
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
print('this setup will override existing gpr files!')
continue_setup = str(input('Are you sure?'))
if not continue_setup in ['yes', 'Yes', 'Y', 'y', 'continue', 'Continue', 'affirmative']:
    print('Exiting program based on your answer.')
    sys.exit()
# Since this program will override the initialization files, 
# be careful running it. Check to make sure you meant to run.
print('You have done the calibration before this.') 
continue_setup = str(input('Are you sure?')) 
if not continue_setup in ['yes', 'Yes', 'Y', 'y', 'continue', 'Continue', 'affirmative']:
    print('Exiting program based on your answer.')
    sys.exit()

# --- Experiment Setup ---------------------------------------------
print('setting up GPR training directories')
# This is the experiment type. Previously there was a case/switch choosing
# from this value, but for now I am just doing soil moisture. Will worry
# about other types of experiments when the time comes.
exp_type = 'soil_moisture'

proj_dir = '/discover/nobackup/jframe/gpr_fluxnet/'
main_dir = proj_dir + 'soil_moisture/'
setup_dir = proj_dir + 'setup_dir/'
setupgpr_dir = setup_dir + 'gpr/'
data_dir = proj_dir + 'data/pals/site_data/'
    
# load site/year combination
siteyears = np.genfromtxt(proj_dir+'data/pals/Site_Years.txt', delimiter = ' ')
sitelatlonoff = np.genfromtxt(proj_dir+'data/pals/Sites.txt', delimiter = ' ')
Ns = siteyears.shape[0]
with np.printoptions(precision=2, suppress=True):
    print('site years')
    print(siteyears)
    print('Ns')
    print(Ns)

#-----  Set up runtime directories    ---------------------------

for s in range(0, Ns):

    # get sites and year, need in string
    S_int = int(siteyears[s,0])
    S = str(S_int)
    Y_int = int(siteyears[s,1])
    Y = str(Y_int)

    siteyear = S + '_' + Y
    
    # Screen report
    print('Setting up gpr directories for site = %d, year = %d, ' % (siteyears[s,0],  siteyears[s,1]))
    print('siteyear: ', siteyear)

    cal_dir = main_dir + 'cal_dirs/' + 'run_' + str(siteyear) + '/'
    gpr_noah_dir = main_dir + 'gpr-noah/train_' + str(siteyear) + '/'
    gpr_test_dir = main_dir + 'gpr-noah/test_' + str(siteyear) + '/'
    gpr_mpi_dir = main_dir + 'gpr-mpi/' + str(siteyear) + '/'

    # get site information
    lat = sitelatlonoff[S_int - 1,0]
    lon = sitelatlonoff[S_int - 1,1]
    offset = sitelatlonoff[S_int - 1,2]
    
    # Delete the old directories for the site/year
    cmd = '/bin/rm -rf '+gpr_mpi_dir
    os.system(cmd)
    cmd = '/bin/rm -rf '+gpr_noah_dir
    os.system(cmd)
    cmd = '/bin/rm -rf '+gpr_test_dir
    os.system(cmd)
    # Copy the directories for the site/year
    cmd = 'mkdir '+gpr_mpi_dir
    os.system(cmd)
    cmd = 'mkdir '+gpr_mpi_dir+'reports/'
    os.system(cmd)
    cmd = 'mkdir '+gpr_noah_dir
    os.system(cmd)
    cmd='cd '+gpr_noah_dir
    os.system(cmd)
    cmd = 'ln -s ../build/examples/noah_mp.exe '+gpr_noah_dir+'noah_mp.exe'
    os.system(cmd)
    cmd = 'mkdir '+gpr_test_dir
    os.system(cmd)
    cmd='cd '+gpr_test_dir
    os.system(cmd)
    cmd = 'ln -s ../build/examples/noah_mp.exe '+gpr_test_dir+'noah_mp.exe'
    os.system(cmd)
        
    # FORCING AND OBSERVATION FILES FOR GPR Train
    cmd = 'ln -s '+data_dir+'forcing_'+S+'.txt '+gpr_noah_dir+'forcing.txt'
    os.system(cmd)
    cmd = 'ln -s '+data_dir+'obs_'+siteyear+'_train.txt '+gpr_noah_dir+'obs.txt'
    os.system(cmd)
    cmd = 'ln -s '+data_dir+'obs_'+S+'.txt '+gpr_noah_dir+'obs_full.txt'
    os.system(cmd)
    cmd = 'ln -s '+data_dir+'obs_'+S+'.txt '+gpr_test_dir+'obs_full.txt'
    os.system(cmd)

    # FORCING AND OBSERVATION FILES FOR GPR TEST
    #cmd = 'cp '+data_dir+'forcing_'+str(siteyear)+'_test.txt'+' '+gpr_test_dir+'forcing.txt'
    cmd = 'ln -s '+data_dir+'forcing_'+siteyear+'_test.txt '+gpr_test_dir+'forcing.txt'
    os.system(cmd)
    cmd = 'ln -s '+data_dir+'obs_'+siteyear+'_test.txt '+gpr_test_dir+'obs.txt'
    #cmd = 'cp '+data_dir+'obs_'+str(siteyear)+'_test.txt'+' '+gpr_test_dir+'obs.txt'
    os.system(cmd)

    # Make a file with the site and year, 
    # since this information isn't in any of the files.
    for idir in [gpr_mpi_dir, gpr_noah_dir, gpr_test_dir]:
        fname = idir + '/site.txt'
        with np.printoptions(precision=7, suppress=True):
            with open(fname, 'w') as F:
                F.write(S)
        fname = idir + '/year.txt'
        with np.printoptions(precision=7, suppress=True):
            with open(fname, 'w') as F:
                F.write(Y)
        # Copy in some example .dat files
        cmd = 'cp '+main_dir+'gpr-mpi/examples/wet_gpr.dat '+idir+'wet_gpr.dat'
        os.system(cmd)
        cmd = 'cp '+main_dir+'gpr-mpi/examples/dry_gpr.dat '+idir+'dry_gpr.dat'
        os.system(cmd)



    # INITIALIZATION FILES
    files2copy = ['parms', 'time_parms']
    for f in files2copy:
        cmd = 'cp data/parms/extract_parms/site_data/' + f + '_' + S +'.txt ' + gpr_noah_dir + f + '.txt'
        os.system(cmd)
    for f in files2copy:
        cmd = 'cp data/parms/extract_parms/site_data/' + f + '_' + S +'.txt ' + gpr_test_dir + f + '.txt'
        os.system(cmd)


    for idir in [gpr_noah_dir, gpr_test_dir]:

        # site-specific noah-mp input files - initial states
        cmd = 'cp initialize/site_data/soil_init_' + S + '.txt ' + idir + 'soil_init.txt'
        os.system(cmd)
        cmd = 'cp initialize/site_data/plant_init_' + S + '.txt ' + idir + 'plant_init.txt'
        os.system(cmd)

        # site-specific noah-mp input files - lat/lon
        fname = idir + 'lat_lon.txt'
        with np.printoptions(precision=7, suppress=True):
            with open(fname, 'w') as F:
                F.write('%f\n%f' % (lat, lon)) 

        # site-specific noah-mp input files - startdate
        if offset < 0:
            startdate = 200012311200 + (12+offset)*100  # offset here is negative to the west
        else:
            startdate = 200101011200 - (12-offset)*100
        startdate = str(int(startdate));
        fname = idir + 'startdate.txt'
        with open(fname,'w') as F:
            F.write(startdate)
 
        # get calibrated parameter VALUES from proc0
        cmd = 'cp '+cal_dir+'/proc0/cal_parms.txt '+ idir +'cal_parms.txt'
        os.system(cmd)

        # Set the number of time steps for the testing data.
        with open(idir+'forcing.txt') as f:
            for nt, l in enumerate(f):
                pass
            with open(idir+'num_times.txt', 'w') as f:
                f.write(str(nt+1))

        # SETUP_DIR/GPR/ plotting, results and initialization file.
        files2copy = ['plot_gprNoah_output_print_results.py', \
                      'rmse.py', 'initialize_gpr_noah.sh', 'init_flag.txt']
        for f in files2copy:
            cmd = 'cp ' + setupgpr_dir + f + ' ' + idir + f
            os.system(cmd)
        files2copy = ['init.txt', 'tbot.txt']
        for f in files2copy:
            cmd = 'cp ' + setup_dir + f + ' ' + idir + f
            os.system(cmd)
# --- End Script ---------------------------------------------------
