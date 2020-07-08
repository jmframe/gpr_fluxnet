#!/gpfsm/dulocal/sles11/other/SLES11.3/miniconda3/2019.03_py3.7/2019-05-15/bin/python
import numpy as np
import shutil, errno
import os
import sys
from io import StringIO

siteyear='1_2003'
main_dir='/discover/nobackup/jframe/gpr_fluxnet/soil_moisture/'
gpr_mpi_dir=main_dir+'gpr-mpi/'+siteyear+'/'
gpr_noah_dir=main_dir+'gpr-noah/train_'+siteyear+'/'

# Define the number of parameters that will be used for training the GPR
numGPRinputs_wet = 18
numGPRinputs_dry = 17

# Include a lag to stabalize the GPR results
# Until otherwise specified this lag is three timesteps.
lag = 3

# A list used for generating the GPR target
dif = []

# List to cut down the training data into just what we want to use.
wet_train_list = []
dry_train_list = []

# Grabbing data from two files, and writing to one. Open them at the same time.
# Note that soil moisture is column 5 (sixth column) of observations and 
# column 10 (11th column) of ouput.out
with open(gpr_noah_dir+'obs.txt', 'r') as Observations_file, \
     open(gpr_noah_dir+'forcing.txt', 'r') as Forcing_file, \
     open(gpr_noah_dir+'output.onestep', 'r') as Onestep_file:
    # Read the input data as delimited.
    forcing_data = np.genfromtxt(Forcing_file)
    observation_data = np.genfromtxt(Observations_file)
    onestep_data = np.genfromtxt(Onestep_file)

    # Get the overall number of records (not just useful observations)
    # And loop through them looking for good points to train.
    with open(gpr_noah_dir+'num_times.txt') as nt:
        total_records = int(nt.readline())

    # These are the indexes of usable observations 
    # Not only do they have good values, but so do their lags.
    iObs = []
    for i in range(lag, total_records):
      #  for il in range(0, lag+1):
      #      if observation_data[i-il][5] > 0:
      #          useData = True
      #      else:
      #          useData = False
      #  if useData:
        if all(q > 0 for q in observation_data[i-3:i+1, 5]):
            iObs.append(i)

    all_target = np.zeros(len(iObs)) 
    all_input = np.zeros((len(iObs), numGPRinputs_wet)) 
    #########################################################################################
    #########   MAIN LOOP   ############   MAIN LOOP   ###############   MAIN LOOP  #########
    #########################################################################################
    # Loop through the usable observations. Pull the training data from values associated
    # with these locations. But then place them in the freshly generated 'all_input/target'
    # arrays starting from zero (intarloc)
    print('looping through data to make gpr-input')
    intarloc=0 #Input and target array locations
    for i in iObs:

        # Make sure all the data are good.
        if all(q > 0 for q in observation_data[i-3:i+1, 5]) is False:
            print('Warning: hit a bad observation in', siteyear)
            print([i-3, i-2, i-1, i])
            print(observation_data[i-3:i+1,5])

        #####################################################################################
        #############     GPR TARGET DATA                ####################################
        #####################################################################################
        # Take the difference between observation and 1-step result, 
        # this is what the GPR trains on as 'Target'
        all_target[intarloc] = float(observation_data[i][5]) - float(onestep_data[i][10])
        
        #####################################################################################
        #############     GPR PREDICTION INPUT DATA      ####################################
        #####################################################################################
        # winputs(1) = lagged(1)
        all_input[intarloc][0] = float(observation_data[i-1][5])
        # winputs(2) = lagged(2)
        all_input[intarloc][1] = float(observation_data[i-2][5])
        #  winputs(3) = lagged(3)
        all_input[intarloc][2] = float(observation_data[i-3][5])
        #  winputs(4) = state(t)%smc(1)
        all_input[intarloc][3] = float(onestep_data[i][10])
        #  winputs(5) = forcing(t)%q2
        all_input[intarloc][4] = float(forcing_data[i][6])
        #  winputs(6) = forcing(t)%prcprate
        all_input[intarloc][5] = float(forcing_data[i][10])
        #  winputs(7) = forcing(t)%lwrad
        all_input[intarloc][6] = float(forcing_data[i][9])
        #  winputs(8) = forcing(t)%swrad
        all_input[intarloc][7] = float(forcing_data[i][8])
        #  winputs(9) = forcing(t)%sfcprs
        all_input[intarloc][8] = float(forcing_data[i][7])
        #  winputs(10) = forcing(t)%sfctmp
        all_input[intarloc][9] = float(forcing_data[i][5])
        #  winputs(11)= forcing(t)%sfcspd
        all_input[intarloc][10] = float(forcing_data[i][4])
        #  winputs(12)= state(t)%smc(2)
        all_input[intarloc][11] = float(onestep_data[i][11])
        #  winputs(13)= state(t)%smc(3)
        all_input[intarloc][12] = float(onestep_data[i][12])
        #  winputs(14)= state(t)%smc(4)
        all_input[intarloc][13] = float(onestep_data[i][13])
        #  winputs(15)= state(t)%sh2o(1)
        all_input[intarloc][14] = float(onestep_data[i][14])
        #  winputs(16)= state(t)%sh2o(2)
        all_input[intarloc][15] = float(onestep_data[i][15])
        #  winputs(17)= state(t)%sh2o(3)
        all_input[intarloc][16] = float(onestep_data[i][16])
        #  winputs(18)= state(t)%sh2o(4)
        all_input[intarloc][17] = float(onestep_data[i][17])


        # USE SOME LOGIC TO SPLIT THE TRAINING SET BETWEEN WET AND DRY
        if float(forcing_data[i][10]) > 0:
            wet_train_list.append(intarloc)
        else:
            dry_train_list.append(intarloc)
        intarloc+=1 
    #########################################################################################
    ####### END MAIN LOOP   ########## END MAIN LOOP   ############# END MAIN LOOP  #########
    #########################################################################################
print('data collected, now writing')
# Split up the data for wet/dry and training/prediction
wet_train_input = all_input[wet_train_list]
dry_train_input = all_input[dry_train_list]
wet_train_target = all_target[wet_train_list]
dry_train_target = all_target[dry_train_list]
# Delete out the precipitation column.
dry_train_input = np.delete(dry_train_input, 5, 1)

# Just printing to make sure the number of samples makes sense.
print('total training samples:', len(iObs))
print('number of wet training samples:', len(wet_train_list))
print('number of dry training samples:', len(dry_train_list))

# Write targets to binary file
with open(gpr_mpi_dir+'targets_wet.bin','wb') as target_file:
    np.array(wet_train_target, dtype=np.float64).tofile(target_file)
with open(gpr_mpi_dir+'targets_dry.bin','wb') as target_file:
    np.array(dry_train_target, dtype=np.float64).tofile(target_file)

# Write training inputs to binary file
with open(gpr_mpi_dir+'inputs_wet.bin','wb') as input_file:
    np.array(wet_train_input, dtype=np.float64).tofile(input_file)
# Write training inputs to binary file
with open(gpr_mpi_dir+'inputs_dry.bin','wb') as input_file:
    np.array(dry_train_input, dtype=np.float64).tofile(input_file)

## Write out the number of values for the training data. 
with open(gpr_mpi_dir+'num_wet_training_points.txt', 'w+') as n_file:
    n_file.write(str(wet_train_input.shape[0]))
with open(gpr_mpi_dir+'num_dry_training_points.txt', 'w+') as n_file:
    n_file.write(str(dry_train_input.shape[0]))
##################    END PROGRAM    ################################
