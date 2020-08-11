#!/discover/nobackup/jframe/anaconda3/bin/python

#Plotting the results of a noah-mp run with calibrated parameters
#Observation vs Model Output

import matplotlib
matplotlib.use('TkAgg')
import numpy as np
import matplotlib.pyplot as plt
import csv
import datetime as dt
import os

mainPath = '/discover/nobackup/jframe/gpr_fluxnet/'

# load site/year combination
sites =	np.genfromtxt(mainPath + 'data/pals/Site_Years.txt', delimiter = ' ')
Ns = sites.shape[0]

#-----  Set up runtime directories    ---------------------------
# Delete and reinit test dirs
plotLoc = mainPath + 'plots/init_plots/'

for s in range(0, Ns):

    # get sites and year, need in string
    S = str(int(sites[s,0]))
    Y = str(int(sites[s,1]))
    siteyear = S + '_' + Y    
    # Screen report
    print('Plotting initialization results for site = %d, year = %d, ' % (sites[s,0],  sites[s,1]))

    dirPath = mainPath + 'soil_moisture/init_dirs/'
    dataDir = dirPath+'run_'+siteyear+'/'

    x = []
    xDays = []
    y_obs = []
    y_noah = []
    y_assm = []
    y_1stp = []
    p = []
    count_row = 0
    xDays.append(0)
    with open(dataDir+'output.noah','r') as csvfile:
        model = csv.reader(csvfile, delimiter=' ', skipinitialspace=True )
        for row in model:
            p.append(float(row[9]))
            y_noah.append(float(row[10]))
        simLength = len(y_noah)
    with open(dataDir+'output.onestep','r') as csvfile:
        model = csv.reader(csvfile, delimiter=' ', skipinitialspace=True )
        for row in model:
            y_1stp.append(float(row[10]))
        simLength = len(y_1stp)
    with open(dataDir+'output.da','r') as csvfile:
        model = csv.reader(csvfile, delimiter=' ', skipinitialspace=True )
        for row in model:
            y_assm.append(float(row[10]))
        simLength = len(y_assm)
    with open(dataDir+'obs.txt','r') as csvfile:
        observations = csv.reader(csvfile, delimiter=' ', skipinitialspace=True )
        for row in observations:
            if count_row < simLength:	
                D = dt.datetime(int(row[0]),1,1) + \
                    dt.timedelta((int(row[1])-1) + float(row[2])/24)
                x.append(D)
                y_obs.append(float(row[5]))
                if count_row > 0:
                    xDays.append(xDays[count_row-1] + 1/48)
                count_row = count_row + 1
    
    sYear = x[0].year
    sMonth = x[0].month
    sDay = x[0].day
    
    startPlot = dt.date(sYear,sMonth,sDay)
    endPlot = startPlot + dt.timedelta(365)
    plotDates = [startPlot, endPlot]
    
    fig, ax1 = plt.subplots(figsize=(15,10)) 
    ax1.plot(x,y_obs, label='Observed')
    ax1.plot(x,y_noah, label='Noah')
    ax1.plot(x,y_1stp, label='Noah 1-step')
    ax1.plot(x,y_assm, label='Noah EnKS')
    #ax1.set_xlim(plotDates)
    ax1.set_ylabel('Soil moisture')
    ax1.set_ylim([0,1])
    #ax1.set_xlabel('')
    plt.legend(loc='upper left')
    ax2 = ax1.twinx()
    ax2.plot(x,p, label='precipitation', linewidth=0.1, color='grey')
    ax2.set_ylabel('precip')
    #ax2.set_xlim(plotDates)
    plt.title(siteyear + ' Observed and modeled soil moisture')
    plt.legend(loc='upper right')
    fig.tight_layout()  # otherwise the right y-label is slightly clipped
    plt.savefig(plotLoc+siteyear+'.png', dpi=400)
    plt.close()
