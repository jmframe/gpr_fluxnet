#include <stdio.h>
#include "gpr.h"
#include "options.h"

using namespace std;

#define print0 if(get_rank()==0) printf
int main(int argc, char** argv)
{
  /* begin parallel region */
  start_parallel(argc, argv);

  /* set up and read in options */
  char fn_gpr_state[256];
  int  num_inputs;
  char fn_pred_inputs[256];
  char fn_pred_targets[256];
  char fn_pred_covariance[256];
  int isCovariance;

  options opt;
  opt.add("gpr_state_filename", fn_gpr_state);
  opt.add("number_of_inputs", num_inputs);
  opt.add("input_filename", fn_pred_inputs);
  opt.add("predictions_filename", fn_pred_targets);
  opt.add("target_covariance_filename", fn_pred_covariance);
  opt.add("isCovariance", isCovariance);
  opt.read_options(argc, argv);

  /* create gaussian processes regression object from saved state*/
  gaussian_process_regression gpr(fn_gpr_state);
  
  /* make prediction -- inputs are binary files with row-major format with x-dim columns and num_sample rows*/
  timer tm;
  tm.start("Computing predictions");
  tm.quiet();
  (isCovariance == 0) ?
  gpr.make_prediction(fn_pred_inputs, num_inputs, fn_pred_targets):
  gpr.make_prediction(fn_pred_inputs, num_inputs, fn_pred_targets, fn_pred_covariance);
  tm.stop();
  
  if(get_rank() == 0) printf("Time to compute predictions = %0.15e\n", tm.time);
  
  /* end parallel region */
  end_parallel();

  return 0;
}
