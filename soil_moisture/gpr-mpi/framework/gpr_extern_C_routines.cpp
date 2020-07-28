#include <stdio.h>
#include <iostream>
#include <string>
#include "gpr.h"
#include "timer.h"

extern "C"
{
  // Create the GPR
  //---------------
  gaussian_process_regression*  _gaussian_process_regression_1(
           int dimx, 
           int dim_train, 
           const char* kernel_type,
           const char* training_input_file, 
           const char* training_targets_file, 
           const char* preprocess_method)
  {
    gaussian_process_regression *gpr;
    gpr = new gaussian_process_regression(dimx,dim_train,kernel_type, training_input_file,training_targets_file, preprocess_method);
    return gpr;
  }

  gaussian_process_regression*  _gaussian_process_regression_2(
           int dimx, 
           int dim_train, 
           const char* kernel_type,
           double* training_input, 
           double* training_targets, 
           const char* preprocess_method)
  {
    gaussian_process_regression *gpr;
    gpr = new gaussian_process_regression(dimx, dim_train, kernel_type, training_input, training_targets, preprocess_method);
    return gpr;
  }

  gaussian_process_regression*  _gaussian_process_regression_3(const char* gpr_state_file)
  {
    gaussian_process_regression *gpr;
    gpr = new gaussian_process_regression(gpr_state_file);
    return gpr;
  }
    
  // Maximize the marginal probability
  // ----------------------------------
  double  _maximize_marginal_probability(
               gaussian_process_regression* gpr, 
               double stepsize, 
               int maxiter, 
               double tol, 
               double tol_gradient)
  {
    // printf("%lf %i %lf %lf\n",stepsize,maxiter,tol,tol_gradient);
    return  gpr->maximize_marginal_probability(stepsize, maxiter, tol, tol_gradient);
  }

  // Make prediction
  // ---------------
  void _make_prediction_1(gaussian_process_regression* gpr,
                        double* input,
                        matrix<double>& predictive_mean,
                        matrix<double>& predictive_covariance,
                        bool isCovariance)
  {
     gpr->make_prediction(input, predictive_mean, predictive_covariance, isCovariance);
  }

  void _make_prediction_2(gaussian_process_regression* gpr,
                        const char* input_filename, 
                        int num_inputs, 
                        const char* target_filename, 
                        const char* target_covariance_filename)
  {
     gpr->make_prediction(input_filename, num_inputs, target_filename, target_covariance_filename);
  }

  void _make_prediction_3(gaussian_process_regression* gpr,
                        double* pred_inputs,
                        int num_inputs,
                        double* prediction_targets)
  {
     gpr->make_prediction(pred_inputs, num_inputs, prediction_targets);
  }

  // Set Parameters
  // --------------
  void _set_signal_to_noise(gaussian_process_regression* gpr, double signal_to_noise)
  {
      gpr->set_signal_to_noise(signal_to_noise);
  }

  void _set_kernel_nugget(gaussian_process_regression* gpr, double kernel_nugget)
  {
      gpr->kernel_nugget = kernel_nugget;
  }

  void _set_signal_to_kernel(gaussian_process_regression* gpr, double signal_to_kernel)
  {
      gpr->set_signal_to_kernel(signal_to_kernel);
  }

  void _write_gpr_state(gaussian_process_regression* gpr, const char* filename)
  {
      gpr->write_gpr_state(filename);
  }

  void _delete(gaussian_process_regression* gpr)
  {
     delete gpr;
  }

  void _print(gaussian_process_regression* gpr)
  {
      printf("dimh %i\n",gpr->dimh);
  }

  // Get the processor rank
  // ----------------------
  int get_rank_()
  {
      int r = get_rank();
      return r;
  }

}
