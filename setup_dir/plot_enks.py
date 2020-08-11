#!/discover/nobackup/jframe/anaconda3/bin/python

#Plotting the results of a noah-mp run with calibrated parameters
#Observation vs Model Output

import matplotlib
matplotlib.use('TkAgg')
import matplotlib.pyplot as plt
import csv
import datetime as dt
import os

days_to_plot = int(input('How many days do you want to plot?\n'))
 
print('thinking...')

x = []
y_obs = []
y_mod = []
y_enks = []
p = []
count_row = 0

with open('output.noah','r') as csvfile:
    model = csv.reader(csvfile, delimiter=' ', skipinitialspace=True )
    for row in model:
        p.append(float(row[9]))
        y_mod.append(float(row[10]))
    simLength = len(y_mod)
with open('enks_mean.out','r') as csvfile:
    assimilation = csv.reader(csvfile, delimiter=' ', skipinitialspace=True )
    for row in assimilation:
        y_enks.append(float(row[7]))
with open('obs.txt','r') as csvfile:
    observations = csv.reader(csvfile, delimiter=' ', skipinitialspace=True )
    for row in observations:
        if count_row < simLength:	
            D = dt.datetime(int(row[0]),1,1) + \
                dt.timedelta((int(row[1])-1) + float(row[2])/24)
            x.append(D)
            y_obs.append(float(row[5]))
            count_row = count_row + 1

sYear = x[0].year
sMonth = x[0].month
sDay = x[0].day

startPlot = dt.date(sYear,sMonth,sDay)
endPlot = startPlot + dt.timedelta(days_to_plot)
plotDates = [startPlot, endPlot]

print('plotting...')
fig, ax1 = plt.subplots(figsize=(15,10)) 
ax1.plot(x,y_obs, 'k', label='Observation')
ax1.plot(x,y_mod, label='Un-perturbed model')
ax1.plot(x,y_enks, 'r--', label='Assimilation mean')
ax1.set_xlim(plotDates)
ax1.set_ylabel('Soil moisture')
plt.legend(loc='upper left')
ax2 = ax1.twinx()
ax2.plot(x,p, label='precipitation', linewidth=0.1, color='grey')
ax2.set_ylabel('precip')
ax2.set_xlim(plotDates)
plt.title('Observed, modeled, and assimilated soil moisture')
plt.legend(loc='upper right')
fig.tight_layout()  # otherwise the right y-label is slightly clipped
plt.show()
