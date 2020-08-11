#!/discover/nobackup/jframe/anaconda3/bin/python

import numpy as np
import csv
import pandas as pd
import pickle as pkl

# load site/year combination
siteyears = np.genfromtxt('data/pals/Site_Years.txt', delimiter = ' ')
main_dir = '/discover/nobackup/jframe/gpr_fluxnet/soil_moisture/'
list_siteyears = []
for i in range(siteyears.shape[0]):
    list_siteyears.append(str(int(siteyears[i][0]))+'_'+str(int(siteyears[i][1])))

pd_results = pd.DataFrame(index=list_siteyears, columns=['RMSE_CalNoah_out','RMSE_NoahGPR_out'])

for S, Y in siteyears:
    siteyear = str(int(S))+'_'+str(int(Y))
#    gpr_dir = main_dir + 'gpr-noah/' + 'train_' + str(siteyear) + '/'
    test_gpr_dir = main_dir + 'gpr-noah/' + 'test_' + str(siteyear) + '/'
    #########  Set up some lists for the data   ################
#    x = []
#    Observed = []
#    Cal = []
#    GPR = []
#    inSample = []
#    outSample = []
#    ########    import all the data   ########################3## 
#    with open(gpr_dir+'obs.txt', 'r') as obsFile:
#        obsData = csv.reader(obsFile, delimiter=' ', skipinitialspace=True)
#        i=0
#        for obsRow in obsData:
#            if float(obsRow[5]) >= 0:
#                inSample.append(i)
#            else:
#                outSample.append(i)
#            Observed.append(float(obsRow[5]))
#            i+=1
#        Observed = np.array(Observed)
#    
#    with open(gpr_dir+'output.noah', 'r') as calFile:
#        calData = csv.reader(calFile, delimiter=' ', skipinitialspace=True)
#        for calRow in calData:
#            Cal.append(float(calRow[10]))
#        Cal = np.array(Cal)
#    
#    with open(gpr_dir+'output.gpr', 'r') as gprFile:
#        gprData = csv.reader(gprFile, delimiter=' ', skipinitialspace=True)
#        for gprRow in gprData:
#            GPR.append(float(gprRow[10]))
#        GPR = np.array(GPR)
#    
#    #################  Calculate some statistics  ####
#    lIS = len(inSample)
#    lOS = len(outSample)
#    L = Observed.shape[0] 
#    # Mean divergence
#    MD = np.sum((np.mean(Observed[inSample]) - Observed[inSample])**2)
#    # RMSE
#    Cal_RMSE = round(np.sqrt(np.sum((Observed[inSample] - Cal[inSample])**2)/lIS),5)
#    GPR_RMSE = round(np.sqrt(np.sum((Observed[inSample] - GPR[inSample])**2)/lIS),5)
#    # NSE
#    Cal_NSE = round(1-np.sum((Observed[inSample] - Cal[inSample])**2)/MD,5)
#    GPR_NSE = round(1-np.sum((Observed[inSample] - GPR[inSample])**2)/MD,5)
#    #Sum of squared error values
#    print('in_CalNOAH_'+siteyear,   Cal_RMSE)
#    print('in_GPRNOAH_'+siteyear, GPR_RMSE)

    noah=[]
    gpr=[]
    obs=[]
    with open(test_gpr_dir+'obs.txt', 'r') as obsFile:
        obsData = csv.reader(obsFile, delimiter=' ', skipinitialspace=True)
        for obsRow in obsData:
            obs.append(float(obsRow[5]))
        obs = np.array(obs)
    with open(test_gpr_dir+'output.noah', 'r') as calFile:
        calData = csv.reader(calFile, delimiter=' ', skipinitialspace=True)
        for calRow in calData:
            noah.append(float(calRow[10]))
        noah = np.array(noah)
    with open(test_gpr_dir+'output.gpr', 'r') as gprFile:
        gprData = csv.reader(gprFile, delimiter=' ', skipinitialspace=True)
        for gprRow in gprData:
            gpr.append(float(gprRow[10]))
        gpr = np.array(gpr)

    NoahRMSE = round(np.sqrt(np.sum((obs - noah)**2)/obs.shape[0]),5)
    GPRRMSE = round(np.sqrt(np.sum((obs - gpr)**2)/obs.shape[0]),5)
    MD = np.sum((np.mean(obs)-obs)**2)
    NoahNSE = round(1-np.sum((obs - noah)**2)/MD,5)
    GPRNSE = round(1-np.sum((obs - gpr)**2)/MD,5)
    print('out_CalNOAH_'+siteyear,NoahRMSE)
    print('out_GPRNOAH_'+siteyear,GPRRMSE)
    pd_results.loc[[siteyear],:] = [NoahRMSE, GPRRMSE]
print(pd_results)
pd_results.to_csv('results.txt', sep=', ')
