#!/discover/nobackup/jframe/anaconda3/bin/python

# Cycle through the calibration folders and remove unnessessary files. 
# Hint: most files will be removed. 

import os

main_dir = os.path.dirname(os.path.realpath(__file__))

#cmd = '/bin/rm -r ' + main_dir + 'reports'
#os.system(cmd)

for subdir, dirs, files in os.walk(main_dir):
    for dir in dirs:
        if os.path.exists(main_dir + '/' + dir + '/' + 'reports'):
            cmd = '/bin/rm -r ' + main_dir + '/' + dir + '/' + 'reports'
            os.system(cmd)
        if os.path.exists(main_dir + '/' + dir + '/' + 'proc0'):
            cmd = '/bin/rm -r ' + main_dir + '/' + dir + '/' + 'proc0'
            os.system(cmd)
        if os.path.exists(main_dir + '/' + dir + '/' + 'OstModel0.txt'):
            cmd = '/bin/rm ' + main_dir + '/' + dir + '/' + 'OstModel0.txt'
            os.system(cmd)
        if os.path.exists(main_dir + '/' + dir + '/' + 'OstErrors0.txt'):
            cmd = '/bin/rm ' + main_dir + '/' + dir + '/' + 'OstErrors0.txt'
            os.system(cmd)
        if os.path.exists(main_dir + '/' + dir + '/' + 'OstOutput0.txt'):
            cmd = '/bin/rm ' + main_dir + '/' + dir + '/' + 'OstOutput0.txt'
            os.system(cmd)
        if os.path.exists(main_dir + '/' + dir + '/' + 'OstStatus0.txt'):
            cmd = '/bin/rm ' + main_dir + '/' + dir + '/' + 'OstStatus0.txt'
            os.system(cmd)
        if os.path.exists(main_dir + '/' + dir + '/' + 'output.out'):
            cmd = '/bin/rm ' + main_dir + '/' + dir + '/' + 'output.out'
            os.system(cmd)
