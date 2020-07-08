#!/bin/bash -x
maindir='/discover/nobackup/jframe/gpr_fluxnet/'
datadir=${maindir}'data/pals/site_data/'
noahtestdir=${maindir}'soil_moisture/gpr-noah/test_'
for siteyear in 1_2004 1_2005 1_2006 2_2000 2_2001 2_2002 2_2003 2_2004 2_2005 2_2006 5_1999 5_2000 5_2002 5_2005 7_2004 7_2005 11_2002 11_2003 11_2004 11_2005 13_2002 13_2003 14_2003 14_2004 14_2005 14_2006 18_2002 18_2003 18_2004 18_2005 
do
cd ${noahtestdir}${siteyear}
./noah_mp.exe 1 0 0 0
./noah_mp.exe 2 0 0 0

done
