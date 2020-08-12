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
with open('output.da', 'r') as f:
    # Read the input data as delimited.
    df = pd.read_csv(f, delim_whitespace=True, header=None)
    df.columns = ['year', 'day', 'hour', 'x1', 'x2', 'x3', 'x4', 'x5', 'x6','x7','sm',
                  'x8','x9','x10','x11','x12','x13','x14','x15','x16','x17','x18','x19','x20','x21']
    df2 = df.loc[(df.year == ty) & (df.day >= tds) & (df.day <= tde), ['year','day','hour','sm']]
    print(df2)
    
## Write out the 30 day 'forecast' to file.
if tds == 0:
    df2.to_csv('30_day_summary.txt', sep=' ', header=False, index=False, index_label=None)
else:
    with open('30_day_summary.txt', 'r') as f:
        df = pd.read_csv(f, delim_whitespace=True, header=None)
        df.append(df2)

# Move the test start date one step forward
with open('test_day_start.txt', 'w+') as f:
    f.write(str(tds+1))
