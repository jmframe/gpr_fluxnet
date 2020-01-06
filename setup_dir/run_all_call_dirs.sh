#!/bin/bash -x
for d in /discover/nobackup/jframe/gpr_fluxnet/soil_moisture/cal_dirs/*/ ; do
    cd ${d}
    pwd
    ./noah_mp.exe
done
