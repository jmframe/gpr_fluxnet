#!/gpfsm/dulocal/sles11/other/SLES11.3/miniconda3/2019.03_py3.7/2019-05-15/bin/python

import numpy as np
import csv
import sys
import datetime as dt

#########################################################################
########    CHANGE THIS TO BE A RELATIVE PATHWAY TO THE INIT_DIR   ######
#########################################################################
proj_dir = '/discover/nobackup/jframe/p05noahgpr/nasa-intern-poster/'
init_dir = proj_dir + 'soil_moisture/init_dirs/2_2000-2004/'

#########  Set up some lists for the data   ################
x = []
ObservedSM = []
noahSM = []
NoahGPR = []
OneStpSM = []
assimSM = []

########    import all the data   ########################3## 
with open('output.out', 'r') as gprFile:
    gprData = csv.reader(gprFile, delimiter=' ', skipinitialspace=True)
    for gprRow in gprData:
        NoahGPR.append(float(gprRow[10]))
    NoahGPR = np.array(NoahGPR)
with open(init_dir + 'obs.txt', 'r') as obsFile:
    obsData = csv.reader(obsFile, delimiter=' ', skipinitialspace=True)
    for obsRow in obsData:
        ObservedSM.append(float(obsRow[5]))
    ObservedSM = np.array(ObservedSM)
with open(init_dir + 'output.noah', 'r') as oFile:
    fData = csv.reader(oFile, delimiter=' ', skipinitialspace=True )
    for row in fData:
        D = dt.datetime(int(row[0]),1,1) + \
            dt.timedelta((int(row[1])-1) + float(row[2])/24)
        x.append(D)
        noahSM.append(float(row[10]))
    noahSM = np.array(noahSM)
    x = np.array(x)
with open(init_dir + 'output.onestep', 'r') as oFile:
    fData = csv.reader(oFile, delimiter=' ', skipinitialspace=True )
    for row in fData:
        OneStpSM.append(float(row[10]))
    OneStpSM = np.array(OneStpSM)
with open(init_dir + 'output.dassim', 'r') as oFile:
    fData = csv.reader(oFile, delimiter=' ', skipinitialspace=True )
    for row in fData:
        assimSM.append(float(row[10]))
    assimSM = np.array(assimSM)


#################  Calculate some statistics  #### 
a = 3
b = 17520
# Mean divergence
MD = np.sum((np.mean(ObservedSM[a:b]) - ObservedSM[a:b])**2)
#Sum of squared error
SSE_noah_only = np.sum((ObservedSM[a:b] - noahSM[a:b])**2)
SSE_noah_gpr = np.sum((ObservedSM[a:b] - NoahGPR[a:b])**2)
SSE_noah_assim = np.sum((ObservedSM[a:b] - assimSM[a:b])**2)
#Nash sutcliffe efficiency
NS_noah_gpr = 1 - (SSE_noah_gpr/MD)
NS_noah_assim = 1 - (SSE_noah_assim/MD)
NS_noah_only = 1 - (SSE_noah_only/MD)
print('The Nash-Sutcliffe Efficiency for the test year (2005) is')
print('For Noah alone: ', str(NS_noah_only))
print('For Noah with Data Assimilation: ', str(NS_noah_assim))
print('For Noah with GPR: ', str(NS_noah_gpr))
# Root mean squared error
print('The root mean squared errors for the test year (2005) are')
print('For Noah alone: ', str(np.sqrt(SSE_noah_only/(b-a))))
print('For Noah with Data Assimilation: ', str(np.sqrt(SSE_noah_assim/(b-a))))
print('For Noah with GPR: ', str(np.sqrt(SSE_noah_gpr/(b-a))))

# END
