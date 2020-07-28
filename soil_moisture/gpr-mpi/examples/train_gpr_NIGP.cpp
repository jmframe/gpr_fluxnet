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
  char fn_nigp_gpr_state[256];
  int  num_inputs, num_refinements;
  int  num_parameters;
  char fn_training_inputs[256];
  char fn_training_targets[256];
  char kernel_type[256];
  int maxiter, isRead, isReadNIGP;
  double stepsize, tol_line, tol_gradient, signal_to_noise, signal_to_kernel, max_time,h2a;

  options opt;
  opt.add("gpr_state_filename", fn_gpr_state);
  opt.add("nigp_state_filename", fn_nigp_gpr_state);
  opt.add("number_of_inputs", num_inputs);
  opt.add("number_of_parameters", num_parameters);
  opt.add("input_filename", fn_training_inputs);
  opt.add("target_filename", fn_training_targets);
  opt.add("kernel_type", kernel_type);
  opt.add("max_number_of_iterations", maxiter);
  opt.add("stepsize", stepsize);
  opt.add("line_search_tolerance", tol_line);
  opt.add("gradient_tolerance", tol_gradient);
  opt.add("signal_to_noise", signal_to_noise);
  opt.add("signal_to_kernel", signal_to_kernel);
  opt.add("max_time", max_time);
  opt.add("isRead", isRead);
  opt.add("isReadNIGP", isReadNIGP);
  opt.add("num_refinements", num_refinements);
  opt.add("h2a", h2a);
  opt.read_options(argc, argv);
  
  timer tm;
  /* create gaussian process object to train */
  gaussian_process_regression *gpr;
  if(isRead) 
  {
    tm.start("allocation");
    gpr = new gaussian_process_regression(fn_gpr_state);
    tm.stop();
    exit(1);
  }
  else
  {
    gpr = new gaussian_process_regression(num_parameters, num_inputs, kernel_type, fn_training_inputs, fn_training_targets, "default");
    gpr->set_signal_to_noise(signal_to_noise);
    gpr->set_signal_to_kernel(signal_to_kernel);
    gpr->max_time = max_time;

    /* train network by determing the MAP estimate */
    tm.start("maximizing marginal probability", "blue");
    gpr->maximize_marginal_probability(stepsize, maxiter, tol_line, tol_gradient);
    tm.stop();
    gpr->write_gpr_state(fn_gpr_state);
  }
 
  gaussian_process_regression* nigp;
  if(isReadNIGP)
  {
    nigp = new gaussian_process_regression(fn_nigp_gpr_state);
  }
  else
  {
    nigp = new gaussian_process_regression(num_parameters, num_inputs, "ARD_NIGP", fn_training_inputs, fn_training_targets, "default");
    for(int i=0; i<gpr->dimh; i++) nigp->hyper_parameters[i] = gpr->hyper_parameters[i];

    nigp->update_df_NIGP();
    nigp->compute_avg_grad_NIGP();
    for(int i=gpr->dimh; i<nigp->dimh; i++) 
      nigp->hyper_parameters[i] = h2a/nigp->avg_grad[i-gpr->dimh];
  }
 
  for(int i=0; i<num_refinements; i++)
  {
    nigp->update_df_NIGP();
    tm.start("maximizing NIGP marginal probability", "blue");
    nigp->maximize_marginal_probability(stepsize, maxiter, tol_line, tol_gradient);
    tm.stop();
    print0("Finished training NIGP GPR in %0.15e [s]\n", tm.time);
  }
  print0("Finished training GPR in %0.15e [s]\n", tm.time);
  nigp->write_gpr_state(fn_nigp_gpr_state);

  delete gpr; 
  delete nigp; 
  /* end parallel region */
  end_parallel();

  return 0;
}
