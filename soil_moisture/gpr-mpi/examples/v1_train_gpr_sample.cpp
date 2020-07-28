#include <stdio.h>
#include "gpr.h"
#include "options.h"

using namespace std;


#define print0 if(get_rank()==0) printf
int main(int argc, char** argv)
{
  /* begin parallel region */
  start_parallel(argc, argv);
  timer tm;

  /* set up and read in options */
  gaussian_process_configuation conf;
  char fn_gpr_state[256];
  int maxiter, isRead;
  double stepsize, tol_line, tol_gradient, signal_to_noise, signal_to_kernel, max_time;

  options opt;
  opt.add("gpr_state_filename", fn_gpr_state);
  opt.add("number_of_inputs", conf.dim_train);
  opt.add("number_of_parameters", conf.dimx);
  opt.add("number_pseudo_inputs", conf.num_pseudo_inputs);
  opt.add("input_filename", conf.training_inputs_filename);
  opt.add("target_filename", conf.training_targets_filename);
  opt.add("kernel_type", conf.kernel_type);
  opt.add("approx_type", conf.approximation_type);
  opt.add("max_number_of_iterations", maxiter);
  opt.add("stepsize", stepsize);
  opt.add("line_search_tolerance", tol_line);
  opt.add("gradient_tolerance", tol_gradient);
  opt.add("signal_to_noise", signal_to_noise);
  opt.add("signal_to_kernel", signal_to_kernel);
  opt.add("max_time", max_time);
  opt.add("isRead", isRead);
  opt.read_options(argc, argv);

  /* create gaussian process object to train */
  gaussian_process_regression gpr(conf);
  gpr.max_time = max_time;
  gpr.set_signal_to_noise(signal_to_noise);
  gpr.set_signal_to_kernel(signal_to_kernel);
  double hyperp[gpr.dimh];
  if(isRead)
  {
    print0("reading hyper parameters");
    if(get_rank() == 0)
    {
      FILE * fp = fopen("hyperp","r");
      fread(hyperp,sizeof(double),gpr.dimh,fp);
    }
    broadcast(hyperp,gpr.dimh);
    for(int i=0; i<gpr.dimh-1; i++) gpr.hyper_parameters[i] = hyperp[i];
  }

  /* train network by determing the MAP estimate */
  gpr.verbose = false;
  tm.start("maximizing marginal probability");
  gpr.maximize_marginal_probability(stepsize, maxiter, tol_line, tol_gradient);
  tm.stop();
  
  print0("Finished training GPR in %0.15e [s]\n", tm.time);
  gpr.write_gpr_state(fn_gpr_state);
 
 if(isRead == 0) 
 {
  if(get_rank() == 0)
  {
    FILE* fp = fopen("hyperp","w");
    fwrite(gpr.hyper_parameters,sizeof(double),gpr.dimh,fp);
    fclose(fp);
  }
 }

  /* end parallel region */
  end_parallel();

  return 0;
}
