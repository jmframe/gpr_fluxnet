#!/gpfsm/dulocal/sles11/other/SLES11.3/miniconda3/2019.03_py3.7/2019-05-15/bin/python
# This is a script to simply copy over one set of parameters
# to a directory to run the calibrated simulation

import numpy as np
from io import StringIO

year = input("What simulation year at Blodgett? ")
type(year)

# get calibrated parameter VALUES from sce.out
cal_dir = 'soil_moisture/cal_dirs/run_2_'+ str(year) + '/sce.out'
f_read = open(cal_dir, "r")
cal_params = f_read.readlines()[-1]
f_read.close()
cal_params = np.genfromtxt(StringIO(cal_params), dtype=None)
fname = 'soil_moisture/init_dirs/run_2_' + str(year) + '/cal_parms.txt'
with np.printoptions(precision=7, suppress=True):
    with open(fname, 'w') as F:
        for v in range(1, cal_params.shape[0]):
            F.write(str(cal_params[v]))
            F.write('\n')

# --- End Script ---------------------------------------------------
