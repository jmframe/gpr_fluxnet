#!/discover/nobackup/jframe/anaconda3/bin/python

# Generate data to Calibrate on ONE YEAR ONLY, and run the model on those remaining. 

import matplotlib.pyplot as plt
import csv
import datetime as dt
import os

main_dir = '/discover/nobackup/jframe/p05noahgpr/blodgett_oneYearCal/'
Site_Year_dir = main_dir+'data/pals/Site_Years.txt'

siteList = []
sitesYears = []

# Here I am creating a dataframe with all the lists 
# to add in the forcing and observation records.
d = {} #Dictionary

#Create a list for each site-year combination
with open(Site_Year_dir, 'r') as SY:
    Sites_and_Years = csv.reader(SY, delimiter=' ')
    # each row will have a different site/year combination
    for row in Sites_and_Years:
        # Stores site AND Years as two columns for use down below when the file is closed
        sitesYears.append(row)
        # Stors ONLY sites, only add if it is new
        if row[0] not in siteList:
            siteList.append(row[0])
        # and for both observations and forcings
        for prefix in ['obs_', 'forcing_']:
            # Naming the columns with site and year, so they can be referenced below.
            column = prefix + str(row[0]) + '_' + str(row[1])
            d[column]=[]
print(d)

# Save the rows of each observations file, site full record
# we'll need to do this for every site in the Site_years.txt file
for site in siteList:
    for prefix in ['obs_', 'forcing_']:
        # Opening the site data file, which has all the years
        File = open(prefix + str(site)+'.txt','r')
        # Since the observations file is very long, only loop through this once.
        for row in File:
            # In these files the first column is the year, bue we leave as text.
            f_year = int(row[0:4])
            # Then loop through the dictionary to find the right place to put the record.
            for column in d:
                # Grab the site and year from the text. 
                # conditional on the digits in the site.
                # This requires a bit of logic to get the right values
                if str(column[0]) == 'o':
                    l1 = 10
                    s1 = 4
                    y1 = 6
                if str(column[0]) == 'f':
                    l1 = 14
                    s1 = 8 
                    y1 = 10
                if len(column) == l1:
                    d_site = int(column[s1])
                    d_year = int(column[y1:y1+4])
                if len(column) == l1+1:
                    d_site = int(column[s1:s1+1])
                    d_year = int(column[y1+1:y1+5])
                # Now that we have the site from the observation/forcing file
                # add it to our dictionary, ONLY if it is part of our training set
                if d_year == f_year:
                    if prefix[0] == column[0]:
                        d[prefix + str(d_site) + '_' + str(d_year)].append(row)

# Now we generate the site/year forcing and observation model files
for prefix in ['obs_', 'forcing_']:
    for site, year in sitesYears:
        # This site_year text is used three times below, so saving it
        site_year = str(site) + '_' + str(year)
        # This should be the number of records for each year, per site
        yearLength = len(d[prefix + site_year])
        # Open the file for writing
        with open(prefix + site_year + '.txt','w') as F:
            # Loop down the file
            for i in range(0,yearLength):
                # And finally write in the information stored in the dictionary
                row = d[prefix + site_year][i]
                F.write(row)
