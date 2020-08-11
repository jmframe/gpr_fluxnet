#!/discover/nobackup/jframe/anaconda3/bin/python

#Plotting the results of a noah-mp run with calibrated parameters
#Observation vs Model Output

import matplotlib
matplotlib.use('TkAgg')
import matplotlib.pyplot as plt
import csv
import datetime as dt
import os

#Number of records
with open('num_times.txt', 'r') as num_times:
    num_times_read_in = csv.reader(num_times)
    for row in num_times_read_in:
        n_records = int(row[0])

# Start plot
sp = int(input('what time step to start the plot? I recommend 0\n'))
# end plot
print('The total number of records in the output file is: ', n_records)
ep = max(sp+1,int(input('what time step should the plot end? I needs to be greater than when it started\n')))

OutputSM = []
ObservedSM_T = []
ObservedSM = []
x = []
print('thinking...')
with open('output.da','r') as oFile:
    fData = csv.reader(oFile, delimiter=' ', skipinitialspace=True )
    for row in fData:
        D = dt.datetime(int(row[0]),1,1) + \
            dt.timedelta((int(row[1])-1) + float(row[2])/24)
        x.append(D)
        OutputSM.append(float(row[10]))
with open('obs.txt','r') as obsFile_T:
    fData = csv.reader(obsFile_T, delimiter=' ', skipinitialspace=True )
    for row in fData:
        ObservedSM_T.append(float(row[5]))
with open('obs.orig','r') as obsFile:
    fData = csv.reader(obsFile, delimiter=' ', skipinitialspace=True )
    for row in fData:
        ObservedSM.append(float(row[5]))
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
plt.legend(EnsLeg)
plt.show()
