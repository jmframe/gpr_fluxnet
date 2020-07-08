#!/gpfsm/dulocal/sles11/other/SLES11.3/miniconda3/2019.03_py3.7/2019-05-15/bin/python

import numpy as np
import csv
import sys
import datetime as dt
output = sys.argv[1]
#########  Set up some lists for the data   ################
x = []
ObservedSM = []
noahSM = []
NoahGPR = []
include = []
########    import all the data   ########################3## 
with open(output, 'r') as gprFile:
    gprData = csv.reader(gprFile, delimiter=' ', skipinitialspace=True)
    for gprRow in gprData:
        NoahGPR.append(float(gprRow[10]))
    NoahGPR = np.array(NoahGPR)
with open('obs.txt', 'r') as obsFile:
    obsData = csv.reader(obsFile, delimiter=' ', skipinitialspace=True)
    i=0
    for obsRow in obsData:
        if float(obsRow[5]) >= 0:
            include.append(i)
        ObservedSM.append(float(obsRow[5]))
        i+=1
    ObservedSM = np.array(ObservedSM)
#################  Calculate some statistics  #### 
#Sum of squared error
SSE_noah_gpr = np.sum((ObservedSM[include] - NoahGPR[include])**2)
print(np.sqrt(SSE_noah_gpr/len(include)))
