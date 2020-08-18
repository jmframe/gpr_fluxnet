#!/discover/nobackup/jframe/anaconda3/bin/python
import numpy as np
import pandas as pd
import matplotlib.pyplot as plt

d = []

with open('30_day_summary.txt', 'r') as f:
    df = pd.read_csv(f, delim_whitespace=True, header=None)
    df.columns = ['day_forecast', 'year','day','hour','sm', 'obs']


for i in range(30):
    df2 = df.loc[(df.day_forecast == i), ['sm', 'obs']]
    d.append(np.sqrt(np.mean(np.square(df2.sm - df2.obs))))

plt.plot(d)
plt.show()
