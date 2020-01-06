#!/bin/bash -x
# This program is part of the initialization process. 
# To initialize we need to 
# 1. Run the calibrated parameters
# 2. Equilibrate (with the calibrated parameters)
#    i.e., burning in the initial states on a noah-mp run with calibrated parameters
# 3. Run the one step simulation
# 4. Run the perturbation simulations for Data Assimilation
# 5. Run the full data Assimilation

S=$(cat "site.txt")
echo 'Initializing site: '${S}

# to check for convergence, introduce sm_old
sm_old=(0 0 0 0)
diff=0

# Print the initial States
echo 'initial states for soil moisture and biomass'
cat "soil_init.orig"
cat "plant_init.orig"

# Copy the initial states
cp soil_init.orig soil_init.txt
cp plant_init.orig plant_init.txt

# copy over the observation data, because we'll transform it for DA
cp obs.orig obs.txt


IFS=$'\n' read -d '' -r -a sm1 < soil_init.txt
IFS=$'\n' read -d '' -r -a pl1 < plant_init.txt
# Loop through the number of times to do the burn-in, 
# We are doing 10+1 times for each year.
for irun in `seq 1 1 11`
do

echo 'for site '${S}' This is burn-in run '${irun}
./noah_mp_basic.exe

# Collect initial state
line=$(tail -n 1 output.out)
read -a arr <<< $line
sm=${arr[@]:10:4}
IFS=', ' read -r -a sm <<< "$sm"
pl=${arr[@]:14:4}
IFS=', ' read -r -a pl <<< "$pl"
printf "%s\n" "${sm[@]}" > soil_init.txt
printf "%s\n" "${pl[@]}" > plant_init.txt

for ism in "${!sm[@]}"
do
  diff=$(echo "($diff + ${sm[$ism]} - ${sm_old[$ism]})" | bc)
  echo $diff
done

if (( $(echo "$diff > 0" |bc -l) ))
then
  echo 'Continuing on with the equilibration'
else
  echo 'the soil moisture states have converged'
  break
fi

diff=0
sm_old=${sm[@]}
IFS=', ' read -r -a sm_old <<< "$sm_old"
# End loop
done

echo 'the soil moisture initial states changed from:'
echo ${sm1[@]}
echo 'to:'
echo ${sm[@]}

# Copy output from noah only model
cp output.out output.noah

#END
