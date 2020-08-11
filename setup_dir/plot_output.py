#!/discover/nobackup/jframe/anaconda3/bin/python

import numpy as np
import csv
import sys
import datetime as dt
import matplotlib
matplotlib.use('TkAgg')
import matplotlib.pyplot as plt

#########  Set up some lists for the data   ################
x = []
ObservedSM = []
noahSM = []
p = []

########    import all the data   ########################3## 
with open('obs.txt', 'r') as obsFile:
    obsData = csv.reader(obsFile, delimiter=' ', skipinitialspace=True)
    for obsRow in obsData:
        ObservedSM.append(float(obsRow[5]))
    ObservedSM = np.array(ObservedSM)
with open('output.out', 'r') as oFile:
    fData = csv.reader(oFile, delimiter=' ', skipinitialspace=True )
    for row in fData:
        D = dt.datetime(int(row[0]),1,1) + \
            dt.timedelta((int(row[1])-1) + float(row[2])/24)
        x.append(D)
        p.append(float(row[9]))
        noahSM.append(float(row[10]))
    noahSM = np.array(noahSM)
    x = np.array(x)

# Start plot
sp = int(input('what day of the record to start the plot? I recommend 0\n'))*48
# end plot
print('The total number of days in the output file is: ', str(int(np.floor(len(x)/48))))
print('What dey of record do you want to end want to plot?') 
ep = int(input('It needs to be greater than when it started\n'))*48
ep = int(max(sp+48,np.floor(ep)))

print('plotting')
fig, ax1 = plt.subplots(figsize=(15,10))
ax1.plot(x[sp:ep],ObservedSM[sp:ep], label='Observed soil moisture', color='k', linewidth=0.5)
ax1.plot(x[sp:ep],noahSM[sp:ep], label='Noah prediction', color='b', linewidth=0.5)
ax1.set_ylabel('Soil moisture')
ax1.set_xlabel('Days')
plt.legend(loc='upper left')
ax2 = ax1.twinx()
ax2.plot(x[sp:ep],p[sp:ep], label='precipitation', linewidth=0.1, color='grey')
ax2.set_ylabel('precip')
plt.legend(loc='upper right')
fig.tight_layout()  # otherwise the right y-label is slightly clipped
plt.show()
