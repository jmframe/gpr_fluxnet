#!/bin/bash -x
savehere='/discover/nobackup/jframe/gpr_fluxnet/z_old/results_2019november13'
mkdir ${savehere}
for siteyear in 1_2004 1_2005 1_2006 2_2000 2_2001 2_2002 2_2003 2_2004 2_2005 2_2006 5_1999 5_2000 5_2002 5_2005 7_2004 7_2005 11_2002 11_2003 11_2004 11_2005 13_2002 13_2003 14_2003 14_2004 14_2005 14_2006 18_2002 18_2003 18_2004 18_2005 
do
cp ${siteyear}/rmse_dry.txt ${savehere}/${siteyear}_rmse_dry.txt
cp ${siteyear}/rmse_wet.txt ${savehere}/${siteyear}_rmse_wet.txt
cp ${siteyear}/slurm.out ${savehere}/${siteyear}_slurm.out
done
