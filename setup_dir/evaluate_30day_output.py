#!/discover/nobackup/jframe/anaconda3/bin/python
import numpy as np
import pandas as pd
import matplotlib.pyplot as plt

main_dir = '/discover/nobackup/jframe/gpr_fluxnet/soil_moisture/'
da_dir = main_dir + 'da_dirs/1_2006/'
gpr_dir = main_dir + 'gpr-noah/test_1_2006/'

list_da = []
list_gp = []

with open(da_dir + '30_day_summary.txt', 'r') as f:
    df_da = pd.read_csv(f, delim_whitespace=True, header=None)
    df_da.columns = ['day_forecast', 'year','day','hour','sm', 'obs']
with open(gpr_dir + '30_day_summary.txt', 'r') as f:
    df_gp = pd.read_csv(f, delim_whitespace=True, header=None)
    df_gp.columns = ['day_forecast', 'year','day','hour','sm', 'obs']

for i in range(30):
    df_da2 = df_da.loc[(df_da.day_forecast == i), ['sm', 'obs']]
    list_da.append(np.sqrt(np.mean(np.square(df_da2.sm - df_da2.obs))))
    df_gp2 = df_gp.loc[(df_gp.day_forecast == i), ['sm', 'obs']]
    list_gp.append(np.sqrt(np.mean(np.square(df_gp2.sm - df_gp2.obs))))

plt.plot(list_da, label='data assimilation')
plt.plot(list_gp, label='gp state update')
plt.legend()
plt.title('site 1, testing year 2006')
plt.xlabel('days ahead of observation')
plt.ylabel('RMSE')
plt.show()
