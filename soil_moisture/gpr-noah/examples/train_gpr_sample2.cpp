#include <stdio.h>
#include <float.h>
#include "gpr.h"
#include "options.h"
#include "sampling_utilities.h"

using namespace std;

double NMSQE(double* preds, double* true_vals, int num_samples)
{
  double msqe = 0;
  for(int i=0; i<num_samples; i++) 
  {
    double x = abs((preds[i]-true_vals[i])/true_vals[i]);
    msqe += x;
  }
  msqe /= num_samples;
  
  double min =  FLT_MAX;
  double max = -FLT_MAX;
  for(int i=0; i<num_samples; i++)
  {
     if(true_vals[i] < min) min = true_vals[i];
     if(true_vals[i] > max) max = true_vals[i];
  }
  return msqe;
}

#define print0 if(get_rank()==0) printf
int main(int argc, char** argv)
{
  /* begin parallel region */
  start_parallel(argc, argv);

  /* set up and read in options */
  timer tm;
  char fn_gpr_state[256];
  int  num_inputs;
  int  num_parameters;
  char fn_training_inputs[256];
  char fn_training_targets[256];
  char kernel_type[256];
  int maxiter,max_increments,start_increment,isUsePreviousHyperp;
  double training_sample_increment,signal_to_noise;
  double stepsize, tol_line, tol_gradient, fraction_training, fraction_validation, fraction_test, nmsqe;

  options opt;
  opt.add("gpr_state_filename", fn_gpr_state);
  opt.add("number_of_inputs", num_inputs);
  opt.add("number_of_parameters", num_parameters);
  opt.add("input_filename", fn_training_inputs);
  opt.add("target_filename", fn_training_targets);
  opt.add("kernel_type", kernel_type);
  opt.add("max_number_of_iterations", maxiter);
  opt.add("stepsize", stepsize);
  opt.add("line_search_tolerance", tol_line);
  opt.add("gradient_tolerance", tol_gradient);
  opt.add("fraction_training", fraction_training);
  opt.add("fraction_validation", fraction_validation);
  opt.add("fraction_test", fraction_test);
  opt.add("training_sample_increment", training_sample_increment);
  opt.add("start_increment", start_increment);
  opt.add("max_increments", max_increments);
  opt.add("signal_to_noise", signal_to_noise);
  opt.add("isUsePreviousHyperp", isUsePreviousHyperp);
  opt.read_options(argc, argv);
 
  /* read in and all data */
  double all_inputs[num_inputs*num_parameters];
  double all_targets[num_inputs];
  double all_predictions[num_inputs];
  if(get_rank()==0)
  {
     FILE *fp = NULL;
     int error = 1;
     fp = fopen(fn_training_inputs,"r");
     if(fp==NULL) graceful_exit(error, "Could not open training input file");
     fread(all_inputs,sizeof(double),num_inputs*num_parameters,fp);
     fp = NULL;
     fp = fopen(fn_training_targets,"r");
     if(fp==NULL) graceful_exit(error, "Could not open training target file");
     fread(all_targets,sizeof(double),num_inputs,fp);
  }

  /* separate training, validation, and test */
  int num_valid_samples = fraction_validation*num_inputs;
  int num_test_samples  = fraction_test*num_inputs;
  int num_train_samples = fraction_training*num_inputs;

  if(get_rank()==0) {
    printf("Training   samples %i\n",num_train_samples);
    printf("Validation samples %i\n",num_valid_samples);
    printf("Test       samples %i\n\n",num_test_samples);
  }

  /* create and fill buffer for selection process */
  double all_train_inputs[num_inputs*num_parameters];
  double all_train_targets[num_inputs];
  for(int i=0; i<num_inputs; i++)
  {
    all_train_targets[i] = all_targets[i];
    for(int j=0; j<num_parameters; j++) 
      all_train_inputs[j+num_parameters*i]= all_inputs[j+num_parameters*i];
  }

  double train_inputs[num_train_samples*num_parameters];
  double train_targets[num_inputs];
  double train_predictions[num_inputs];

  double valid_inputs[num_valid_samples*num_parameters];
  double valid_targets[num_valid_samples];
  double valid_predictions[num_valid_samples];

  //double test_inputs[num_test_samples*num_parameters];
  //double test_targets[num_test_samples];
  //double test_predictions[num_test_samples];

  /* create sets */
  if(get_rank()==0) {
    spaceFillingSample(all_train_inputs,all_train_targets,valid_inputs,valid_targets,num_parameters, num_inputs, num_valid_samples);
//    spaceFillingSample(all_train_inputs,all_train_targets,train_inputs,train_targets,num_parameters, num_inputs, num_train_samples);
    //spaceFillingSample(all_train_inputs,all_train_targets,test_inputs,test_targets,num_parameters,num_inputs-num_valid_samples,num_test_samples);
  }

  char name[265];
  double hyperp[num_parameters+1];
  //int increments = round(1.0/training_sample_increment);
  for(int i=start_increment; i<max_increments; i++)
  {
    int fraction = training_sample_increment*(i+1)*num_train_samples;
    if(get_rank() == 0) {
      spaceFillingSample(all_train_inputs,all_train_targets,train_inputs,train_targets,num_parameters,num_inputs-num_valid_samples,fraction,-1,false,true);
    }
    
    /* create gaussian process object to train */
    gaussian_process_regression gpr(num_parameters, fraction, kernel_type, train_inputs, train_targets, "default");
    gpr.signal_to_noise = signal_to_noise;
    gpr.initialize_hyperparameters();

    if(i>start_increment) {
      if(isUsePreviousHyperp)
        for(int i=0; i<gpr.dimh; i++) gpr.hyper_parameters[i] = hyperp[i];
    }
    
    /* train network by determing the MAP estimate */
    tm.start("Maximizing marginal probability");
    gpr.maximize_marginal_probability(stepsize, maxiter, tol_line, tol_gradient);
    tm.stop();

    /* grab hyper parameters */
    for(int i=0; i<gpr.dimh; i++) hyperp[i] = gpr.hyper_parameters[i];

    /* write gpr state */
    sprintf(name,"%04i_%s",fraction,fn_gpr_state);
    gpr.write_gpr_state(name);

    /* write results */
    gpr.make_prediction(train_inputs,fraction,train_predictions);
    if(get_rank()==0) 
    {
      FILE* fp = NULL;
      sprintf(name,"%04i_train_predictions.bin", fraction);
      fp = fopen(name, "w");
      fwrite(train_targets,sizeof(double),fraction,fp);
      fwrite(train_predictions,sizeof(double),fraction,fp);
      fclose(fp);
    }

#if 0
    gpr.make_prediction(test_inputs, num_test_samples,test_predictions);
    if(get_rank()==0) 
    {
      FILE* fp = NULL;
      sprintf(name,"%04i_test_predictions.bin", num_test_samples);
      fp = fopen(name, "w");
      fwrite(test_targets,sizeof(double),num_test_samples,fp);
      fwrite(test_predictions,sizeof(double),num_test_samples,fp);
      fclose(fp);
    }
#endif

    gpr.make_prediction(valid_inputs, num_valid_samples,valid_predictions);
    nmsqe = NMSQE(valid_predictions, valid_targets, num_valid_samples);
    if(get_rank()==0) printf("\nValidation NMSQE = %0.15e\n", nmsqe);
    if(get_rank()==0) 
    {
      FILE* fp = NULL;
      sprintf(name,"%04i_valid_predictions.bin", fraction);
      fp = fopen(name, "w");
      fwrite(valid_targets,sizeof(double),num_valid_samples,fp);
      fwrite(valid_predictions,sizeof(double),num_valid_samples,fp);
      fclose(fp);
    }
    
    gpr.make_prediction(all_inputs, num_inputs,all_predictions);
    nmsqe = NMSQE(all_predictions, all_targets, num_inputs);
    if(get_rank()==0) printf("\nEntire NMSQE = %0.15e\n", nmsqe);
    if(get_rank()==0) 
    {
      FILE* fp = NULL;
      sprintf(name,"%04i_all_predictions.bin", fraction);
      fp = fopen(name, "w");
      fwrite(all_targets,sizeof(double),num_inputs,fp);
      fwrite(all_predictions,sizeof(double),num_inputs,fp);
      fclose(fp);
    }
  }

  /* end parallel region */
  end_parallel();

  return 0;
}
