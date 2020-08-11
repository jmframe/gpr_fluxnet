#!/discover/nobackup/jframe/anaconda3/bin/python

import numpy as np
import sys

#########################################################################
########    CHANGE THIS TO BE A RELATIVE PATHWAY TO THE INIT_DIR   ######
#########################################################################
proj_dir = '/discover/nobackup/jframe/p05noahgpr/nasa-intern-poster/'
init_dir = proj_dir + 'soil_moisture/init_dirs/run_2_2000/'

# Open all the data files and get the information in Numpy Arrays

with open('lag.txt', 'r') as lag_file:
    lag = int(np.genfromtxt(lag_file))

with open('wet_prediction_indices.txt', 'r') as index_file:
    iwet = np.genfromtxt(index_file, dtype=int)
    iwet_list = list(iwet)
    wet_tup_gpr = tuple(np.array(iwet, dtype=int))
    wet_tup_out = tuple(np.array(iwet-lag+1, dtype=int))
with open('dry_prediction_indices.txt', 'r') as index_file:
    idry = np.genfromtxt(index_file, dtype=int)
    idry_list = list(idry)
    dry_tup_gpr = tuple(np.array(idry, dtype=int))
    dry_tup_out = tuple(np.array(idry-lag+1, dtype=int))

with open('wet_predictions.bin', 'rb') as f:
    wet_predictionz = np.fromfile(f, dtype=np.float64)

with open('dry_predictions.bin', 'rb') as f:
    dry_predictionz = np.fromfile(f, dtype=np.float64)

with open(init_dir + 'output.noah', 'r') as f:
    outputFromFile = np.genfromtxt(f)

# The predictions rely on a lag, so no predictions can be made before the number
# of time steps is equal or greater than the lag value.
gpr_noah = np.zeros(wet_predictionz.shape[0] + dry_predictionz.shape[0] + lag)
for i in range(0,lag):
    gpr_noah[i] = outputFromFile[i][10]

# Add both the wet and dry predictions to the noah model output
gpr_noah[iwet_list] = wet_predictionz + outputFromFile[wet_tup_out,10]
gpr_noah[idry_list] = dry_predictionz + outputFromFile[dry_tup_out,10]

# And finally print the results to a file
with open('gpr_noah_prediction.txt', 'w+') as f:
    for i in range(0,gpr_noah.shape[0]):
        f.write(str(gpr_noah[i]))
        f.write('\n')

