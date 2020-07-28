#include <stdio.h>
#include <float.h>
#include <algorithm>
#include <set>
#include <random>
#include <iostream>
#include <fstream>
#include "gpr.h"
#include "options.h"
#include "sampling_utilities.h"

using namespace std;

double absoluteRelativeError(double* preds, double* true_vals, int num_samples)
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
  char fn_training_inputs[256];
  char fn_training_targets[256];
  char fn_summary_statistics[256];
  int  num_inputs;
  int  num_parameters;
  char kernel_type[256];
  int maxiter,begin_increment,end_increment,isUsePreviousHyperp,nKfolds, isWriteGPRState;
  double training_sample_increment,signal_to_noise;
  double stepsize, tol_line, tol_gradient, nmsqe;

  options opt;
  opt.add("gpr_state_filename", fn_gpr_state);
  opt.add("number_of_samples", num_inputs);
  opt.add("number_of_parameters", num_parameters);
  opt.add("input_filename", fn_training_inputs);
  opt.add("target_filename", fn_training_targets);
  opt.add("summary_filename", fn_summary_statistics);
  opt.add("kernel_type", kernel_type);
  opt.add("max_number_of_iterations", maxiter);
  opt.add("stepsize", stepsize);
  opt.add("line_search_tolerance", tol_line);
  opt.add("gradient_tolerance", tol_gradient);
  opt.add("num_kfolds", nKfolds);
  opt.add("training_sample_increment", training_sample_increment);
  opt.add("begin_increment", begin_increment);
  opt.add("end_increment",  end_increment);
  opt.add("signal_to_noise", signal_to_noise);
  opt.add("isUsePreviousHyperp", isUsePreviousHyperp);
  opt.add("isWriteGPRState", isWriteGPRState);
  opt.read_options(argc, argv);

  /* start increments from 1 */
  int max_increment = std::round(1.0/training_sample_increment);
  if(begin_increment < 0 ) begin_increment = 1;
  if(end_increment < 0 )   end_increment = max_increment;
  begin_increment -= 1;
 
  /* create buffers for training data*/
  double all_inputs[num_inputs*num_parameters];
  double all_targets[num_inputs];
  
  double train_inputs[num_inputs*num_parameters];
  double train_targets[num_inputs];
  
  double stratified_train_inputs[num_inputs*num_parameters];
  double stratified_train_targets[num_inputs];
  double stratified_train_predictions[num_inputs];

  double valid_inputs[num_inputs*num_parameters];
  double valid_targets[num_inputs];
  double valid_predictions[num_inputs];
  
  /* create buffers for summary statistics*/
  int nInc = end_increment - begin_increment;
  double train_error[nKfolds*nInc];
  double valid_error[nKfolds*nInc];
  int stratSampleSize[nInc];

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

  /* get kfold validation sizes --- last one may be larger as it includes the remainder */
  int sizeKfolds[nKfolds];
  for(int i=0; i<nKfolds; i++)
  {
    (i==nKfolds-1) ? sizeKfolds[i] = num_inputs/nKfolds + num_inputs % nKfolds : sizeKfolds[i] = num_inputs/nKfolds;
  }

  /* create index set and random shuffle indices */
  std::vector<int> indices; 
  for(int i=0; i<num_inputs; i++) indices.push_back(i);
  std::vector<int> shuff(indices.begin(), indices.end());
  std::shuffle(shuff.begin(), shuff.end(), default_random_engine(1));
   
  int offset = 0;
  char name[512];
  double hyperp[num_parameters+2];
  for(int k=0; k<nKfolds; k++)
  {
     /* get validation indices and complement */
     std::vector<int> kset(shuff.begin()+offset,shuff.begin()+offset+sizeKfolds[k]);
     std::sort(kset.begin(), kset.end());
     std::vector<int> complement;
     std::set_difference(indices.begin(),indices.end(),kset.begin(),kset.end(), std::inserter(complement, complement.begin()));
     offset += sizeKfolds[k];
    int num_valid_samples = kset.size();
    int nTrainSamples = complement.size();
    if(get_rank() == 0)
       printf("Training k-fold %02i valid samples = %i train samples = %i\n",k,num_valid_samples,nTrainSamples);

    /* pull validation set */
    for(int i=0; i<num_valid_samples; i++)
    for(int j=0; j<num_parameters; j++)
      valid_inputs[j+num_parameters*i] = all_inputs[j+kset[i]*num_parameters];
    for(int i=0; i<num_valid_samples; i++) valid_targets[i] = all_targets[kset[i]];
    
    /* pull training set */
    for(int i=0; i<nTrainSamples; i++)
    for(int j=0; j<num_parameters; j++)
      train_inputs[j+num_parameters*i] = all_inputs[j+complement[i]*num_parameters];
    for(int i=0; i<nTrainSamples; i++) train_targets[i] = all_targets[complement[i]];
      
    for(int i=begin_increment; i<end_increment; i++)
    {
      /* determine train size for this increment and pull stratified sample -- do not overwrite*/
      int nStratSamples = std::round(training_sample_increment*(i+1)*nTrainSamples);
      stratSampleSize[i] = nStratSamples;
      if(get_rank() == 0) {
        spaceFillingSample(train_inputs,train_targets,stratified_train_inputs,stratified_train_targets,num_parameters,nTrainSamples,nStratSamples,-1,false,true);
      }

      /* create gaussian process object to train */
      gaussian_process_regression gpr(num_parameters, nStratSamples, kernel_type, stratified_train_inputs, stratified_train_targets, "default");
      gpr.signal_to_noise = signal_to_noise;
      gpr.initialize_hyperparameters();
      gpr.verbose = true;

      /* use previous hyper-parameter results to initialize if specified */
      if(i>begin_increment) {
        if(isUsePreviousHyperp)
          for(int i=0; i<gpr.dimh; i++) gpr.hyper_parameters[i] = hyperp[i];
      }

      /* train network by determing the MAP estimate */
      tm.start("Maximizing marginal probability");
      gpr.maximize_marginal_probability(stepsize, maxiter, tol_line, tol_gradient);
      tm.stop();

      /* grab hyper parameters */
      for(int j=0; j<gpr.dimh; ++j) hyperp[j] = gpr.hyper_parameters[j];

      /* write gpr state */
      if(isWriteGPRState)
      {
        sprintf(name,"kfold_%02i_nTrainSamples_%04i_%s",k,nStratSamples,fn_gpr_state);
        gpr.write_gpr_state(name);
      }

      /* write results */
      gpr.make_prediction(stratified_train_inputs,nStratSamples,stratified_train_predictions);
      nmsqe = absoluteRelativeError(stratified_train_predictions,stratified_train_targets,nStratSamples);
      train_error[k+nKfolds*(i-begin_increment)] = nmsqe;
      if(get_rank()==0) 
      {
        FILE* fp = NULL;
        sprintf(name,"kfold%02i_nTrainSamples%04i_train_predictions.bin",k,nStratSamples);
        fp = fopen(name, "w");
        fwrite(stratified_train_targets,sizeof(double),nStratSamples,fp);
        fwrite(stratified_train_predictions,sizeof(double),nStratSamples,fp);
        fclose(fp);
      }

      gpr.make_prediction(valid_inputs,num_valid_samples,valid_predictions);
      nmsqe = absoluteRelativeError(valid_predictions, valid_targets, num_valid_samples);
      valid_error[k+nKfolds*(i-begin_increment)] = nmsqe;
      if(get_rank()==0) 
      {
        FILE* fp = NULL;
        sprintf(name,"kfold%02i_nTrainSamples%04i_valid_predictions.bin",k,nStratSamples);
        fp = fopen(name, "w");
        fwrite(valid_targets,sizeof(double),num_valid_samples,fp);
        fwrite(valid_predictions,sizeof(double),num_valid_samples,fp);
        fclose(fp);
      }
      
      if(get_rank()==0) 
      {
        FILE* fp = NULL;
        sprintf(name,"kfold%02i_nTrainSamples%04i_hyper_parameters.bin",k,nStratSamples);
        fp = fopen(name, "w");
        fwrite(hyperp,sizeof(double),gpr.dimh,fp);
        fclose(fp);
      }

    }
  }//end kfold

  /* print summary statistics */
  if(get_rank() == 0)
  {
    ofstream os(fn_summary_statistics,ios::trunc);
    os << "Absolute squared-error\n\n";
    os << std::left << std::setw(10) << "nSamples";
    char tmp[50];
    for(int k=0; k<nKfolds; k++)
    {
      sprintf(tmp,"k-fold %i",k);
      os << std::left <<  std::setprecision(4) << std::setw(10) << tmp;
    }
    os << std::setw(10) << "Average";
    os << std::endl;
    for(int k=0; k<60; k++) os << "-";
    os << endl; 
    os << "Training:" << "\n";

    for(int i=0; i<nInc; i++)
    {
      double avg = 0;
      os << std::setw(10) << stratSampleSize[i];
      for(int k=0; k<nKfolds; k++)
      {
        os  << std::scientific << std::setprecision(2) << std::setw(10) << train_error[k+nKfolds*i];
        avg += train_error[k+nKfolds*i];
      }
      os << std::scientific << std::setprecision(2) << std::setw(10) << avg/4 << std::endl;
    }
    
    os << std::endl;
    os << "Validation:" << "\n";
    for(int i=0; i<nInc; i++)
    {
      double avg = 0;
      os << std::setw(10) << stratSampleSize[i];
      for(int k=0; k<nKfolds; k++)
      {
        os << std::setw(10) << std::scientific << std::setprecision(2) << valid_error[k+nKfolds*i];
        avg += valid_error[k+nKfolds*i];
      }
      os << std::scientific << std::setprecision(2) << std::setw(10) << avg/4 << std::endl;
    }
    os.close();
  }

  /* end parallel region */
  end_parallel();

  return 0;
}
