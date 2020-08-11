#!/discover/nobackup/jframe/anaconda3/bin/python

#Plotting the results of a noah-mp run with calibrated parameters
#Observation vs Model Output

import matplotlib
matplotlib.use('TkAgg')
import matplotlib.pyplot as plt
import csv
import datetime as dt
import os

#Number of ensembles
with open('da_flag.txt', 'r') as da_flag:
    da_flag_read_in = csv.reader(da_flag)
    for row in da_flag_read_in:
        EnsN = min(9,abs(int(row[0])))
print('The number of ensembles plotting is: ', EnsN)
#Number of records
with open('num_times.txt', 'r') as num_times:
    num_times_read_in = csv.reader(num_times)
    for row in num_times_read_in:
        n_records = int(row[0])

#column_list = ['year', 'day', 'time', 'temp', 'wind', 'pressure', 'humidity', 'LW_rad', 'SW_rad', 'precip', 'SM1', 'SM2', 'SM3', 'SM4', 'rtmass', 'wood', 'lfmass', 'stmass', 'qe', 'qh', 'nee']
#print('output:column | year:0 | day:1 | time:2 temp:3 | wind:4 | pressure:5     | humidity:6 | LW_rad:7 | SW_rad:8 | precip:9 SM1:10 | SM2:11 | SM3:12 | SM4    :13 | rtmass:14 | wood:15 | lfmass:16 | stmass:17 | qe:18 | qh:19 | nee:20')

#F = int(input('what column in the Output data do you want to plot?\n'))

# Start plot
sp = int(input('what time step to start the plot? I recommend 0\n'))
# end plot
print('The total number of records in the output file is: ', n_records)
ep = max(sp+1,int(input('what time step should the plot end? I needs to be greater than when it started\n')))

OutputSM = []
ObservedSM_T = []
ObservedSM = []
OpenMeanSM = []
x = []
EnsembleSM = {}
for iE in range(1,EnsN):
    EnsembleSM['e'+str(iE)] = []
print('thinking...')
with open('output.out','r') as oFile:
    fData = csv.reader(oFile, delimiter=' ', skipinitialspace=True )
    for row in fData:
        D = dt.datetime(int(row[0]),1,1) + \
            dt.timedelta((int(row[1])-1) + float(row[2])/24)
        x.append(D)
        OutputSM.append(float(row[10]))
for iE in range(1, EnsN):
    with open('open_'+str(iE)+'.out','r') as oFile:
        fData = csv.reader(oFile, delimiter=' ', skipinitialspace=True )
        for row in fData:
            EnsembleSM['e'+str(iE)].append(float(row[10]))
with open('obs.txt','r') as obsFile_T:
    fData = csv.reader(obsFile_T, delimiter=' ', skipinitialspace=True )
    for row in fData:
        ObservedSM_T.append(float(row[5]))
with open('obs.orig','r') as obsFile:
    fData = csv.reader(obsFile, delimiter=' ', skipinitialspace=True )
    for row in fData:
        ObservedSM.append(float(row[5]))
with open('open_mean.out','r') as openLoopMeanFile:
    fData = csv.reader(openLoopMeanFile, delimiter=' ', skipinitialspace=True )
    for row in fData:
        OpenMeanSM.append(float(row[7]))
print('plotting')
colorz = ['coral','g','c','m','y','b','teal','maroon', 'slategrey', 'sky blue', 'thistle1']
EnsLeg = []
fig = plt.figure() 
plt.ylabel('Soil Moisture')
plt.plot(x[sp:ep], ObservedSM[sp:ep], color='grey', linewidth=0.5)
EnsLeg.append('Observed Soil Moisture')
plt.plot(x[sp:ep], ObservedSM_T[sp:ep], '-', color='k', linewidth=1.5)
EnsLeg.append('Observed Soil Moisture (cdf matched)')
plt.plot(x[sp:ep], OutputSM[sp:ep], '--', color='lime', linewidth=1.5)
EnsLeg.append('Assimilated model prediction')
plt.plot(x[sp:ep], OpenMeanSM[sp:ep], '-', color='r', linewidth=2.5)
EnsLeg.append('Open loop mean')
plt.plot(x[sp:ep], EnsembleSM['e1'][sp:ep], color='pink', linewidth=1.5)
EnsLeg.append('Un-assimilated prediction')
for iE in range(1, EnsN):
    plt.plot(x[sp:ep], EnsembleSM['e'+str(iE)][sp:ep], color=colorz[iE-1], linewidth=0.5)
    EnsLeg.append('ensemble member')
plt.legend(EnsLeg)
plt.show()
