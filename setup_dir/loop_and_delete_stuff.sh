#!/bin/bash -x
maindir='/discover/nobackup/jframe/gpr_fluxnet/'
datadir=${maindir}'data/pals/site_data/'
noahtestdir=${maindir}'soil_moisture/gpr-noah/test_'
noahtraindir=${maindir}'soil_moisture/gpr-noah/train_'
for siteyear in 2_2000 2_2001 2_2002 2_2003 2_2004 2_2005 2_2006 21_2009 21_2010 21_2011 21_2012 21_2013 21_2014 21_2015 21_2016 21_2017 21_2018 
do
cd ${noahtraindir}${siteyear}
rm init_flag.txt
rm initialize_gpr_noah.sh

done
