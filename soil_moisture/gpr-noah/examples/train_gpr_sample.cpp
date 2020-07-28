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
  gaussian_process_regression* gpr;
  if(isRead) 
  {
    gpr = new gaussian_process_regression(fn_gpr_state);
  }
  else
  {
    gpr = new gaussian_process_regression(conf);
    gpr->set_signal_to_noise(signal_to_noise);
    gpr->set_signal_to_kernel(signal_to_kernel);
  }
  gpr->max_time = max_time;
#if 0
  for(int k=0; k<gpr->dimh; k++)
  {
  for(int i=0; i<20; i++)
  {
     double z = -double(10) + i;
     double r = gpr->minimizer_transformation_x(z,k,false);
     double rr = gpr->minimizer_transformation_x(r,k,true);
     double df = gpr->minimizer_transformation_df(z,k);
     printf("%0.15e %0.15e %0.15e %0.15e %0.15e\n",z,r,rr,rr-z,df);

  }
  }
  exit(1);
#endif

  if(get_rank()==0)
  {
  FILE *fp = fopen("train","w");
  fwrite(gpr->train_matrix,sizeof(double),conf.dim_train,fp);
  }
  
  char fn[256];
  sprintf(fn,"%02i_start",conf.num_pseudo_inputs);
  gpr->write_pseudo_inputs(fn);

  
  /* train network by determing the MAP estimate */
  gpr->verbose = false;
  tm.start("maximizing marginal probability");
  gpr->maximize_marginal_probability(stepsize, maxiter, tol_line, tol_gradient);
  tm.stop();
  
  sprintf(fn,"%02i_pseudo",conf.num_pseudo_inputs);
  gpr->write_pseudo_inputs(fn);

  gpr->output_vector->write("out");




  
  print0("Finished training GPR in %0.15e [s]\n", tm.time);
  gpr->write_gpr_state(fn_gpr_state);

  /* end parallel region */
  end_parallel();

  return 0;
}
