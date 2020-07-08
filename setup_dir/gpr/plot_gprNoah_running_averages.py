#!/gpfsm/dulocal/sles11/other/SLES11.3/miniconda3/2019.03_py3.7/2019-05-15/bin/python

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
steps2average = 200

#########################################################################
########    CHANGE THIS TO BE A RELATIVE PATHWAY TO THE INIT_DIR   ######
#########################################################################
proj_dir = '/discover/nobackup/jframe/p05noahgpr/nasa-intern-poster/'
init_dir = proj_dir + 'soil_moisture/init_dirs/2_2005-2006/'

#########  Set up some lists for the data   ################
x = []
ObservedSM = []
noahSM = []
NoahGPR = []
OneStpSM = []
assimSM = []
p = []

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
        p.append(float(row[9]))
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

##############       Run the averages     ##################
a_ObservedSM = ObservedSM
a_noahSM = noahSM
a_NoahGPR = NoahGPR
a_OneStpSM = OneStpSM
a_assimSM = assimSM
a_p = p
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
            a_p[i] = p[i]

# Start plot
#sp = int(input('what day of the record to start the plot? I recommend 0\n'))
# end plot
#print('The total number of days in the output file is: ', str(int(np.floor(len(x)/48))))
#ep =int(input('How many days do you want to plot? I needs to be greater than when it started\n'))*48
#ep = int(max(sp+1,np.floor(ep)))
sp = 0
ep = x.shape[0]
print('plotting')
#EnsLeg = []
#fig = plt.figure()
#plt.ylabel('Soil Moisture')
#plt.plot(x[sp:ep], a_ObservedSM[sp:ep], color='k', linewidth=0.5)
#EnsLeg.append('Observed Soil Moisture')
#plt.plot(x[sp:ep], a_noahSM[sp:ep], color='b', linewidth=0.5)
#EnsLeg.append('Noah prediction')
#plt.plot(x[sp:ep], a_NoahGPR[sp:ep], '--', color='r', linewidth=1.0)
#EnsLeg.append('NoahMP with GPR prediction')
#plt.plot(x[sp:ep], a_assimSM[sp:ep], '--', color='g', linewidth=1.0)
#EnsLeg.append('Noah with EnKS data assimilation')
#plt.legend(EnsLeg)
#plt.show()
#plt.show()
print('plotting')
fig, ax1 = plt.subplots(figsize=(15,10))
ax1.plot(x[sp:ep],a_ObservedSM[sp:ep], label='Observed soil moisture', color='k', linewidth=0.5)
ax1.plot(x[sp:ep],a_noahSM[sp:ep], label='Noah prediction', color='b', linewidth=0.5)
ax1.plot(x[sp:ep],a_NoahGPR[sp:ep], '--', label='NoahMP with GPR prediction', color='r', linewidth=1.0)
ax1.plot(x[sp:ep],a_assimSM[sp:ep], '--', label='Noah with EnKS data assimilation', color='g', linewidth=1.0)
ax1.set_ylabel('Soil moisture')
ax1.set_xlabel('Days')
plt.legend(loc='upper left')
ax2 = ax1.twinx()
ax2.plot(x[sp:ep],a_p[sp:ep], label='precipitation', linewidth=0.1, color='grey')
ax2.set_ylabel('precip')
plt.legend(loc='upper right')
fig.tight_layout()  # otherwise the right y-label is slightly clipped
plt.show()
