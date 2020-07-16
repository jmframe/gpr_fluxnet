#!/discover/nobackup/jframe/anaconda3/bin/python

# Generate data to Calibrate on ALL YEARS EXCEPT ONE (Leave one out)
# This is based on some files with the lists of all test/train years.
# In this script I am going to replace soil moisture observations
# with a mask value (-9999) if the years aren't in our set.

import numpy as np
import csv
import datetime as dt
import os
print('working................')
main_dir = '/discover/nobackup/jframe/gpr_fluxnet/'
test_dir = main_dir+'data/pals/TestSiteYears.txt'
train_dir= main_dir+'data/pals/TrainSiteYears.txt'

with open(train_dir, mode='r') as train_file:
    train_read = csv.reader(train_file, delimiter=' ')
    train_dict = {rows[0]:rows[1:] for rows in train_read}
with open(test_dir, mode='r') as test_file:
    test_read = csv.reader(test_file, delimiter=' ')
    test_dict = {rows[0]:rows[1] for rows in test_read}

# Save the rows of each observations file, site full record
# we'll need t o do this for every site in the Site_years.txt file
obs_train_dict = {key:[] for key in train_dict.keys()}
forc_train_dict = {key:[] for key in train_dict.keys()}
obs_test_dict = {key:[] for key in test_dict.keys()}
forc_test_dict = {key:[] for key in test_dict.keys()}
# Observation training data
print('Working on the observation Training data')
for siteyear in train_dict.keys():
    site = siteyear.split('-')[0]
    year = siteyear.split('-')[1]
    print( 'working on site:',site, 'and year: ', year)
    # Open ing the site data file, which has all the years
    with open('obs_' + str(site)+'.txt','r') as File:
        obsDat = np.genfromtxt(File)
        obsTrain = obsDat
        #  Since the observations file is very long, only loop through this once.
        i=0
        for row in obsTrain:
             # In these files the first column is the year, bue we leave as text.
             f_year = str(int(row[0]))
             if f_year in test_dict[siteyear]:
                 obsTrain[i][5] = -9999
             if f_year not in train_dict[siteyear]:
                 obsTrain[i][5] = -9999
             i += 1
    with open('obs_' + str(site) + '_' + str(year) + '_train.txt', 'w') as File:
        for row in obsTrain:
            i = 0
            for col in row:
                if i < 2:
                    File.write(str(int(round(col))))
                else:
                    File.write(str(col)) 
                File.write(' ')      
                i += 1
            File.write('\n')
# Observation testing data
print('Working on the observation Testing data')
for siteyear in train_dict.keys():
    site = siteyear.split('-')[0]
    year = siteyear.split('-')[1]
    print( 'working on site:',site, 'and year: ', year)
    # Open ing the site data file, which has all the years
    with open('obs_' + str(site)+'.txt','r') as File:
        obsDat = np.genfromtxt(File)
        obsTest = obsDat
        #  Since the observations file is very long, only loop through this once.
        i=0
        iExport=[]
        for row in obsTest:
             # In these files the first column is the year, bue we leave as text.
             f_year = int(row[0])
             if f_year == int(test_dict[siteyear]):
                 iExport.append(i)
             i += 1
        obsTest = obsTest[iExport]
    with open('obs_' + str(site) + '_' + str(year) + '_test.txt', 'w') as File:
        for row in obsTest:
            i = 0
            for col in row:
                if i < 2:
                    File.write(str(int(round(col))))
                else:
                    File.write(str(col)) 
                File.write(' ')      
                i += 1
            File.write('\n')
print('Working on the forcing Testing data')
for siteyear in train_dict.keys():
    site = siteyear.split('-')[0]
    year = siteyear.split('-')[1]
    print( 'working on site:',site, 'and year: ', year)
    # Open ing the site data file, which has all the years
    with open('forcing_' + str(site)+'.txt','r') as File:
        forcDat = np.genfromtxt(File)
        forcTest = forcDat
        #  Since the observations file is very long, only loop through this once.
        i=0
        iExport=[]
        for row in forcTest:
             # In these files the first column is the year, bue we leave as text.
             f_year = int(row[0])
             if f_year == int(test_dict[siteyear]):
                 iExport.append(i)
             i += 1
        forcTest = forcTest[iExport]
    with open('forcing_' + str(site) + '_' + str(year) + '_test.txt', 'w') as File:
        for row in forcTest:
            i = 0
            for col in row:
                if i < 2:
                    File.write(str(int(round(col))))
                else:
                    File.write(str(col)) 
                File.write(' ')      
                i += 1
            File.write('\n')
print('done')
