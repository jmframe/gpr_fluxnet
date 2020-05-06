My (Jonathan Frame's) master copy of the noah-mp code.
Make major revisions to the code here. Copy this code if need 
to make minor changes to the options, but if you change anything 
substantial, do it hear and re-copy. I would prefer to link to this 
executable for experiments. Keep track of the version number, 
and keep an archive of code versions.


to do:
* The next major chanch will be to impliment Craig's GPR.
* replace the objfun.F90 with some better named head file
  then put all the stuff in the current objfun.F90 in 
  some real option modules.


version    Note
------------------------------------------------------------------
20190717   Implemented a one-step state upade option. To run the
One-step state update set the da_flag.txt to -1.
Added some logic so when the model is run 
withouot Data Assimilation it does not try to read in DA specific 
files for sig_sm, Nlag and obs_cov.txt.

20190716   I added some code to run the Ensemble Kalman Smoother, 
This was mostly implimented by Grey, but I made a few changes, 
like the perturbations to the forcing data.

