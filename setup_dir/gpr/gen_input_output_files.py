#!/discover/nobackup/jframe/anaconda3/bin/python

# This script generates binary files for training/testing the gpr.

import numpy as np
from array import array
import csv
import os 


with open('site.txt') as site_file:
    site = int(site_file.readline())
print('generating gpr training data for site ', site)

# Set the path to this script, and work from there.
script_dir = os.path.dirname(__file__) #<-- absolute dir the script is in
#########################################################################
########    CHANGE THIS TO BE A RELATIVE PATHWAY TO THE INIT_DIR   ######
#########################################################################
proj_dir = '/discover/nobackup/jframe/gpr_fluxnet/'
init_dir = proj_dir + 'soil_moisture/init_dirs/run_' + str(site) + '/'
print(init_dir)
# Define the number of parameters that will be used for training the GPR
numGPRinputs_wet = 10
numGPRinputs_dry = 9

# Include a lag to stabalize the GPR results
# Until otherwise specified this lag is three timesteps.
lag = 3

# A list used for generating the GPR target
dif = []

# List to cut down the training data into just what we want to use.
wet_train_list = []
dry_train_list = []

# Counting the number in each of these records, 
# for ensuring consistency, and for making numpy arrays
count = {'forcing':0, 'onestep':0, 'observation':0}

# Grabbing data from two files, and writing to one. Open them at the same time.
# Note that soil moisture is column 5 (sixth column) of observations and 
  # column 10 (11th column) of ouput.out
with open(init_dir + 'obs.txt', 'r') as Observations_file, \
     open(init_dir + 'forcing.txt', 'r') as Forcing_file, \
     open(init_dir + 'output.noah', 'r') as Noah_file, \
     open(init_dir + 'output.onestep', 'r') as Onestep_file:

    # set the number of timesteps to train on, in this case (gpr_fluxnet)
    # We are training on the entire calibrated/equilibrated/onstep record.
    with open(init_dir + 'num_times.txt') as nt:
        ntrain = int(nt.readline())

    # Read the input data as delimited.
    forcing_data = np.genfromtxt(Forcing_file)
    observation_data = np.genfromtxt(Observations_file)
    noah_data = np.genfromtxt(Noah_file)
    onestep_data = np.genfromtxt(Onestep_file)

    # Simply count all the records, to make sure they are the same length
    # Also, we want the record lengths to make numpy arrays
    for obs, out, forc in zip(observation_data, onestep_data, forcing_data):
        if float(obs[5]) > 0:
            count['observation'] = count['observation'] + 1
        else:
            print('something may be wrong with the observation record', obs[5])
        if float(out[10]) > 0:
            count['onestep'] = count['onestep'] + 1
        else:
            print('something may be wrong with the output record', out[10])
        if float(forc[10]) >= 0:
            count['forcing'] = count['forcing'] + 1
        else:
            print('something may be wrong with the forcing record', forc[10])

    # Define numpy arrays, because we'll need these to convert to binary
    # TARGETS: we need both the observation and output to make the target, 
    # i.e., difference(observation, model output)
    # So the better be the same length
    if count['onestep'] == count['observation']:
        all_target = np.zeros(count['observation']-(lag)) 
    else:
        print('The observation and onestep records are not the same length, so cant make GPR target file')
    # INPUTS: we need both the onestep (model states) and the forcing (model inputs) to make the GPR input file
    if count['onestep'] == count['forcing']:
        # Note that numGPRinputs_wet includes precipitation, one more parameter than dry, 
                                                             #will be deleted in dry below.
        all_input = np.zeros((count['observation']-(lag), numGPRinputs_wet))
    else:
        print('The output and forcing records are not the same length, so cant make GPR input file')

    # Now we should have tested that all the counts are equal, so we can replace it with just one value
    count = count['onestep']

    #########################################################################################
    #########   MAIN LOOP   ############   MAIN LOOP   ###############   MAIN LOOP  #########
    #########################################################################################
    # Once again loop through both the input data simultaneously, but now we know the lengths
    loop_counter = 0
    for i in range(lag, count):

        #Input and target array locations
        intarloc = i-(lag)
        # mean indexing
        ito = i-1
        ifrom = i-lag

        #####################################################################################
        #############     GPR TARGET DATA                ####################################
        #####################################################################################
        # Take the difference between observation and 1-step result, 
        # this is what the GPR trains on as 'Target'
        all_target[intarloc] = float(observation_data[i][5]) - float(onestep_data[i][10])
        
        #####################################################################################
        #############     GPR PREDICTION INPUT DATA      ####################################
        #####################################################################################

        # Precipitation
        all_input[intarloc][0] = float(forcing_data[i][10])

        # Soil moisture state
        all_input[intarloc][1] = float(onestep_data[i][10])

        # Lagged soil moisture state
        all_input[intarloc][2] = float(observation_data[i-1][5])
        all_input[intarloc][3] = float(observation_data[i-2][5])
        all_input[intarloc][4] = float(observation_data[i-3][5])

        # Surface Temp
        all_input[intarloc][5] = float(forcing_data[i][5])

        # Short wave solar radiation	
        all_input[intarloc][6] = float(forcing_data[i][8])

        # Long wave solar radiation
        all_input[intarloc][7] = float(forcing_data[i][9])
        # Surface pressure
        all_input[intarloc][8] = float(forcing_data[i][7])
        # Humidity
        all_input[intarloc][9] = float(forcing_data[i][6])
        # Wind speed
        #all_input[intarloc][10] = float(forcing_data[i][4])

        # USE SOME LOGIC TO SPLIT THE TRAINING SET BETWEEN WET AND DRY
        if float(forcing_data[i][10]) > 0:
            if intarloc < ntrain:
                wet_train_list.append(intarloc)
        else:
            if intarloc < ntrain:
                dry_train_list.append(intarloc)
            
        loop_counter = loop_counter+1
    #########################################################################################
    ####### END MAIN LOOP   ########## END MAIN LOOP   ############# END MAIN LOOP  #########
    #########################################################################################

# Split up the data for wet/dry and training/prediction
wet_train_input = all_input[wet_train_list]
dry_train_input = all_input[dry_train_list]
wet_train_target = all_target[wet_train_list]
dry_train_target = all_target[dry_train_list]
# Delete out the precipitation column.
dry_train_input = np.delete(dry_train_input, 0, 1)

# Check that all the sets add up.
n_total_train =  wet_train_input.shape[0] + dry_train_input.shape[0]
if n_total_train != ntrain:
    print('WARNING:"')
    print('the combined sets for training wet and dry GPR is:', n_total_train)
    print('But should be:', ntrain - 1)
    print('Now we have ', wet_train_input.shape[0], 'wet elements and ',\
           dry_train_input.shape[0], 'dry')
else:
    print('Number of training points for wet: ', wet_train_input.shape[0])
    print('Number of training points for dry: ', dry_train_input.shape[0])

# Write targets to binary file
with open('targets_wet.bin','wb') as target_file:
    np.array(wet_train_target, dtype=np.float64).tofile(target_file)
with open('targets_dry.bin','wb') as target_file:
    np.array(dry_train_target, dtype=np.float64).tofile(target_file)

# Write training inputs to binary file
with open('inputs_wet.bin','wb') as input_file:
    np.array(wet_train_input, dtype=np.float64).tofile(input_file)
# Write training inputs to binary file
with open('inputs_dry.bin','wb') as input_file:
    np.array(dry_train_input, dtype=np.float64).tofile(input_file)

## Write out the number of values for the training data. 
with open('num_wet_training_points.txt', 'w+') as n_file:
    n_file.write(str(wet_train_input.shape[0]))
with open('num_dry_training_points.txt', 'w+') as n_file:
    n_file.write(str(dry_train_input.shape[0]))
##################    END PROGRAM    ################################
