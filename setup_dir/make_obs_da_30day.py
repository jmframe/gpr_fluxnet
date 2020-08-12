#!/discover/nobackup/jframe/anaconda3/bin/python
import numpy as np
import pandas as pd

# Save the year that we are testing.
with open('year.txt') as f:
    ty = int(f.readline())
# Note the day of testing year that we lose observations
with open('test_day_start.txt') as f:
    tds = int(f.readline())
tde = tds + 30

# Note that soil moisture is column 5 (sixth column) of observations and 
  # column 10 (11th column) of ouput.out
with open('obs_full.txt', 'r') as observations_file:
    # Read the input data as delimited.
    df = pd.read_csv(observations_file, delim_whitespace=True, header=None)
    df.columns = ['year', 'day', 'hour', 'x1', 'x2', 'sm1', 'sm2', 'x3']
    #conds = [(df.year == ty) & (df.day >= tds) & (df.day <= tde)]
    #print(conds)
    df.loc[(df.year == ty) & (df.day >= tds) & (df.day <= tde), 'sm1'] = -9999
    df.loc[(df.year == ty) & (df.day >= tds) & (df.day <= tde), 'sm2'] = -9999

## Write out the number of values for the training data. 
df.to_csv('obs.txt', sep=' ', header=False, index=False, index_label=None)

