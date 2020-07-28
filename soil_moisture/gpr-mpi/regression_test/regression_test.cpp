#include <stdio.h>
#include "gpr.h"
#include "timer.h"

#define PI 3.141592653589793
int main(int argc, char** argv)
{
  start_parallel(argc,argv);
  timer tm;
  gaussian_process_configuation conf;
  double stepsize     = 0.0001;
  int maxiter         = 1000;
  double tol_line     = 0.00;
  double tol_gradient = 1e-5;
  char fn_training_inputs_1D[256]  = "./regression_test/training_inputs_1D.bin";
  char fn_training_targets_1D[256] = "./regression_test/training_targets_1D.bin";
  char fn_training_inputs_2D[256]  = "./regression_test/training_inputs_2D.bin";
  char fn_training_targets_2D[256] = "./regression_test/training_targets_2D.bin";
  int num = 20;
  int num_samples = 20;
  double range = num_samples*2.0*PI/20.0;

  /* create training/target/prediction files for xSin(x)*/
  double* predInputs;
  double* predTargets;
  double* predTrueValue;
  predInputs = new double[2*num*num];
  predTargets = new double[num*num];
  predTrueValue = new double[num*num];
  for(int i=0; i<num; i++) 
  {
    double x = range/num*i;
    predInputs[i] = x;
    predTrueValue[i] = x*sin(x); 
  }

  conf.dimx = 1;
  conf.dim_train = num_samples;
  strcpy(conf.training_inputs_filename,fn_training_inputs_1D);
  strcpy(conf.training_targets_filename,fn_training_targets_1D);
  strcpy(conf.kernel_type,"ARD_without_noise");
  gaussian_process_regression gpr(conf);
  tm.start("Regression xSin(x)");
  gpr.set_signal_to_kernel(1e+5);
  gpr.maximize_marginal_probability(stepsize, maxiter, tol_line, tol_gradient);
  tm.stop();

  /* test the read and write utilities */
  gpr.write_gpr_state("gpr_state_1D.dat");
  gaussian_process_regression gprp("gpr_state_1D.dat");

  tm.start("Prediction");
  gprp.make_prediction(predInputs,num,predTargets);
  tm.stop();

  double msqe = 0;
  for(int i=0; i<num; i++) 
  {
    double x = predTargets[i]-predTrueValue[i];
    msqe += x*x;
  }
  msqe /= num;

  if(get_rank()==0) 
  {
    printf("MSQE %0.15e\n", msqe);
    FILE *fp=fopen("prediction_targets_1D.bin","w");
    fwrite(predTargets,sizeof(double),num,fp);
    fclose(fp);
  }
  strcpy(conf.kernel_type,"ARD_with_noise");
  gaussian_process_regression gprn(conf);
  gprn.set_signal_to_noise(1e+10);
  gprn.set_signal_to_kernel(1e+4);
 
  tm.start("Regression xSin(x)");
  gprn.maximize_marginal_probability(stepsize, maxiter, tol_line, tol_gradient);
  tm.stop();

  tm.start("Prediction");
  gprn.make_prediction(predInputs,num,predTargets);
  tm.stop();

  msqe = 0;
  for(int i=0; i<num; i++) 
  {
    double x = predTargets[i]-predTrueValue[i];
    msqe += x*x;
  }
  msqe /= num;

  if(get_rank()==0) 
  {
    printf("MSQE %0.15e\n", msqe);
    FILE *fp=fopen("prediction_targets_1D.bin","w");
    fwrite(predTargets,sizeof(double),num,fp);
    fclose(fp);
  }

  for(int j=0; j<num; j++) 
    for(int i=0; i<num; i++) 
    {
      double x = range/num*i;
      double y = range/num*j;
      predInputs[0+2*i+2*num*j] = x;
      predInputs[1+2*i+2*num*j] = y;
      predTrueValue[i+num*j] = x*sin(x) + y*sin(y);
    }

  conf.dimx = 2;
  conf.dim_train = num_samples*num_samples;
  strcpy(conf.training_inputs_filename,fn_training_inputs_2D);
  strcpy(conf.training_targets_filename,fn_training_targets_2D);
  strcpy(conf.kernel_type,"ARD_without_noise");
  gaussian_process_regression gpr2D(conf);
  gpr2D.set_signal_to_kernel(1e+4);
  tm.start("Regression xSin(x)+ySin(y)");
  gpr2D.maximize_marginal_probability(stepsize, maxiter, tol_line, tol_gradient);
  tm.stop();

  tm.start("Prediction");
  gpr2D.make_prediction(predInputs,num*num,predTargets);
  tm.stop();

  msqe = 0;
  for(int i=0; i<num*num; i++) 
  {
    double x = predTargets[i]-predTrueValue[i];
    msqe += x*x;
  }
  msqe /= (num*num);

  if(get_rank()==0) 
  {
    printf("MSQE %0.15e\n", msqe);
    FILE *fp=fopen("prediction_targets_2D.bin","w");
    fwrite(predTargets,sizeof(double),num*num,fp);
    fclose(fp);
  }
  
  strcpy(conf.kernel_type,"ARD_with_noise");
  gaussian_process_regression gpr2Dn(conf);
  gpr2Dn.set_signal_to_noise(1e+4);
  gpr2Dn.kernel_nugget = 1e-3;
  tm.start("Regression xSin(x)+ySin(y)");
  gpr2Dn.maximize_marginal_probability(stepsize, maxiter, tol_line, tol_gradient);
  tm.stop();

  tm.start("Prediction");
  gpr2Dn.make_prediction(predInputs,num*num,predTargets);
  tm.stop();

  msqe = 0;
  for(int i=0; i<num*num; i++) 
  {
    double x = predTargets[i]-predTrueValue[i];
    msqe += x*x;
  }
  msqe /= (num*num);

  if(get_rank()==0) 
  {
    printf("MSQE %0.15e\n", msqe);
    FILE *fp=fopen("prediction_targets_2D.bin","w");
    fwrite(predTargets,sizeof(double),num*num,fp);
    fclose(fp);
  }

  delete predInputs;
  delete predTargets;
  delete predTrueValue;

  end_parallel();
  return 0;
}
