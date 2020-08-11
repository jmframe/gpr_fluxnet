#!/discover/nobackup/jframe/anaconda3/bin/python

import numpy as np
import csv
import sys
import datetime as dt
import matplotlib
matplotlib.use('TkAgg')
import matplotlib.pyplot as plt

#########################################################################
########    set the number of time steps to average                ######
#########################################################################
steps2average = 500

#########################################################################
########    CHANGE THIS TO BE A RELATIVE PATHWAY TO THE INIT_DIR   ######
#########################################################################
proj_dir = '/discover/nobackup/jframe/p05noahgpr/nasa-intern-poster/'
init_dir = proj_dir + 'soil_moisture/init_dirs/run_2_2000/'


#########  Set up some lists for the data   ################
x = []
ObservedSM = []
noahSM = []
NoahGPR = []
OneStpSM = []
assimSM = []

########    import all the data   ########################3##                                 
with open('gpr_noah_prediction.txt', 'r') as gprFile:
    gprData = csv.reader(gprFile, delimiter=' ')
    for gprRow in gprData:
        NoahGPR.append(float(gprRow[0]))
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
a = 17521
b = ObservedSM.shape[0]
# Mean divergence
MD = np.sum((np.mean(ObservedSM[a:b]) - ObservedSM[a:b])**2)
#Sum of squared error
SSE_noah_only = np.sum((ObservedSM[a:b] - noahSM[a:b])**2)
SSE_noah_gpr = np.sum((ObservedSM[a:b] - NoahGPR[a:b])**2)
#Nash sutcliffe efficiency
NS_noah_gpr = 1 - (SSE_noah_gpr/MD)
NS_noah_only = 1 - (SSE_noah_only/MD)
print('The Nash-Sutcliffe Efficiency for the out of sample year is')
print('For Noah alone: ', str(NS_noah_only))
print('For Noah with GPR: ', str(NS_noah_gpr))
# Root mean squared error
print('The root mean squared errors for the out of sample year are')
print('For Noah alone: ', str(np.sqrt(SSE_noah_only/(b-a))))
print('For Noah with GPR: ', str(np.sqrt(SSE_noah_gpr/(b-a))))

##############       Run the averages     ##################
a_ObservedSM = ObservedSM
a_noahSM = noahSM
a_NoahGPR = NoahGPR
a_OneStpSM = OneStpSM
a_assimSM = assimSM
for i in range(0,x.shape[0]):
    if i > (steps2average/2+1):
        if i < (x.shape[0] + steps2average/2):
            a1 = int(np.floor(i - steps2average/2))
            a2 = int(np.ceil(i + steps2average/2))
            a_ObservedSM[i] = np.mean(ObservedSM[a1:a2])
            a_noahSM[i] = np.mean(noahSM[a1:a2])
            a_NoahGPR[i] = np.mean(NoahGPR[a1:a2])
            a_OneStpSM[i] = np.mean(OneStpSM[a1:a2])
            a_assimSM[i] = np.mean(assimSM[a1:a2])

# Start plot
sp = int(input('what day of the record to start the plot? I recommend 0\n'))
# end plot
print('The total number of days in the output file is: ', str(int(np.floor(len(x)/48))))
ep =int(input('How many days do you want to plot? I needs to be greater than when it started\n'))*48
ep = int(max(sp+1,np.floor(ep)))

print('plotting')
colorz = ['coral','g','c','m','y','b','teal','maroon', 'slategrey', 'sky blue', 'thistle1']
EnsLeg = []
fig = plt.figure()
plt.ylabel('Soil Moisture')
plt.plot(x[sp:ep], a_ObservedSM[sp:ep], color='k', linewidth=0.5)
EnsLeg.append('Observed Soil Moisture')
plt.plot(x[sp:ep], a_noahSM[sp:ep], color='b', linewidth=0.5)
EnsLeg.append('Noah prediction')
plt.plot(x[sp:ep], a_NoahGPR[sp:ep], '--', color='r', linewidth=1.0)
EnsLeg.append('NoahMP with GPR prediction')
plt.plot(x[sp:ep], a_assimSM[sp:ep], '--', color='g', linewidth=1.0)
EnsLeg.append('Noah with EnKS data assimilation')
plt.legend(EnsLeg)
plt.show()
plt.show()
