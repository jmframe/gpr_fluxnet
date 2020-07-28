#ifndef H_GAUSSIAN_PRCOESS_REGRESSION
#define H_GAUSSIAN_PRCOESS_REGRESSION
#include "layout.h"
#include <iostream>
#include <algorithm>
#include <random>
#include "scalapack_matrix.h"
#include "comm.h"
#include <math.h>
#include "timer.h"
#include "conjugate_gradient.h"
#include "utils.h"

/* standardized input for the gpr */

struct gaussian_process_configuation
{
  /* gpr configuration */
  int  input_type = 0;
  char kernel_type[256];
  char approximation_type[256] = "none";
  char training_inputs_filename[512];
  char training_targets_filename[512];
  char preprocess_method[256] = "default";
  double **training_inputs;
  double **training_targets;
  int dimx = -1;
  int dim_train = -1;
  int num_shuffles_spgp = 1;

  /* pseudo spars gpr */
  int num_pseudo_inputs = 1;
};

/* functions for gsl_conjugate gradient ... gsl specific functions shouldn't be here ... should restructure */
double gpr_gsl_function(const gsl_vector*x, void *params);
void   gpr_gsl_gradient(const gsl_vector *x, void *params, gsl_vector* df);
void   gpr_gsl_fdf(const gsl_vector* x, void* params, double *f, gsl_vector* df);

static long double Pi = 3.141592653589793;
class gaussian_process_regression
{
  public:
    int dimx;                          //number of parameters.
    int dim_train;                     //number of training samples.
    int dimh;                          //number of hyper-parameters.
    int num_pseudo_inputs = 1;         //number of pseudo inputs if using spgp
    layout ly;                         //layout of MPI grid.
    matrix<double>* output_vector;     //training targets.
    double *train_matrix;              //training matrix dim_train X dimx.
    vector **train_data;               //vector construct for training data.
    vector **df_NIGP;                  //vector containter for slope of posterior predictive mean for NIGP
    double *avg_grad;
    char kernel_type[256];             //GPR kernel specification.
    char approximation_type[256];      //GPR approximations right now only spgp coded
    double *hyper_parameters;          //hyper-parameters.
    double kernel_nugget = 0;          //constant diagonal matrix added to K(X',X) to avoid poor conditioning #.
    double signal_to_kernel = 1e+6;    //default signal_to_kernel ratio to use to set the kernel nugget
    double signal_to_noise  = 1e+1;    //set noise guess as var(noise) = signal_to_noise*var(f) (default %10).
    bool verbose = false;               //verbose print out.
    timer wallclock;                   //wall timer for gpr. Starts at allocation.
    double max_time  = -1;             //max time wall time. If < 0 there's no max;
    bool isNoise = true;               //to add noise targets or not
    bool isRegSPGP = false;             //switch to turn on Titsias 2009 regularization term
int num_shuffles_spgp = 1;

    /* precalculate quantities to avoid redundant calculations */
    matrix<double>* K_pre_calc         = NULL ;
    matrix<double>* Kinv_pre_calc      = NULL ;
    matrix<double>* Choleski_pre_calc  = NULL ;
    matrix<double>* beta_pre_calc      = NULL ;
    double log_determinant_pre_calc    = 0;

    /* pointers for SPGP quantities */
    double        *  gAKy        = NULL;
    matrix<double>*  KMM         = NULL;
    matrix<double>*  FKMM        = NULL;
    matrix<double>*  KMMInv      = NULL;
    matrix<double>*  KNM         = NULL;
    matrix<double>*  NM          = NULL;
    matrix<double>*  bKNM        = NULL;
    matrix<double>*  bKNMA       = NULL;
    matrix<double>*  A           = NULL;
    matrix<double>*  AInv        = NULL;
    matrix<double>*  ybar        = NULL; //Gamma^-1 y  the name is confusing
    matrix<double>*  ybbar       = NULL; //Gamma^-1/2 y
    matrix<double>*  KKy         = NULL;
    matrix<double>*  KKy1        = NULL;
    matrix<double>*  AKy         = NULL;
    matrix<double>*  KAKy        = NULL;
    matrix<double>*  KAKyybar    = NULL;
    matrix<double>*  KAK         = NULL;
    matrix<double>*  dotKMM      = NULL;
    matrix<double>*  dotKNM      = NULL;
    matrix<double>*  bdotKNM     = NULL;
    matrix<double>*  bdotKNN     = NULL;
    matrix<double>*  bKNMKMMInv  = NULL;
    matrix<double>*  bbKNMKMMInv = NULL;
    matrix<double>*  bKKK        = NULL;
    matrix<double>*  bbKKK       = NULL;
    matrix<double>*  bKKKy       = NULL;
    matrix<double>*  bKKKy1      = NULL;
    matrix<double>*  bKKKy2      = NULL;
    matrix<double>*  bKKKA       = NULL;
    matrix<double>*  MM          = NULL;
    matrix<double>*  MM1         = NULL;
    matrix<double>*  ytmp        = NULL;
    matrix<double>*  g           = NULL;
    matrix<double>*  gh          = NULL;

    /* data processing information */
    char preprocess_method[256];
    double* training_std_deviation  = NULL;
    double* training_sample_mean    = NULL;
    double training_targets_variance = 0;
    double training_targets_mean     = 0;
    double *xmin; //minimimum values in training set
    double *xmax; //maximimum values in training set

    void allocate_buffers()
    {
      /* allocate arrays */
      output_vector    = new matrix<double>(dim_train,1,ly);
      hyper_parameters = new double[dimh];
      train_matrix     = new double[dimx*dim_train];
      train_data       = new vector*[dim_train];
      for(int i=0; i<dim_train; i++)
        train_data[i]  = new vector(dimx); 
      df_NIGP          = new vector*[dim_train];
      avg_grad = new double[dimx];
      for(int i=0; i<dim_train; i++)
      {
        df_NIGP[i] = new vector(dimx);
        for(int k=0; k<dimx; k++)
        df_NIGP[i]->d[k] = 0.0;
      }
      
      training_std_deviation = new double[dimx];
      training_sample_mean   = new double[dimx];
      xmin                   = new double[dimx];
      xmax                   = new double[dimx];
  
      /* pre calculate quantities */
      if(strcmp(approximation_type,"spgp"))
      {
        K_pre_calc        = new matrix<double>(dim_train, dim_train, ly);
        Kinv_pre_calc     = new matrix<double>(dim_train, dim_train, ly);
        Choleski_pre_calc = new matrix<double>(dim_train, dim_train, ly);
        beta_pre_calc     = new matrix<double>(dim_train, 1, ly);
      }
      else
      {
        gAKy        = new double[num_pseudo_inputs];
        KMM         = new matrix<double>(num_pseudo_inputs,num_pseudo_inputs,ly);
        FKMM        = new matrix<double>(num_pseudo_inputs,num_pseudo_inputs,ly);
        KMMInv      = new matrix<double>(num_pseudo_inputs,num_pseudo_inputs,ly);
        KNM         = new matrix<double>(dim_train,num_pseudo_inputs,ly);
        NM          = new matrix<double>(dim_train,num_pseudo_inputs,ly);
        bKNM        = new matrix<double>(dim_train,num_pseudo_inputs,ly);
        bKNMA       = new matrix<double>(dim_train,num_pseudo_inputs,ly);
        A           = new matrix<double>(num_pseudo_inputs,num_pseudo_inputs,ly);
        AInv        = new matrix<double>(num_pseudo_inputs,num_pseudo_inputs,ly);
        ybar        = new matrix<double>(dim_train,1,ly);
        ybbar       = new matrix<double>(dim_train,1,ly);
        KKy         = new matrix<double>(dim_train,num_pseudo_inputs,ly);
        KKy1        = new matrix<double>(dim_train,num_pseudo_inputs,ly);
        AKy         = new matrix<double>(num_pseudo_inputs,1,ly);
        KAKy        = new matrix<double>(dim_train,1,ly);
        KAKyybar    = new matrix<double>(dim_train,1,ly);
        KAK         = new matrix<double>(dim_train,1,ly);
        dotKMM      = new matrix<double>(num_pseudo_inputs,num_pseudo_inputs,ly);
        dotKNM      = new matrix<double>(dim_train,num_pseudo_inputs,ly);
        bdotKNM     = new matrix<double>(dim_train,num_pseudo_inputs,ly);
        bdotKNN     = new matrix<double>(dim_train,1,ly);
        bKNMKMMInv  = new matrix<double>(dim_train,num_pseudo_inputs,ly);
        bbKNMKMMInv = new matrix<double>(dim_train,num_pseudo_inputs,ly);
        bKKK        = new matrix<double>(num_pseudo_inputs,num_pseudo_inputs,ly);
        bbKKK       = new matrix<double>(num_pseudo_inputs,num_pseudo_inputs,ly);
        bKKKy       = new matrix<double>(num_pseudo_inputs,num_pseudo_inputs,ly);
        bKKKy1      = new matrix<double>(num_pseudo_inputs,num_pseudo_inputs,ly);
        bKKKy2      = new matrix<double>(num_pseudo_inputs,num_pseudo_inputs,ly);
        bKKKA       = new matrix<double>(num_pseudo_inputs,num_pseudo_inputs,ly);
        MM          = new matrix<double>(num_pseudo_inputs,num_pseudo_inputs,ly);
        MM1         = new matrix<double>(num_pseudo_inputs,num_pseudo_inputs,ly);
        ytmp        = new matrix<double>(dim_train,1,ly);
        g           = new matrix<double>(dim_train,1,ly);
        gh          = new matrix<double>(dim_train,1,ly);
      }
    }

    gaussian_process_regression(gaussian_process_configuation& conf) :
      dimx(conf.dimx),
      dim_train(conf.dim_train),
      num_pseudo_inputs(conf.num_pseudo_inputs),
      num_shuffles_spgp(conf.num_shuffles_spgp),
      ly(32,32,2,1,false)
    {
      /* begin wall clock for gpr */
      wallclock.quiet();
      wallclock.start("start gpr timer");

      strcpy(this->kernel_type, conf.kernel_type);
      strcpy(this->preprocess_method, conf.preprocess_method);
      strcpy(this->approximation_type, conf.approximation_type);
      
      /* set layout correctly */
      if(!strcmp(approximation_type,"spgp"))
      {
        ly.ratio = double(dim_train)/num_pseudo_inputs;
        ly.layout_dimension = 1;
      }
      else
        ly.ratio = 1;
      ly.verbose = true;
      ly.init();

      if(!strcmp(preprocess_method,"default") && !strcmp(preprocess_method,"none")) exit(1);
      set_kernel(); 

      /* allocate memory */
      allocate_buffers();

      if(ly.rank==0)
      {
        printf("GPR configuration:\n");
        for(int i=0; i<50; i++) printf("-");
        printf("\n");
        printf("kernel = %s\n",kernel_type);
        printf("approximation = %s\n",approximation_type);
        printf("\n");
      }

      /* allocate for preprocessing coordinate transformation, read in data,
      which also applies the coordinate transformation and fills the training vectors */
      (conf.input_type == 0) ?
      read_training_set(conf.training_inputs_filename, conf.training_targets_filename) :
      read_training_set(*conf.training_inputs, *conf.training_targets);
      get_coordinate_ranges();
      initialize_hyperparameters(true);
    }

/****************************** these allocators are depracated ********************************************************/    
    /* allocate data from a file */ 
    gaussian_process_regression(int dimx, int dim_train, const char* kernel_type, 
        const char* training_input_file, const char* training_targets_file, const char* preprocess_method="default") : 
      dimx(dimx), 
      dim_train(dim_train),
      ly(32,32,2,double(dim_train)/num_pseudo_inputs)
    {
      /* begin wall clock for gpr */
      wallclock.quiet();
      wallclock.start("start gpr timer");

      strcpy(this->kernel_type, kernel_type);
      strcpy(this->preprocess_method, preprocess_method);
      if(!strcmp(preprocess_method,"default") && !strcmp(preprocess_method,"none")) exit(1);
      set_kernel(); 
      
      /* allocate arrays */
      output_vector = new matrix<double>(dim_train,1,ly);
      hyper_parameters = new double[dimh];
      train_matrix    = new double[dimx*dim_train];
      train_data = new vector*[dim_train];
      for(int i=0; i<dim_train; i++)
        train_data[i] = new vector(dimx); 
      df_NIGP = new vector*[dim_train];
      avg_grad = new double[dimx];
      for(int i=0; i<dim_train; i++)
      {
        df_NIGP[i] = new vector(dimx);
        for(int k=0; k<dimx; k++)
        df_NIGP[i]->d[k] = 0.0;
      }
  
      /* pre calculate quantities */
      K_pre_calc = new matrix<double>(dim_train, dim_train, ly);
      Kinv_pre_calc = new matrix<double>(dim_train, dim_train, ly);
      Choleski_pre_calc = new matrix<double>(dim_train, dim_train, ly);
      beta_pre_calc = new matrix<double>(dim_train, 1, ly);

      /* allocate for preprocessing coordinate transformation, read in data,
      which also applies the coordinate transformation and fills the training vectors */
      training_std_deviation = new double[dimx];
      training_sample_mean   = new double[dimx];
      read_training_set(training_input_file, training_targets_file);
      initialize_hyperparameters(true);
    }
   
    /* alloate from data poitners */ 
    gaussian_process_regression(int dimx, int dim_train, const char* kernel_type, 
        double* training_input, double* training_targets, const char* preprocess_method="default") : 
      dimx(dimx), 
      dim_train(dim_train),
      ly(32,32,2,double(dim_train)/num_pseudo_inputs)
    {
      strcpy(this->kernel_type, kernel_type);
      strcpy(this->preprocess_method, preprocess_method);
      if(!strcmp(preprocess_method,"default") && !strcmp(preprocess_method,"none")) exit(1);
      set_kernel(); 
      
      /* allocate arrays */
      output_vector = new matrix<double>(dim_train,1,ly);
      hyper_parameters = new double[dimh];
      train_matrix    = new double[dimx*dim_train];
      train_data = new vector*[dim_train];
      for(int i=0; i<dim_train; i++)
        train_data[i] = new vector(dimx); 
      df_NIGP = new vector*[dim_train];
      avg_grad = new double[dimx];
      for(int i=0; i<dim_train; i++)
      {
        df_NIGP[i] = new vector(dimx);
        for(int k=0; k<dimx; k++)
        df_NIGP[i]->d[k] = 0.0;
      }

      /* pre calculated quantities */
      K_pre_calc = new matrix<double>(dim_train, dim_train, ly);
      Kinv_pre_calc = new matrix<double>(dim_train, dim_train, ly);
      Choleski_pre_calc = new matrix<double>(dim_train, dim_train, ly);
      beta_pre_calc = new matrix<double>(dim_train, 1, ly);

      /* allocate for preprocessing coordinate transformation, read in data,
      which also applies the coordinate transformation and fills the training vectors */
      training_std_deviation = new double[dimx];
      training_sample_mean   = new double[dimx];
      read_training_set(training_input, training_targets);
      initialize_hyperparameters(true);
    }
/****************************************************************************************************************/

    /* allocate from a saved gpr state */ 
    gaussian_process_regression(const char* gpr_state_file) : ly(32,32,2,1,false)
    {
      /* begin wall clock for gpr */
      wallclock.quiet();
      wallclock.start("start gpr timer");

      FILE *fp = NULL;
      int error = 0;
      dimx = dim_train = dimh = 0;
      if(ly.rank == 0)
      { 
        fp = fopen(gpr_state_file, "r");
        if(fp == NULL) error = 1;
      }
      graceful_exit(error, "Could not read gpr_state_file\n");

      /* read in dimensions and broadcast */
      if(ly.rank == 0)
      {
        fread(&dimx, sizeof(int), 1, fp);
        fread(&dim_train, sizeof(int), 1, fp);
        fread(&dimh, sizeof(int), 1, fp);
        fread(&num_pseudo_inputs, sizeof(int), 1, fp);
        fread(approximation_type, sizeof(char), 256, fp);
      }
      
      broadcast(&dimx,1);
      broadcast(&dim_train,1);
      broadcast(&dimh,1);
      broadcast(&num_pseudo_inputs,1);
      broadcast(approximation_type,256);
      
      /* set layout correctly */
      if(!strcmp(approximation_type,"spgp"))
      {
        ly.ratio = double(dim_train)/num_pseudo_inputs;
        ly.layout_dimension = 1;
      }
      else
        ly.ratio = 1;
      ly.verbose = true;
      ly.init();

      /* allocate buffers */
      allocate_buffers();

      /* read in buffers */
      if(ly.rank == 0)
      {
        fread(train_matrix, sizeof(double), dimx*dim_train, fp);
        fread(kernel_type, sizeof(char), 256, fp);
        fread(hyper_parameters, sizeof(double), dimh, fp);
        fread(&log_determinant_pre_calc, sizeof(double), 1, fp);
        fread(&kernel_nugget, sizeof(double), 1, fp);
        fread(training_sample_mean, sizeof(double), dimx, fp);
        fread(training_std_deviation, sizeof(double), dimx, fp);
        fread(&training_targets_mean, sizeof(double), 1, fp);
        fread(&training_targets_variance, sizeof(double), 1, fp);
        fread(xmin, sizeof(double), dimx, fp);
        fread(xmax, sizeof(double), dimx, fp);
        fread(preprocess_method, sizeof(char), 256, fp);
        for(int i=0; i<dim_train; i++) fread(df_NIGP[i]->d, sizeof(double), dimx, fp);
      }
      
      /* read in the rest -- these quantites are already distributed across nodes when the read
      function is called. The read function _should_ be called by all processors */
      output_vector->read(fp);
      if(strcmp(approximation_type,"spgp"))
        beta_pre_calc->read(fp);
      else
        AKy->read(fp);
      if(ly.rank == 0) fclose(fp);
      
      /* broadcast */ 
      broadcast(train_matrix, dimx*dim_train);
      broadcast(kernel_type, 256);
      broadcast(hyper_parameters, dimh);
      broadcast(&log_determinant_pre_calc, 1);
      broadcast(&kernel_nugget, 1);
      broadcast(training_sample_mean, dimx);
      broadcast(training_std_deviation, dimx);
      broadcast(&training_targets_mean, 1);
      broadcast(&training_targets_variance, 1);
      broadcast(xmin,dimx);
      broadcast(xmax,dimx);
      broadcast(preprocess_method, 256);
      for(int i=0; i<dim_train; i++) broadcast(df_NIGP[i]->d,dimx);
     
      /* fill train_data */
      fill_training_vectors(train_matrix, train_data, dim_train);
    }
    
    ~gaussian_process_regression() 
    {
      delete [] training_sample_mean;
      delete [] training_std_deviation;
      delete [] xmin; 
      delete [] xmax;
      if(strcmp(approximation_type,"spgp"))
      {
        delete beta_pre_calc;
        delete Choleski_pre_calc;
        delete Kinv_pre_calc;
        delete K_pre_calc;
      }
      else
      {
        delete [] gAKy;
        delete KMM;
        delete FKMM;
        delete KMMInv;
        delete KNM;
        delete NM;
        delete bKNM;
        delete bKNMA;
        delete A;
        delete AInv;
        delete ybar; //Gamma^-1 y  the name is confusing
        delete ybbar; //Gamma^-1/2 y
        delete KKy;
        delete KKy1;
        delete AKy;
        delete KAKy;
        delete KAKyybar;
        delete KAK;
        delete dotKMM;
        delete dotKNM;
        delete bdotKNM;
        delete bdotKNN;
        delete bKNMKMMInv;
        delete bbKNMKMMInv;
        delete bKKK;
        delete bbKKK;
        delete bKKKy;
        delete bKKKy1;
        delete bKKKy2;
        delete bKKKA;
        delete MM;
        delete MM1;
        delete ytmp;
        delete g;
        delete gh;
      }
      delete [] train_matrix;
      for(int i=0; i<dim_train; i++) delete train_data[i];
      for(int i=0; i<dim_train; i++) delete df_NIGP[i];
      delete [] train_data;
      delete [] avg_grad;
      delete [] df_NIGP;
      delete [] hyper_parameters;
      delete  output_vector;
    }

    void initialize_hyperparameters(bool verbose=false)
    {
     /* set inital hyper-parameters ... note this only makes sense if you normalized the variance 
        or used the "default" pre-processing if you did not do that then the user should set the hyper-paramters themselves
     */
     if(!strcmp(kernel_type, "ARD_without_noise"))
     {
       for(int i=0; i<=dimx; i++) hyper_parameters[i] = double(1);
       kernel_nugget = double(1)/signal_to_kernel;
       if(ly.rank == 0)
       {
         if(verbose)
         {
           printf("%s: Setting hyper-parameters var(x_i) to 1.0.\n", kernel_type);
           printf("Amplitude variance = %+0.15e\n", hyper_parameters[dimx]);
           printf("Kernel nugget      = %+0.15e (%0.1e signal-to-kernal variance)\n\n", kernel_nugget, signal_to_kernel);
         }
       }
     }
     else if(!strcmp(kernel_type, "ARD_with_noise"))
     {
       for(int i=0; i<dimx; i++) hyper_parameters[i] = double(0.5);
       hyper_parameters[dimx] = double(0.5);
       hyper_parameters[dimx+1] =  hyper_parameters[dimx]/sqrt(signal_to_noise); //signal-to-noise = %10
       kernel_nugget = double(1)/signal_to_kernel;
       if(ly.rank == 0)
       {
         if(verbose)
         {
           printf("%s: Setting hyper-parameters var(x_i) to 1.0.\n", kernel_type);
           printf("Amplitude variance = %+0.15e\n", hyper_parameters[dimx]);
           printf("Noise variance     = %+0.15e\n", hyper_parameters[dimx+1]);
           printf("Kernel nugget      = %+0.15e (%0.1e signal-to-kernal variance)\n\n", kernel_nugget, signal_to_kernel);
         }
       }
     }
     else if(!strcmp(kernel_type, "ARD_NIGP"))
     {
       for(int i=0; i<=dimx; i++) hyper_parameters[i] = double(1);
       hyper_parameters[dimx+1] =  hyper_parameters[dimx]/sqrt(signal_to_noise); //signal-to-noise = %10
       for(int i=dimx+2; i<dimh; i++) hyper_parameters[i] = double(0); /* set NIGP to zero to start */
       kernel_nugget = double(1)/signal_to_kernel;
       if(ly.rank == 0)
       {
         printf("Using ARD_NIGP kernel requires the user to set the initial hyper-parameters.\n");
         printf("They should be set to trained hyper-parameters of standard GP ARD_with_noise.\n");
       }
     }

     if(!strcmp(approximation_type,"spgp"))
     {
       /* set pseudo inputs to randomly selected points in the training set */
       int offset = dimh - num_pseudo_inputs*dimx;
       std::vector<int> indexes;
       for(int i=0; i<dim_train; i++) indexes.push_back(i);
       std::default_random_engine dre = std::default_random_engine(0);
       for(int i=0; i<num_shuffles_spgp; i++)
         std::shuffle(indexes.begin(),indexes.end(),dre);
       indexes.resize(num_pseudo_inputs);
       for(int i = 0; i < num_pseudo_inputs; i++)
       for(int k=0; k<dimx; k++) 
       {
         hyper_parameters[offset + dimx*i + k] = train_data[indexes[i]]->d[k] + 1e-6;
       }
     }
    }
    
    void set_signal_to_noise(double signal_to_noise)
    {
      this->signal_to_noise = signal_to_noise;
      if(ly.rank == 0) printf("signal-to-noise set to %0.15e\n", this->signal_to_noise);
      initialize_hyperparameters();
    }
    
    void set_signal_to_kernel(double signal_to_kernel)
    {
      this->signal_to_kernel = signal_to_kernel;
      if(ly.rank == 0) printf("signal-to-kernel set to %0.15e\n", this->signal_to_kernel);
      initialize_hyperparameters();
    }

    void compute_coordinate_transformation_parameters(double* x)
    {
      if(!strcmp(preprocess_method,"default"))
      {
        if(ly.rank == 0)
        {
          printf("\nData preprocessing method = \"default\":\n"); 
          printf("Rescaling the training set to have zero mean and unit variance.\n\n");
        }
        for(int j=0; j<dimx; j++) 
        {
          training_sample_mean[j]=0.;
          training_std_deviation[j]=0.;
        }
       
        for(int i=0; i<dim_train; i++)
        for(int j=0; j<dimx; j++)
          training_sample_mean[j]+=x[j+i*dimx];
        for(int j=0; j<dimx; j++) training_sample_mean[j]/=dim_train;
        
        for(int i=0; i<dim_train; i++)
        for(int j=0; j<dimx; j++)
        {
          double residual = x[j+i*dimx]-training_sample_mean[j];
          training_std_deviation[j] += residual*residual;
        }
        for(int j=0; j<dimx; j++) training_std_deviation[j] = sqrt(training_std_deviation[j]/(dim_train - 1.0));

        /* target variance */
        training_targets_variance = training_targets_mean = 0.0;
        for(int i=0; i<output_vector->lrows*output_vector->lcols; i++) training_targets_mean += output_vector->pMat[i];
        training_targets_mean /= dim_train;
        global_sum(training_targets_mean);
        for(int i=0; i<output_vector->lrows*output_vector->lcols; i++) 
        {
          double z;
          z = (training_targets_mean - output_vector->pMat[i]);
          training_targets_variance += z*z;
        }
        training_targets_variance /= (dim_train - 1.0);
        global_sum(training_targets_variance);
      }
      else
      {
        if(ly.rank == 0)
        {
          printf("Data preprocessing method = \"none\":\n"); 
          printf("Nothing done.\n\n");
        }
      }
    }

    void apply_coordinate_transformation(double* x, int num_samples, bool isInverse=false)
    {
      if(!strcmp(preprocess_method,"default"))
      {
        for(int i=0; i<num_samples; i++)
        for(int j=0; j<dimx; j++)
        {
          if(training_std_deviation[j] > 1e-100 ) 
          {
            if(isInverse)
               x[j+i*dimx] = x[j+i*dimx]*training_std_deviation[j]+training_sample_mean[j];
            else
               x[j+i*dimx] = (x[j+i*dimx]-training_sample_mean[j])/training_std_deviation[j];
          }
          else 
          {
            if(ly.rank==0) printf("Warning: parameter %i is constant var(x) = %0.15e. Setting variance to 1. You probably want to remove it.\n",j,training_std_deviation[j]);
            training_std_deviation[j] = double(1.0);
          }
        }
      }
      else if(!strcmp(preprocess_method,"none")) {}
      else 
      {
        int error = 1;
        graceful_exit(error, "The data coordinate transformation specificiation is invalid");
      }
    }
    
    void apply_target_transformation(matrix<double>& targets, int isInverse)
    {
      if(!strcmp(preprocess_method,"default")) {
       if(isInverse) 
       {
         for(int j=0; j<targets.lcols; j++)
         for(int i=0; i<targets.lrows; i++)
         targets(i,j) = targets(i,j)*sqrt(training_targets_variance)+training_targets_mean;
       }
       else
       {
         for(int j=0; j<targets.lcols; j++)
         for(int i=0; i<targets.lrows; i++)
           targets(i,j) = (targets(i,j)-training_targets_mean)/sqrt(training_targets_variance);
       }
      }
    }
    
    void apply_covariance_transformation(matrix<double>& cov)
    {
      if(!strcmp(preprocess_method,"default")) {
        cov *= training_targets_variance;
      }
    }
    
    void write_pseudo_inputs(const char* filename)
    {
      FILE* fp = NULL;
      if(ly.rank == 0)
      {
        double tmp[num_pseudo_inputs*dimx];
        int offset = dimh - num_pseudo_inputs*dimx;
        for(int i=0; i<num_pseudo_inputs*dimx; i++) tmp[i] = hyper_parameters[offset+i];
        apply_coordinate_transformation(tmp,num_pseudo_inputs,true);
        fp = fopen(filename, "w");
        fwrite(tmp, sizeof(double), dimx*num_pseudo_inputs, fp);
        fclose(fp);
      }
    }

    void write_gpr_state(const char* filename)
    {
      FILE* fp = NULL;
      if(ly.rank == 0)
      {
        fp = fopen(filename, "w");
        //if(fp==NULL) gpr_error("Couldn't open output file to write gpr_state");
        fwrite(&dimx, sizeof(int), 1, fp);
        fwrite(&dim_train, sizeof(int), 1, fp);
        fwrite(&dimh, sizeof(int), 1, fp);
        fwrite(&num_pseudo_inputs, sizeof(int), 1, fp);
        fwrite(approximation_type, sizeof(char), 256, fp);
        fwrite(train_matrix, sizeof(double), dimx*dim_train, fp);
        fwrite(kernel_type, sizeof(char), 256, fp);
        fwrite(hyper_parameters, sizeof(double), dimh, fp);
        fwrite(&log_determinant_pre_calc, sizeof(double), 1, fp);
        fwrite(&kernel_nugget, sizeof(double), 1, fp);
        fwrite(training_sample_mean, sizeof(double), dimx, fp);
        fwrite(training_std_deviation, sizeof(double), dimx, fp);
        fwrite(&training_targets_mean, sizeof(double), 1, fp);
        fwrite(&training_targets_variance, sizeof(double), 1, fp);
        fwrite(xmin, sizeof(double), dimx, fp);
        fwrite(xmax, sizeof(double), dimx, fp);
        fwrite(preprocess_method, sizeof(char), 256, fp);
        for(int i=0; i<dim_train; i++) fwrite(df_NIGP[i]->d, sizeof(double), dimx, fp);
      }
      barrier();
      output_vector->write(fp);
      if(strcmp(approximation_type,"spgp"))
        beta_pre_calc->write(fp);
      else
        AKy->write(fp);
      if(ly.rank == 0) fclose(fp);
    }
    
    void set_kernel()
    {
     if(!strcmp(kernel_type, "ARD_without_noise"))
     {
       dimh = dimx + 1;
       isNoise = false;
       isRegSPGP = false;
     }
     else if(!strcmp(kernel_type, "ARD_with_noise"))
     {
       dimh = dimx + 2;
       isNoise = true;
       isRegSPGP = true;
     }
     else if(!strcmp(kernel_type, "ARD_NIGP"))
     {
       dimh = 2*dimx + 2; 
       isNoise = true;
     }
     else
     {
        int error = 1;
        graceful_exit(error, "The kernel specification specificiation is invalid");
     }

     /* add spgp hyper-parameters */
     if(!strcmp(approximation_type, "spgp")) 
     {
       //if(strcmp(kernel_type, "ARD_with_noise")) 
       //  graceful_exit(1,"SPGP only works with the ARD_with_noise kernel");
       dimh += num_pseudo_inputs*dimx;
     }
    }

    void read_training_set(const char* training_input_file, const char* training_targets_file)
    {

      /* read in data and send to everyone*/
      FILE* fp = NULL;
      int error = 0;
      if(ly.rank == 0)
      {
        fp = fopen(training_input_file, "r");
        if(fp==NULL) error = 1;
      } 
      graceful_exit(error, "Could not open the training_input_file");

      if(ly.rank == 0)
      {
        fread(train_matrix, dimx*dim_train, sizeof(double), fp);
        fclose(fp);
      }
      broadcast(train_matrix, dimx*dim_train);

      /* read in training targets --- read function already MPI aware and broadcast to everyone */
      if(ly.rank == 0)
      {
        fp = fopen(training_targets_file, "r");
        if(fp==NULL) error = 1;
      }
      graceful_exit(error, "Could not open training_targets_file");
      output_vector->read(fp);
      if(ly.rank == 0) fclose(fp);
      
      /* compute, set, and apply standardization transformation parameters */
      compute_coordinate_transformation_parameters(train_matrix);
      apply_coordinate_transformation(train_matrix, dim_train);
      fill_training_vectors(train_matrix,train_data, dim_train);
      apply_target_transformation(*output_vector,false);

      barrier();
    }

    void get_coordinate_ranges()
    {
      for(int i=0; i<dimx; i++)
      {
        xmin[i] = train_matrix[i];
        xmax[i] = train_matrix[i];
      }
      
      for(int i=0; i<dim_train; i++)
      for(int j=0; j<dimx; j++)
      {
         if(train_matrix[j+dimx*i] > xmax[j]) xmax[j] = train_matrix[j+dimx*i];
         if(train_matrix[j+dimx*i] < xmin[j]) xmin[j] = train_matrix[j+dimx*i];
      }
      
      //add padding
      double s,b;
      for(int j=0; j<dimx; j++)
      {
        s = xmin[j];
        b = xmax[j];
        xmin[j] = s-(b-s)*0.001;
        xmax[j] = b+(b-s)*0.001;
      }

    }
    
    void read_training_set(double* training_input, double* training_targets)
    {

      /* read in data and send to everyone*/
      if(ly.rank == 0)
      {
        for(int i=0; i<dimx*dim_train; i++)
          train_matrix[i] = training_input[i];
      }
      broadcast(train_matrix, dimx*dim_train);

      /* read in training targets --- read function already MPI aware and broadcast to everyone */
      int gi, gj;
      int rows = output_vector->rows;
      int cols = output_vector->cols;
      int lrows = output_vector->lrows;
      int lcols = output_vector->lcols;
      double* g = new double[rows*cols];
      if(ly.rank == 0)
      {
        for(int i=0; i<rows*cols; i++)
          g[i] = training_targets[i];
      }
      broadcast(g, rows*cols);
      for(int j=0; j<lcols; j++)
      for(int i=0; i<lrows; i++)
      {
        gi = ly.localToGlobalRow(i);
        gj = ly.localToGlobalCol(j);
        output_vector->pMat[i+lrows*j] = g[gj+lcols*gi];
      };
      delete [] g;
      
      /* compute, set, and apply standardization transformation parameters */
      compute_coordinate_transformation_parameters(train_matrix);
      apply_coordinate_transformation(train_matrix, dim_train);
      fill_training_vectors(train_matrix,train_data, dim_train);
      apply_target_transformation(*output_vector,false);

      barrier();
    }

    void fill_training_vectors(double* y, vector** x, int num_samples)
    {
      for(int i=0; i<num_samples; i++)
      {
        int offset = i*dimx;
        for(int j=0; j<dimx; j++) 
        {
          x[i]->d[j] = y[offset+j];
        }
      }
    };

    void update_df_NIGP()
    {
      matrix<double> K(dim_train,dim_train,ly);
      matrix<double> beta(dim_train, 1, ly);
      function_covariance_ARD_with_noise(hyper_parameters,K);
      K.choleski_lower();
      solve_choleski_system(K, *output_vector, beta);
     
      /* clear buffers */ 
      for(int i=0; i<dim_train; i++)
      for(int k=0; k<dimx; k++) df_NIGP[i]->d[k] = 0.0;

      for(int i=0; i<dim_train; i++)
      for(int k=0; k<beta.lcols; k++)
      for(int j=0; j<beta.lrows; j++)
      {
        int gj = ly.localToGlobalRow(j);
        for(int l=0; l<dimx; l++)
          df_NIGP[i]->d[l] -= hyper_parameters[l]*hyper_parameters[l]*(train_data[i]->d[l] - train_data[gj]->d[l])*squared_exponential_ARD(hyper_parameters, *train_data[i], *train_data[gj])*beta(j,0);
      }

      for(int i=0; i<dim_train; i++)
      for(int l=0; l<dimx; l++)
        global_sum(df_NIGP[i]->d[l]);
    }

    void compute_avg_grad_NIGP()
    {
         for(int j=0; j<dimx; j++)
           avg_grad[j] = double(0);

         for(int i=0; i<dim_train; i++)
         for(int j=0; j<dimx; j++)
           avg_grad[j] += sqrt(df_NIGP[i]->d[j]*df_NIGP[i]->d[j]);
         
         for(int j=0; j<dimx; j++) avg_grad[j] /= (dimx*dim_train);
         if(get_rank()==0) for(int j=0; j<dimx; j++) printf("avg %i %0.15e\n",j,avg_grad[j]);
    }
    
    double squared_exponential_ARD(double *hyperp, vector& x, vector& xp)
    {
      double expon = 0;
      for(int i=0; i<dimx; i++)
      {
        double z = (x.d[i] - xp.d[i])*hyperp[i];
        expon += z*z;
      }
      expon *= -double(0.5);
      return hyperp[dimx]*hyperp[dimx]*exp(expon);
    }
    
    double squared_exponential_ARD(double *hyperp, double* x, double* xp)
    {
      double expon = 0;
      for(int i=0; i<dimx; i++)
      {
        double z = (x[i] - xp[i])*hyperp[i];
        expon += z*z;
      }
      expon *= -double(0.5);
      return hyperp[dimx]*hyperp[dimx]*exp(expon);
    }

    void function_covariance_ARD_NM_spgp(double* hyperp, matrix<double>& covariance)
    {
      double *pseudo = hyperp + (dimh - num_pseudo_inputs*dimx);
      /* KM */
      for(int j=0; j<covariance.lcols; j++)
      for(int i=0; i<covariance.lrows; i++)
      {
        int gi = ly.localToGlobalRow(i);
        int gj = ly.localToGlobalCol(j);
        covariance(i,j) = squared_exponential_ARD(hyperp, train_data[gi]->d, pseudo + gj*dimx);
        if(gi == gj) covariance(i,j) += kernel_nugget;
      }
    }

    void function_covariance_ARD_MM_spgp(double* hyperp, matrix<double>& covariance)
    {
      double *pseudo = hyperp + (dimh - num_pseudo_inputs*dimx);
      /* KM */
      for(int j=0; j<covariance.lcols; j++)
      for(int i=0; i<covariance.lrows; i++)
      {
        int gi = ly.localToGlobalRow(i);
        int gj = ly.localToGlobalCol(j);
         covariance(i,j) = squared_exponential_ARD(hyperp, pseudo + gi*dimx, pseudo + gj*dimx);
        if(gi==gj) covariance(i,j) += kernel_nugget;
      }
    }
    
    void function_covariance_ARD_gamma_spgp(double* hyperp,  matrix<double>& KNM, matrix<double>& KNMKMMInv,  matrix<double>& gamma)
    {
      //ensure gamma is not a negative small number by taking absolute value
       diag_spgp2(KNM,KNMKMMInv,gamma);
       if(isNoise)
       {
         for(int i=0; i<gamma.lrows; i++) 
         {
           gamma(i,0) = (hyperp[dimx]*hyperp[dimx] + kernel_nugget - gamma(i,0))/(hyperp[dimx+1]*hyperp[dimx+1]) + double(1);
           if(abs(gamma(i,0)) < 0) gamma(i,0) =  abs(gamma(i,0));
         }
       }
       else
       {
         for(int i=0; i<gamma.lrows; i++) 
         {
           gamma(i,0) = (hyperp[dimx]*hyperp[dimx] + kernel_nugget - gamma(i,0));
           if(gamma(i,0) < 0) gamma(i,0) =  abs(gamma(i,0));
         }
       }
    }
    
    void function_covariance_ARD_with_noise(double *hyperp, matrix<double>& covariance)
    {
        double sqsigma = hyperp[dimx+1]*hyperp[dimx+1];
        for(int j=0; j<covariance.lcols; j++)
        for(int i=0; i<covariance.lrows; i++)
        {
            int gi = ly.localToGlobalRow(i);
            int gj = ly.localToGlobalCol(j);
            covariance(i,j) = squared_exponential_ARD(hyperp, train_data[gi]->d, train_data[gj]->d);
           if(gi==gj) covariance(i,j) += sqsigma + kernel_nugget;
        }
    }
    
    void function_covariance_ARD_no_noise(double *hyperp, matrix<double>& covariance)
    {
      for(int j=0; j<covariance.lcols; j++)
        for(int i=0; i<covariance.lrows; i++)
        {
          int gi = ly.localToGlobalRow(i);
          int gj = ly.localToGlobalCol(j);
          covariance(i,j) = squared_exponential_ARD(hyperp, *train_data[gi], *train_data[gj]);
          if(gi==gj) covariance(i,j) += kernel_nugget;
        }
    }
    
    void function_covariance_ARD_NIGP(double *hyperp, matrix<double>& covariance)
    {
        double sqsigma = hyperp[dimx+1]*hyperp[dimx+1];
        if(sqsigma < kernel_nugget) sqsigma = kernel_nugget;
        for(int j=0; j<covariance.lcols; j++)
        for(int i=0; i<covariance.lrows; i++)
        {
            int gi = ly.localToGlobalRow(i);
            int gj = ly.localToGlobalCol(j);
            covariance(i,j) = squared_exponential_ARD(hyperp, *train_data[gi], *train_data[gj]);
            if(gi==gj) {
              covariance(i,j) += sqsigma;
              /* add NIGP input variance */
              for(int k=0; k<dimx; k++)
                covariance(i,j) += df_NIGP[gi]->d[k]*df_NIGP[gi]->d[k]*hyperp[dimx+2+k]*hyperp[dimx+2+k];
            }
        }
    }

    void function_covariance(double *hyperp, matrix<double>& covariance)
    {
      if(!strcmp(kernel_type, "ARD_without_noise"))
      {
        function_covariance_ARD_no_noise(hyperp, covariance);
      }
      else if(!strcmp(kernel_type, "ARD_with_noise"))
      {
        function_covariance_ARD_with_noise(hyperp,covariance);
      }
      else
        function_covariance_ARD_NIGP(hyperp,covariance);
    }
    
    void derivative_function_covariance_ARD_no_noise(double *hyperp, int dir, matrix<double>& covariance)
    {
        if(dir<dimx)
        {
          for(int j=0; j<covariance.lcols; j++)
          for(int i=0; i<covariance.lrows; i++)
          {
            int gi = ly.localToGlobalRow(i);
            int gj = ly.localToGlobalCol(j);
            double diff = train_data[gi]->d[dir] - train_data[gj]->d[dir];
            if(gi == gj)
              covariance(i,j) = double(0);
            else
              covariance(i,j) = -squared_exponential_ARD(hyperp,*train_data[gi],*train_data[gj])*hyperp[dir]*diff*diff;
          }
        }
        else if(dir == dimx)
        {
          for(int j=0; j<covariance.lcols; j++)
          for(int i=0; i<covariance.lrows; i++)
          {
            int gi = ly.localToGlobalRow(i);
            int gj = ly.localToGlobalCol(j);
            if(gi == gj) 
              covariance(i,j) = double(2)*hyperp[dimx];
            else
              covariance(i,j) = double(2)/hyperp[dimx]*squared_exponential_ARD(hyperp,*train_data[gi], *train_data[gj]);
          }
        }
        else
          fprintf(stderr,"error derivative_function_covariance_ARD_no_noise dir > dimh");
        //covariance.write("cov");
    }
    
    
    void derivative_function_covariance_ARD_with_noise(double *hyperp, int dir, matrix<double>& covariance)
    {
      if(dir <= dimx)
        derivative_function_covariance_ARD_no_noise(hyperp,dir,covariance);
      else
      {
        for(int j=0; j<covariance.lcols; j++)
          for(int i=0; i<covariance.lrows; i++)
          {
            int gi = ly.localToGlobalRow(i);
            int gj = ly.localToGlobalCol(j);
            gi==gj ? covariance(i,j) = double(2)*hyperp[dimx+1] : covariance(i,j) = double(0);
          }
      }
    }
    
    void derivative_function_covariance_ARD_NIGP(double *hyperp, int dir, matrix<double>& covariance)
    {
        for(int j=0; j<covariance.lcols; j++)
        for(int i=0; i<covariance.lrows; i++)
        {
            int gi = ly.localToGlobalRow(i);
            int gj = ly.localToGlobalCol(j);
            double expon = 0.0;
            double diff_dir = 0.0;
            for(int k=0; k<dimx; k++)
            {
              double diff = (train_matrix[k+dimx*gi] - train_matrix[k+dimx*gj]);
              expon += -0.5*diff*hyperp[k]*hyperp[k]*diff;
              if(k==dir) diff_dir = diff;
            }
            /* STD GP derivatives */
            if(dir<dimx) {
              covariance(i,j) = -0.5*hyperp[dimx]*hyperp[dimx]*exp(expon)*diff_dir*diff_dir*2.0*hyperp[dir];
            }
            else if(dir==dimx) {
              covariance(i,j) = 2.0*hyperp[dir]*exp(expon);
            }
            else if(dir==dimx+1) {
             gi==gj ? covariance(i,j) = 2.0*hyperp[dir] : covariance(i,j) = 0.0;
            }
            else 
            {
             gi==gj ? covariance(i,j) = 2.0*hyperp[dir]*df_NIGP[gi]->d[dir-dimx+2]* df_NIGP[gi]->d[dir-dimx+2] : covariance(i,j) = 0.0;
            }
        }
    }
    
    void derivative_function_covariance_ARD_NN_spgp(double* hyperp, int dir, double* diagKNN) 
    {
      if(dir == dimx)
      {
       for(int i=0; i<dim_train; i++)
          diagKNN[i] = double(2)*hyperp[dimx];
      }
      else
      {
       for(int i=0; i<dim_train; i++)
          diagKNN[i] = double(0);
      }
    }
    
    void derivative_function_covariance_ARD_NN_spgp(double* hyperp, int dir, matrix<double>& diagKNN) 
    {
      diagKNN.clear();
      if(dir == dimx)
      {
       for(int i=0; i<diagKNN.lrows; i++)
          diagKNN(i,0) = double(2)*hyperp[dimx];
      }
    }

    void derivative_function_covariance_ARD_NM_spgp(double* hyperp, int dir, matrix<double>& covariance) 
    {
      int offset = dimh - num_pseudo_inputs*dimx;
      double *p = hyperp+offset;
      if(dir<dimx)
      {
        for(int j=0; j<covariance.lcols; j++)
          for(int i=0; i<covariance.lrows; i++)
          {
            int gi = ly.localToGlobalRow(i);
            int gj = ly.localToGlobalCol(j);
            double diff = train_data[gi]->d[dir] - p[dir+gj*dimx];
            //covariance(i,j) = -squared_exponential_ARD(hyperp,train_data[gi]->d,p+gj*dimx)*hyperp[dir]*diff*diff;
            if(gi == gj) 
            {
//              covariance(i,j) += kernel_nugget*hyperp[dir]*diff*diff;
              covariance(i,j) = -((*KNM)(i,j)-kernel_nugget)*hyperp[dir]*diff*diff;
            }
            else
              covariance(i,j) = -(*KNM)(i,j)*hyperp[dir]*diff*diff;
          }
      }
      else if(dir == dimx)
      {
        for(int j=0; j<covariance.lcols; j++)
          for(int i=0; i<covariance.lrows; i++)
          {
            int gi = ly.localToGlobalRow(i);
            int gj = ly.localToGlobalCol(j);
            //covariance(i,j) = double(2)/hyperp[dimx]*squared_exponential_ARD(hyperp,train_data[gi]->d, p+gj*dimx);
            //covariance(i,j) = double(2)/hyperp[dimx]*(*KNM)(i,j);
            if(gi == gj) 
              covariance(i,j) = double(2)/hyperp[dimx]*((*KNM)(i,j)-kernel_nugget);
            else
              covariance(i,j) = double(2)/hyperp[dimx]*(*KNM)(i,j);
            //  covariance(i,j) -= double(2)/hyperp[dimx]*kernel_nugget;
          }
      }
      else if(dir == dimx + 1 && isNoise) {
        fprintf(stderr,"error in spgp derivative\n");
      }
      else
      {
        int n = dir - offset;
        int idx = n / dimx;
        int k = n - idx*dimx;
        double *p = hyperp + offset;
        for(int j=0; j<covariance.lcols; j++)
        for(int i=0; i<covariance.lrows; i++)
        {
          int gi = ly.localToGlobalRow(i);
          int gj = ly.localToGlobalCol(j);
          //covariance(i,j) = squared_exponential_ARD(hyperp,train_data[gi]->d,p+gj*dimx)*(train_data[gi]->d[k]- p[k + gj*dimx])*hyperp[k]*hyperp[k];
          //covariance(i,j) = (*KNM)(i,j)*(train_data[gi]->d[k]- p[k + gj*dimx])*hyperp[k]*hyperp[k];
          if(gi == gj) 
            covariance(i,j) = ((*KNM)(i,j)-kernel_nugget)*(train_data[gi]->d[k]- p[k + gj*dimx])*hyperp[k]*hyperp[k];
          else
            covariance(i,j) = (*KNM)(i,j)*(train_data[gi]->d[k]- p[k + gj*dimx])*hyperp[k]*hyperp[k];
        }
      }
    }
    
    /* derivative with respect to m in a col argument d/dy'K(x,y) */  
    void derivative_function_covariance_ARD_MM_spgp(double* hyperp, int dir, matrix<double>& covariance)
    {
        int offset = dimh - num_pseudo_inputs*dimx;
        double * p = hyperp + offset;
        if(dir<dimx)
        {
          for(int j=0; j<covariance.lcols; j++)
          for(int i=0; i<covariance.lrows; i++)
          {
            int gi = ly.localToGlobalRow(i);
            int gj = ly.localToGlobalCol(j);
            double diff = p[dir+gi*dimx] - p[dir+gj*dimx];
            if(gi==gj)
              covariance(i,j) = double(0);
            else
              covariance(i,j) = -(*FKMM)(i,j)*hyperp[dir]*diff*diff;
          }
        }
        else if(dir == dimx)
        {
          for(int j=0; j<covariance.lcols; j++)
          for(int i=0; i<covariance.lrows; i++)
          {
            int gi = ly.localToGlobalRow(i);
            int gj = ly.localToGlobalCol(j);
            if(gi==gj) 
              covariance(i,j) = double(2)*hyperp[dimx];
            else 
              covariance(i,j) = double(2)/hyperp[dimx]*((*FKMM)(i,j));
          }
        }
        else if(dir == dimx + 1 && isNoise)
        {
          if(ly.rank == 0) fprintf(stderr, "error in derivative_function_covariance_ARD_MM_spgp\n");
        }
        else
        {
          int n = dir - offset;
          int idx = n / dimx;
          int k = n - idx*dimx;
          for(int j=0; j<covariance.lcols; j++)
          for(int i=0; i<covariance.lrows; i++)
          {
            int gi = ly.localToGlobalRow(i);
            int gj = ly.localToGlobalCol(j);
            //covariance(i,j) = squared_exponential_ARD(hyperp,p+gi*dimx,p+gj*dimx)*(p[k+gi*dimx] - p[k+gj*dimx])*hyperp[k]*hyperp[k];
            //covariance(i,j) = ((*FKMM)(i,j))*(p[k+gi*dimx] - p[k+gj*dimx])*hyperp[k]*hyperp[k];
            if(gi == gj) 
              covariance(i,j) = ((*FKMM)(i,j)-kernel_nugget)*(p[k+gi*dimx] - p[k+gj*dimx])*hyperp[k]*hyperp[k];
            else
              covariance(i,j) = ((*FKMM)(i,j))*(p[k+gi*dimx] - p[k+gj*dimx])*hyperp[k]*hyperp[k];
              //covariance(i,j) -= kernel_nugget*(p[k+gi*dimx] - p[k+gj*dimx])*hyperp[k]*hyperp[k];
          }
        }
    }
    
    void derivative_function_covariance_ARD_MM_spgp_col(double* hyperp, int dir, matrix<double>& covariance)
    {
      int offset = dimh - num_pseudo_inputs*dimx;
      double * p = hyperp + offset;
      int n = dir - offset;
      int idx = n / dimx;
      int k = n - idx*dimx;
      int gj = idx;
      for(int j=0; j<covariance.lcols; j++)
      for(int i=0; i<covariance.lrows; i++)
      {
        int gi = ly.localToGlobalRow(i);
        covariance(i,0) = -squared_exponential_ARD(hyperp,p+gi*dimx,p+gj*dimx)*(p[k+gi*dimx] - p[k+gj*dimx])*hyperp[k]*hyperp[k];
      }
    }
    
    void derivative_function_covariance_ARD_MM_spgp_row(double* hyperp, int dir, matrix<double>& covariance)
    {
      int offset = dimh - num_pseudo_inputs*dimx;
      double * p = hyperp + offset;
      int n = dir - offset;
      int idx = n / dimx;
      int k = n - idx*dimx;
      int gi = idx;
      for(int i=0; i<covariance.lrows; i++)
      for(int j=0; j<covariance.lcols; j++)
      {
        int gj = ly.localToGlobalCol(j);
        covariance(0,j) = squared_exponential_ARD(hyperp,p+gi*dimx,p+gj*dimx)*(p[k+gi*dimx] - p[k+gj*dimx])*hyperp[k]*hyperp[k];
      }
    }
    
    void derivative_function_covariance(double *hyperp, int dir, matrix<double>& covariance)
    {
      if(!strcmp(kernel_type, "ARD_without_noise")) {
        derivative_function_covariance_ARD_no_noise(hyperp, dir, covariance);
      }
      else if(!strcmp(kernel_type, "ARD_with_noise")) {
        derivative_function_covariance_ARD_with_noise(hyperp, dir, covariance);
      }
      else if(!strcmp(kernel_type, "ARD_NIGP")) {
        derivative_function_covariance_ARD_NIGP(hyperp, dir, covariance);
      }
      else 
      {
        if(ly.rank==0)
        {
          int error = 1;
          graceful_exit(error, "Kernel specification is invalid");
        }
      }
    }
    
    //negative log of marginal probability 
    double marginal_probability_full(double* hyperp)
    {
        timer tm;
        matrix<double> L(dim_train,dim_train,ly);
        matrix<double> v(dim_train,1,ly);
        function_covariance(hyperp,L);
        L.choleski_lower();
        solve_choleski_system(L, (*output_vector), v);
        double res = v.scalar_product(*output_vector);
        //sum log
        double log_det = 0;
        int gi, gj;
        for(int j=0; j<L.lcols; j++)
        for(int i=0; i<L.lrows; i++)
        {
          gi = ly.localToGlobalRow(i);
          gj = ly.localToGlobalCol(j);
          if( gi == gj )
            log_det += log(L(i,j));
        }
        global_sum(log_det);
        log_det *= 2.0;
        //printf("l %0.15e %0.15e %0.15e\n", res,log_det,0.5*( res + log_det + dim_train*log(2*Pi)));
        return 0.5*( res + log_det + dim_train*log(2*Pi) );
    }
    
    double marginal_probability_spgp(double* hyperp)
    {
      double l;
      double noise = hyperp[dimx+1]*hyperp[dimx+1];
      if(!isNoise) noise = double(1);
      /* compute L1 */
      l = A->log_determinant();
     // A->print();
     // printf("ll %0.15e\n",l);
      //l-= KMM->log_determinant();
      //for(int i=0; i<dim_train; i++) l+= log(gamma[i]);

      double tr = 0;
      for(int i=0; i<g->lrows; i++) tr += log((*g)(i,0));
      global_sum(tr);
      l += tr;
      if(isNoise) l += (dim_train - num_pseudo_inputs)*log(noise);
      
      /* compute L2 */
      double z = double(0);
      z += output_vector->scalar_product(*ybar);
      z -= ybbar->scalar_product(*KAKy);
      z /= noise;
      l += z;
      l += dim_train*log(double(2)*Pi);
      if(isRegSPGP) l += double(2)*regularization_spgp(hyperp);
      return l/double(2) ; 
      //return l/double(2); 
    }

    double regularization_spgp(double *hyperp)
    {
      double tr = 0;
      for(int i=0; i<g->lrows; i++) tr += double(1)/((*g)(i,0));
      global_sum(tr);
      return (tr - dim_train)/double(2);
    }
    
    double derivative_regularization_spgp(double *hyperp, int dir)
    {
      double noise = hyperp[dimx+1]*hyperp[dimx+1];
      if(!isNoise) noise = double(1);
      int offset = dimh - num_pseudo_inputs*dimx;
      int n = dir - offset;
      int idx = n / dimx;

      /* if noise derivative */
      if(dir == dimx+1 && isNoise)
      {
        double tr = 0;
        for(int i=0; i<g->lrows; i++) tr += (double(1)/((*g)(i,0)) - double(1)/( ((*g)(i,0)) * ((*g)(i,0))));
        return tr/hyperp[dimx+1];
      }
      
      double l1 = 0;
      if(n >=0 ) 
      { 
        //derivatives of pseudo-inputs
        double tr;
        timer tm;

        /* trace dotGamma */ 
        tr =  double(-2)*trace_invC(*bdotKNM,*bKNMKMMInv,*g,idx);
        tr += double(2)*trace(*dotKMM,*bbKKK,idx);     
        tr  *= -double(0.5)/noise;
        l1 = tr;
      } 
      else 
      { 
        //derivatives of standard hyper-parameters
        timer tm;
        double tr;
        /* trace dotGamma bar-bar-bar*/
        tr = 0;
        derivative_function_covariance_ARD_NN_spgp(hyperp,dir,*bdotKNN); // diag(dotKNN)
        for(int i=0; i<bdotKNN->lrows; i++) tr += (*bdotKNN)(i,0)/((*g)(i,0)*(*g)(i,0));
        tr += double(-2)*trace_invC(*bdotKNM,*bKNMKMMInv,*g);
        tr += trace(*dotKMM,*bbKKK);     
        tr *= -double(0.5)/noise;
        l1 = tr;
      }
 //    tm.stop();
      return l1;
    }
    
    double marginal_probability(double* hyperp)
    {
      if(!strcmp(approximation_type,"spgp")) {
        return marginal_probability_spgp(hyperp);
      }
      else {
       return  marginal_probability_full(hyperp);
      }
    }
    
    
    double marginal_probability_pre_calc_full(double* hyperp)
    {
        double res = output_vector->scalar_product(*beta_pre_calc);
        return 0.5*( res + log_determinant_pre_calc + dim_train*log(2*Pi) );
    }
    
    double marginal_probability_pre_calc(double* hyperp)
    {
      if(!strcmp(approximation_type,"spgp")) {
        return marginal_probability_spgp(hyperp); //have to change to pre calc
      }
      else {
       return  marginal_probability_pre_calc_full(hyperp);
      }
    }

   double derivative_marginal_probability_spgp(double *hyperp, int dir)
   {
//timer tm;
//tm.start("nder");
      double noise = hyperp[dimx+1]*hyperp[dimx+1];
      if(!isNoise) noise = double(1);
      int offset = dimh - num_pseudo_inputs*dimx;
      int n = dir - offset;
      int idx = n / dimx;

      /* if noise derivative */
      if(dir == dimx+1 && isNoise)
      {
        double l1 ,l2;
        l1 = 0;
        diag_multiply(*gh,*bKNM,*NM,true); // KNM_barbar
        double tr = 0;
        for(int i=0; i<g->lrows; i++) tr += double(1)/(*g)(i,0);
        l1+=tr;
      
        tr = 0;
        for(int i=0; i<KAK->lrows; i++) 
          tr += (*KAK)(i,0)/(*g)(i,0);
        l1 -= tr;
        
        if(isNoise) l1 /= hyperp[dimx+1];
        l2 = trace_vv(*ybar,*ybar);

        diag_multiply(*gh,*KAKy,*ytmp,true);
        l2 += trace_vv(*ytmp,*ytmp);
        l2  -= double(2)*trace(*ytmp,*ybar);
        if(isNoise) l2 /= (noise*hyperp[dimx+1]);
        if(isRegSPGP) l1 += derivative_regularization_spgp(hyperp,dir);
        return l1 - l2;
       // return l1 - l2;
      }
      
      double l1 = 0;
      double l2 = 0;
      if(n >=0 ) 
      { 
        double tr;
        timer tm;

        /* trace dotGamma */ 
        tr =  double(-2)*trace(*bdotKNM,*bKNMKMMInv,idx);
        tr += double(2)*trace(*dotKMM,*bKKK,idx);     
        tr /= noise;
        l1 = tr;

        /* trace A^-1dotA */
        l1 +=  double(2)*noise*trace(*dotKMM,*AInv,idx);
        l1 +=  double(2)*trace(*bdotKNM,*bKNMA,idx);
        tr  = -double(2)*trace(*bdotKNM, *bKNMKMMInv,*KAK,idx);
        tr +=  double(2)*trace(*dotKMM, *bKKKA,idx);
        tr /= noise;
        l1 -= tr;

        /* trace KMM^-1dotKMM */
        l1 -= double(2)*trace(*KMMInv,*dotKMM,idx);
        l1 = l1/double(2); 
        
        /* second term in snelson thesis */
        //l2 = double(-0.5)*scalar_product(*ybbar,*bdg,*ybbar);
        /* 1/2 ybar bdotgamma ybar -- these two could be combined to reduce pre comp */
        l2 = trace(*bdotKNM,*KKy,*ybbar,idx)/noise;
        l2 += double(-1)*trace(*dotKMM,*bKKKy,idx)/noise;

   //   l2 += scalar_product(*KAKy,*bdg,*KAKyy);
        l2 += double(-2)*trace(*bdotKNM,*KKy,*KAKy,idx)/noise;
        l2 += double(2)*trace(*dotKMM,*bKKKy2,idx)/noise;
        l2 += trace(*bdotKNM,*KKy1,*KAKy,idx)/noise;
        l2 -= trace(*dotKMM,*bKKKy1,idx)/noise;
        
        l2 += double(noise)*trace_vmv(*AKy,*dotKMM,*AKy,idx);
        l2 += trace_vmv(*KAKyybar,*bdotKNM,gAKy,idx);     
        l2 /= noise;
      } 
      else 
      { 
        timer tm;
        double tr;
        derivative_function_covariance_ARD_NN_spgp(hyperp,dir,*bdotKNN); // diag(dotKNN)
        for(int i=0; i<bdotKNN->lrows; i++) (*bdotKNN)(i,0) /= (*g)(i,0);
        
        /* trace dotGamma */
        tr = 0;
        for(int i=0; i<bdotKNN->lrows; i++) tr += (*bdotKNN)(i,0);
        tr += double(-2)*trace(*bdotKNM,*bKNMKMMInv);
        tr += trace(*dotKMM,*bKKK);     
        tr /= noise;
        l1 = tr;

        /* trace A^-1dotA */
        l1 += noise*trace(*dotKMM,*AInv);
        l1 += double(2)*trace(*bdotKNM,*bKNMA);

        tr = trace(*bKNM,*bKNMA,*bdotKNN);
        tr +=  -double(2)*trace(*bdotKNM, *bKNMKMMInv,*KAK);
        tr +=  trace(*dotKMM, *bKKKA);
        tr /= noise;
        l1 -= tr;

        /* trace KMM^-1dotKMM */
        l1 -= trace(*KMMInv,*dotKMM);
        l1 = l1/double(2); 

        /* snelson second term */
        /* 1/2 ybar bdotgamma ybar -- these two could be combined to reduce pre comp */
        l2 = double(-0.5)*trace_vvv(*ybbar,*bdotKNN,*ybbar)/noise;
        l2 += trace(*bdotKNM,*KKy,*ybbar)/noise;
        l2 += double(-0.5)*trace(*dotKMM,*bKKKy)/noise;
        
        //l2 += double(-0.5)*trace_vvv(*KAKy,*bdg,*KAKy);
        l2 += double(-0.5)*trace_vvv(*KAKy,*bdotKNN,*KAKy)/noise;
        l2 += trace(*bdotKNM,*KKy1,*KAKy)/noise;
        l2 -= double(0.5)*trace(*dotKMM,*bKKKy1)/noise;

        //l2 += trace_vvv(*KAKy,*bdg,*ybbar);
        l2 += trace_vvv(*KAKy,*bdotKNN,*ybbar)/noise;
        l2 += double(-2)*trace(*bdotKNM,*KKy,*KAKy)/noise;
        l2 += trace(*dotKMM,*bKKKy2)/noise;
        
        l2 +=  double(noise/2)*trace_vmv(*AKy,*dotKMM,*AKy);
        l2 += trace_vmv(*KAKyybar,*bdotKNM,gAKy);
        l2 /= noise;
      }
 //    tm.stop();
      if(isRegSPGP) l1 += derivative_regularization_spgp(hyperp,dir);
      return l1 + l2;
      //return l1 + l2;
   }

    //derivative of negative log of marginal probability
    double derivative_marginal_probability_full(double *hyperp, int dir)
    {
        matrix<double> K(dim_train,dim_train,ly);
        matrix<double> Kinv(dim_train,dim_train,ly);
        matrix<double> choleski(dim_train,dim_train,ly);
        matrix<double> dK(dim_train,dim_train,ly);
        matrix<double> alpha(dim_train,1,ly);
        matrix<double> beta(dim_train,1,ly);
        matrix<double> A(dim_train,dim_train,ly);
       
        function_covariance(hyperp,K);
        choleski = K;
        choleski.choleski_lower();
        Kinv = choleski;
        Kinv.inverse_choleski();
        derivative_function_covariance(hyperp, dir, dK);
       
        //compute A matrix 
        //alpha = Kinv*(*output_vector);
        solve_choleski_system(choleski, *output_vector, alpha);
        beta = dK*alpha;
        double res = alpha.scalar_product(beta);

        A = Kinv*dK;
        return -0.5*(res - A.trace());
    }
    
    double derivative_marginal_probability_pre_calc_full(double* hyperp,  matrix<double>& Kinv, matrix<double>& alpha, int dir)
    {
        matrix<double> dK(dim_train,dim_train,ly);
        matrix<double> beta(dim_train,1,ly);
        matrix<double> A(dim_train,dim_train,ly);
       
        derivative_function_covariance(hyperp, dir, dK);
        beta = dK*alpha;
        double res = alpha.scalar_product(beta);

        A = Kinv*dK;
        return -0.5*(res - A.trace());
    }

    void gradient_marginal_probability(double* hyperp, double *result)
    {
      //timer tm;
      //tm.start("gradient");
      if(!strcmp(approximation_type,"spgp")) 
      {
        for(int i=0; i<dimh; i++) result[i] = 0;
        int offset = dimh-num_pseudo_inputs*dimx;
        for(int i=0; i<offset; i++) 
        {
          set_pre_calculated_derivative_objects_spgp(hyperp,i);
          result[i] = derivative_marginal_probability_spgp(hyperp, i);
        }

        for(int k=0; k<dimx; k++) 
        {
          //the pseudo derivative only depend on k since they calculate the full matrix
          // all at once.  Therefore we start with offset and add k to get the right thing
          set_pre_calculated_derivative_objects_spgp(hyperp,offset + k);
          for(int j=0; j<num_pseudo_inputs; j++) 
          {
            int idx = offset + k + dimx*j;
            result[idx] = derivative_marginal_probability_spgp(hyperp, idx);
          }
        }
        global_sum(result,dimh);
        //tm.stop();
      }
      else {
        for(int i=0; i<dimh; i++) 
        {
          result[i] = derivative_marginal_probability_full(hyperp, i);
        }
      }
      //tm.stop();
    }
   
    void gradient_marginal_probability_pre_calc(double* hyperp, double *result)
    {
      if(!strcmp(approximation_type,"spgp")) 
      {
        //timer tm;
        for(int i=0; i<dimh; i++) result[i] = 0;
        int offset = dimh-num_pseudo_inputs*dimx;
        for(int i=0; i<offset; i++) 
        {
          set_pre_calculated_derivative_objects_spgp(hyperp,i);
          result[i] = derivative_marginal_probability_spgp(hyperp, i);
        }

        for(int k=0; k<dimx; k++) 
        {
          //the pseudo derivative only depend on k since they calculate the full matrix
          // all at once.  Therefore we start with offset and add k to get the right thing
          set_pre_calculated_derivative_objects_spgp(hyperp,offset + k);
          for(int j=0; j<num_pseudo_inputs; j++) 
          {
            int idx = offset + k + dimx*j;
            result[idx] = derivative_marginal_probability_spgp(hyperp, idx);
          }
        }
        global_sum(result,dimh);
      }
      else 
      {
           for(int i=0; i<dimh; i++) result[i] = derivative_marginal_probability_pre_calc_full(hyperp, *Kinv_pre_calc, *beta_pre_calc, i);
      }
    }

    /* transformations for minimization procedure */
    double minimizer_transformation_x(double x, int i, bool isInverse)
    {    
           double z;
         if(std::isnan(x) || std::isinf(x)) 
         {
           printf("x %0.15e %i %s\n",x,i,(isInverse) ? "true" : "false");
           graceful_exit(1,"x is NaN in minimizer transformation");
         }
         if(!strcmp(approximation_type,"spgp"))
         {
           /* do not transform pseudo inputs */
           int offset = dimh-num_pseudo_inputs*dimx;
           int n = i - offset;
           int idx = n / dimx;
           int k = n - idx*dimx;
           if( i >= offset ) 
           {
           //        return x;
             if(isInverse) 
             {
               if(std::abs(x - double(0.5)*(xmin[k]+xmax[k])) < std::numeric_limits<double>::epsilon()) return double(0);
               if( minimizer_transformation_df(x,i) < double(0))
                 z = (xmin[k]-xmax[k]+double(2)*sqrt(std::abs(-xmax[k]*xmin[k]+xmin[k]*x+xmax[k]*x-x*x)))/(xmin[k]+xmax[k]-double(2)*x);
               else
                 z = (xmin[k]-xmax[k]-double(2)*sqrt(std::abs(-xmax[k]*xmin[k]+xmin[k]*x+xmax[k]*x-x*x)) )/(xmin[k]+xmax[k]-double(2)*x);
               if(std::isnan(z) || std::isinf(z)) graceful_exit(1,"NaN in minimizer transformation");
             }
             else
             {
               z =  (xmax[k]-xmin[k])*x/(double(1)+x*x)+double(0.5)*(xmax[k]+xmin[k]);
               if(std::isnan(z) || std::isinf(z)) graceful_exit(1,"NaN in minimizer transformation");
               return z;
             }
           }
         }

         //don't let the noise go below the nugget
         if(!strcmp(kernel_type,"ARD_with_noise"))
         {
           if( i == dimx +1)
           {
             z =  (isInverse) ? log(std::abs(x-sqrt(kernel_nugget))) : exp(x)+sqrt(kernel_nugget);
             if(std::isnan(z) || std::isinf(z)) graceful_exit(1,"out NaN in minimizer transformation");
             return z;
           }
         }
         z =  (isInverse) ? log(std::abs(x)) : exp(x);
         if(std::isnan(z) || std::isinf(z)) graceful_exit(1,"out NaN in minimizer transformation");
         return z;
    }
    
    double minimizer_transformation_df(double x, int i)
    {
      if(!strcmp(approximation_type,"spgp"))
      {
        /* do not transform pseudo inputs */
        int offset = dimh-num_pseudo_inputs*dimx;
        int n = i - offset;
        int idx = n / dimx;
        int k = n - idx*dimx;
        if( i >= (dimh-num_pseudo_inputs*dimx) )
        {
          //return double(1);
          return (xmin[k]-xmax[k])*(x*x-double(1))/( (double(1)+x*x)*(double(1)+x*x) );
        }
      }
      return exp(x); // dx'/dx
    }
    
    double maximize_marginal_probability_full(double* x0, double stepsize, int maxiter, double tol, double tol_gradient)
    {
      gsl_multimin_function_fdf my_func;
      my_func.n = dimh;
      my_func.f = &gpr_gsl_function;
      my_func.df = &gpr_gsl_gradient;
      my_func.fdf = &gpr_gsl_fdf;
      my_func.params = (void *)this;
      double res;
     
      /* take log of initial guess to start */
      for(int i=0; i<dimh; i++) {
        x0[i] = minimizer_transformation_x(x0[i],i,true);
      }
      res = multi_function_minimizer(&my_func,x0,stepsize,maxiter,tol,tol_gradient,wallclock,max_time,verbose);
      
      /* take exponential to transform back hyperparameters */
      for(int i=0; i<dimh; i++) hyper_parameters[i] = minimizer_transformation_x(x0[i],i,false);

      /* set pre_calculate objects to be used later in making predictions */
      set_pre_calculated_objects(hyper_parameters);

      print_hyper_parameters();
      
      return res;
    }
    
    // returns f_min x0 -> xmin
    double  maximize_marginal_probability_spgp(double* x0, double stepsize, int maxiter, double tol, double tol_gradient)
    {
      gsl_multimin_function_fdf my_func;
      my_func.n = dimh;
      my_func.f = &gpr_gsl_function;
      my_func.df = &gpr_gsl_gradient;
      my_func.fdf = &gpr_gsl_fdf;
      my_func.params = (void *)this;
      double res;
     
      /* take log of initial guess to start */
      for(int i=0; i<dimh; i++) 
        x0[i] = minimizer_transformation_x(x0[i],i,true);
      
      res = multi_function_minimizer(&my_func,x0,stepsize,maxiter,tol,tol_gradient,wallclock,max_time,verbose);
      
      /* take exponential to transform back hyperparameters */
      for(int i=0; i<dimh; i++) hyper_parameters[i] = minimizer_transformation_x(x0[i],i,false);

      /* set pre_calculate objects to be used later in making predictions */
      set_pre_calculated_objects(hyper_parameters);

      print_hyper_parameters();
      
      return res;
    }
    
    double  maximize_marginal_probability(double* x0, double stepsize, int maxiter, double tol, double tol_gradient)
    {
      if(!strcmp(approximation_type,"spgp")) {
        return maximize_marginal_probability_spgp(x0,stepsize,maxiter,tol,tol_gradient);
      }
      else {
        return maximize_marginal_probability_full(x0,stepsize,maxiter,tol,tol_gradient);
      }
    }

    double  maximize_marginal_probability(double stepsize, int maxiter, double tol, double tol_gradient)
    {
     /* Set the initial guess to the value of the hyper-parameters. *
      * These are set using initally to something reasonable unless *
      * the user specifiies "none" for preprocessing                */
      double x0[dimh], res;
      for(int i=0; i<dimh; i++) x0[i] = hyper_parameters[i]; 
      res = maximize_marginal_probability(x0,stepsize,maxiter,tol,tol_gradient);
      return res;
    }
    
    void make_prediction_full(double* input,  matrix<double>& predictive_mean, matrix<double>& predictive_covariance, bool isCovariance=false)
    {

      int num_of_inputs = predictive_mean.rows;
      matrix<double> KS_K(num_of_inputs,dim_train,ly);
     
      /* set input data */
      vector **input_data;
      input_data = new vector*[num_of_inputs];
      for(int i=0; i<num_of_inputs; i++) input_data[i] = new vector(dimx);
      fill_training_vectors(input, input_data, num_of_inputs); 
     
      /* K*K */
      for(int j=0; j<KS_K.lcols; j++)
      for(int i=0; i<KS_K.lrows; i++)
      {
        int gi = ly.localToGlobalRow(i);
        int gj = ly.localToGlobalCol(j);
        KS_K(i,j) = squared_exponential_ARD(hyper_parameters, *input_data[gi], *train_data[gj]);
      }
       
      predictive_mean = KS_K*(*beta_pre_calc);
      apply_target_transformation(predictive_mean,true);

      if(isCovariance)
      {
        matrix<double> K_KS(dim_train,num_of_inputs,ly);
        matrix<double> KS_KS(num_of_inputs,num_of_inputs,ly);
        matrix<double> tmp1(num_of_inputs, dim_train, ly);
        /* this computes K^-1 and for that reason is very expensive when you want the covariance */
        set_pre_calculated_objects(hyper_parameters);

        /* KK* */
        for(int j=0; j<K_KS.lcols; j++)
          for(int i=0; i<K_KS.lrows; i++)
          {
            int gi = ly.localToGlobalRow(i);
            int gj = ly.localToGlobalCol(j);
            K_KS(i,j) = squared_exponential_ARD(hyper_parameters, *train_data[gi], *input_data[gj]);
          }


        /* K*K* */
        for(int j=0; j<KS_KS.lcols; j++)
          for(int i=0; i<KS_KS.lrows; i++)
          {
            int gi = ly.localToGlobalRow(i);
            int gj = ly.localToGlobalCol(j);
            KS_KS(i,j) = squared_exponential_ARD(hyper_parameters, *input_data[gi], *input_data[gj]);
          }

        tmp1 = KS_K*(*Kinv_pre_calc);
        predictive_covariance = tmp1*K_KS;
        predictive_covariance = KS_KS -  predictive_covariance;
      
        /* apply transformation due to normalization of targets */
        apply_covariance_transformation(predictive_covariance);
      }
      for(int i=0; i<num_of_inputs; i++) delete input_data[i];
      delete [] input_data;
    }
    
    void make_prediction_spgp(double* input,  matrix<double>& predictive_mean, matrix<double>& predictive_covariance, bool isCovariance=false)
    {
      int num_of_inputs = predictive_mean.rows;
      matrix<double> KSM(num_of_inputs,num_pseudo_inputs,ly);
      
      /* set input data */
      vector **input_data;
      input_data = new vector*[num_of_inputs];
      for(int i=0; i<num_of_inputs; i++) input_data[i] = new vector(dimx);
      fill_training_vectors(input, input_data, num_of_inputs); 
     
      /* K*MK */
      double *p = hyper_parameters + dimh - dimx*num_pseudo_inputs;
      for(int j=0; j<KSM.lcols; j++)
      for(int i=0; i<KSM.lrows; i++)
      {
        int gi = ly.localToGlobalRow(i);
        int gj = ly.localToGlobalCol(j);
        KSM(i,j) = squared_exponential_ARD(hyper_parameters, input_data[gi]->d, p+dimx*gj);
      }
      predictive_mean = KSM*(*AKy);
      apply_target_transformation(predictive_mean,true);
      for(int i=0; i<num_of_inputs; i++) delete input_data[i];
      delete [] input_data;
    }

    void make_prediction(double* input,  matrix<double>& predictive_mean, matrix<double>& predictive_covariance, bool isCovariance=false)
    {
      if(!strcmp(approximation_type, "spgp"))
        make_prediction_spgp(input,predictive_mean,predictive_covariance,isCovariance);
      else
        make_prediction_full(input,predictive_mean,predictive_covariance,isCovariance);
    }
    
    void make_prediction(const char* input_filename, int num_inputs, const char* target_filename, const char* target_covariance_filename = NULL)
    {
      bool isCovariance;
      (target_covariance_filename == NULL) ? isCovariance = false : isCovariance = true;
      FILE *fp = NULL;
      double* inputs;
      int error = 0;
      inputs = new double[num_inputs*dimx];
      if(ly.rank == 0)
      { 
        fp = fopen(input_filename, "r");
        if(fp == NULL) error = 1;
      }
      graceful_exit(error, "Could not open prediction input file");
      
      if(ly.rank == 0){
        fread(inputs, sizeof(double), num_inputs*dimx, fp);
        fclose(fp);
      }
      broadcast(inputs, dimx*num_inputs);
      apply_coordinate_transformation(inputs, num_inputs);
      matrix<double> mean(num_inputs, 1, ly);
      matrix<double>* cov = NULL;
      if(isCovariance) cov = new matrix<double>(num_inputs, num_inputs, ly);

      make_prediction(inputs, mean, *cov, isCovariance);
      mean.write(target_filename);
      if(isCovariance)
      {
        cov->write(target_covariance_filename);
        delete cov;
      }
      delete [] inputs;
    }
    
    void make_prediction(double* pred_inputs, int num_inputs, double* prediction_targets)
    {
      double prediction_inputs[num_inputs*dimx];
      for(int i=0; i<num_inputs*dimx; i++) prediction_inputs[i] = pred_inputs[i];
      broadcast(prediction_inputs, dimx*num_inputs);
      apply_coordinate_transformation(prediction_inputs, num_inputs);
      matrix<double> mean(num_inputs, 1, ly);
      matrix<double> *cov = NULL;
      make_prediction(prediction_inputs, mean, *cov,false);
      for(int i=0; i<num_inputs; i++) prediction_targets[i] = 0;

      for(int j=0; j<mean.lcols; j++)
      for(int i=0; i<mean.lrows; i++) 
      {
        int gi = ly.localToGlobalRow(i);
        prediction_targets[gi] = mean(i,0);
      }
      for(int i=0; i<num_inputs; i++) global_sum(prediction_targets[i]);
    }

    void set_pre_calculated_derivative_objects_spgp(double *hyperp, int dir)
    {
      //timer tm;
      //tm.start("pder");
      matrix<double> dotKMMT(num_pseudo_inputs,num_pseudo_inputs,ly);
      if(isNoise && dir == dimx + 1) return;

      derivative_function_covariance_ARD_MM_spgp(hyperp, dir, *dotKMM); 
      derivative_function_covariance_ARD_NM_spgp(hyperp, dir, *dotKNM);
      diag_multiply(*gh,*dotKNM,*bdotKNM,true);
      //tm.stop();
    }
    
    void check_nan(matrix<double>& m, char *msg)
    {
        bool isNan = false;
        for(int j=0; j<m.lcols; j++)
        for(int i=0; i<m.lrows; i++)
          if(std::isnan(m(i,j)) || std::isinf(m(i,j))) isNan = true;
        if(isNan) graceful_exit(1,msg);
    }

    void set_pre_calculated_objects(double *hyperp)
    {
      if(!strcmp("spgp",approximation_type))
      {
        //timer tm;
        //tm.start("spgp");
        double noise = hyperp[dimx+1]*hyperp[dimx+1];
        if(!isNoise) noise = double(1);

        /* KMM^1/2 */
        function_covariance_ARD_MM_spgp(hyperp,*KMM);
        function_covariance_ARD_NM_spgp(hyperp,*KNM);
        *FKMM = *KMM;
//KMM->write("kmm.ck");
        KMM->choleski_lower();
        *KMMInv = *KMM;
        KMMInv->inverse_choleski();
//KMMInv->write("kmmi.ck");

        /* gamma */
        *bKNMKMMInv = (*KNM)*(*KMMInv);
        function_covariance_ARD_gamma_spgp(hyperp,*KNM,*bKNMKMMInv,*g);
        for(int i=0; i<gh->lrows; i++) (*gh)(i,0) = sqrt((*g)(i,0));
//gh->write("g.ck");
        diag_multiply(*gh,*KNM,*bKNM,true);
        diag_multiply(*g, *output_vector, *ybar,true);
        diag_multiply(*gh, *output_vector, *ybbar,true);
        diag_multiply(*gh,*bKNMKMMInv,*bKNMKMMInv,true);
        diag_multiply(*gh,*bKNMKMMInv,*bbKNMKMMInv,true);

        /* A^-1/2 */
        AInv->transposeAxB(*bKNM,*bKNM,*AInv); //O(Nm2)
        *MM = (*KMMInv)*(*KMM);
//MM->write("mm.ck");
        *A = (*AInv)*(*MM);
        MM->transposeAxB(*MM,*A,*AInv);
        AInv->transpose(*A);
        *AInv = *AInv + *A;
        *AInv *= double(0.5);
#if 0 
        *A = (*AInv);
        KMMInv->write("is.ck");
        *A = ((*KMMInv)*(*AInv)*(*KMMInv))*(*KMM);
        MM->transposeAxB(*KMM,*A,*AInv);
#endif


//AInv->write("true.ck");
        A->Id();
        *A *= noise;
        *A += *AInv;
        A->choleski_lower();
        *AInv = *A;
        AInv->inverse_choleski();
//A->write("ai.ck");
        *MM1 = (*MM)*(*AInv);
        AInv->AxtransposeB(*MM1,*MM,*AInv);

        *bKNMA = (*bKNM)*(*AInv); //O(Nm2)
        bKNM->transposeAxB(*bKNMA,*ybbar,*AKy);
        for(int i=0; i<num_pseudo_inputs; i++) gAKy[i] = double(0);
        for(int i=0; i<AKy->lrows; i++) 
        {
          int gi = ly.localToGlobalRow(i);
          gAKy[gi] = (*AKy)(i,0);
        }
        global_sum(gAKy,num_pseudo_inputs);
        *KAKy = (*bKNM)*(*AKy);
        *KAKyybar = *KAKy - *ybbar;
        diag_spgp4(*bKNMA,*bKNM,*KAK);
        bKKK->transposeAxB(*bKNMKMMInv,*bKNMKMMInv,*bKKK);
        bbKKK->transposeAxB(*bbKNMKMMInv,*bbKNMKMMInv,*bbKKK);
        diag_multiply(*KAK,*bKNMKMMInv,*NM);
        bKKKA->transposeAxB(*NM,*bKNMKMMInv,*bKKKA);
        diag_multiply(*ybbar,*bKNMKMMInv,*KKy);
        bKKKy->transposeAxB(*KKy,*KKy,*bKKKy);

        diag_multiply(*KAKy,*bKNMKMMInv,*KKy1);
        bKKKy1->transposeAxB(*KKy1,*KKy1,*bKKKy1);
        bKKKy2->transposeAxB(*KKy,*KKy1,*bKKKy2);
        //check_nan(*KMM, "KMM");
        //check_nan(*KMM, "KNM");
        //check_nan(*KMM, "KMMInv");
        //check_nan(*A, "A");
        //check_nan(*AInv, "A");
        //tm.stop();
      }
      else
      {
        function_covariance(hyperp,*K_pre_calc);
        *Choleski_pre_calc = *K_pre_calc;
        Choleski_pre_calc->choleski_lower();
        *Kinv_pre_calc = *Choleski_pre_calc;
        /* compute the log determinant */
        log_determinant_pre_calc = Choleski_pre_calc->log_determinant();
        solve_choleski_system(*Choleski_pre_calc, *output_vector, *beta_pre_calc);
        Kinv_pre_calc->inverse_choleski();
      }
    }

    void print_hyper_parameters()
    {
      if(ly.rank == 0)
      {
        if(!strcmp(kernel_type,"ARD_with_noise"))
        {
          printf("\nhyper-parameter %s (sqrt(var(x)) = characteristic length)\n",kernel_type);
          for(int i=0; i<69; i++) printf("-");
          printf("\n");

          for(int i=0; i<dimx; i++) printf("l(x_%i)  = %+0.15e\n",i,1/hyper_parameters[i]);
          printf(  "std(f)      = %+0.15e\n",hyper_parameters[dimx]);
          printf(  "std(noise)  = %+0.15e\n",hyper_parameters[dimx+1]);

         // if(!strcmp(approximation_type,"spgp"))
         // {
         //   for(int i=dimx+2; i<dimh; i++) 
         //   {
         //     int idx = i - dimx - 2;
         //     printf(  "x_%i[%i]  = %+0.15e\n",idx/dimx,idx - (idx/dimx)*dimx,hyper_parameters[i]);
         //   }
         // }
        }
        else if(!strcmp(kernel_type,"ARD_without_noise"))
        {
          printf("\nhyper-parameter %s (sqrt(var(x)) = characteristic length)\n",kernel_type);
          for(int i=0; i<72; i++) printf("-");
          printf("\n");
          for(int i=0; i<dimx; i++) printf("l(x_%i)   = %+0.15e\n",i,1/hyper_parameters[i]);
          printf(  "std(f)      = %+0.15e\n",hyper_parameters[dimx]);
        }
        else if(!strcmp(kernel_type,"ARD_NIGP"))
        {
          printf("\nhyper-parameter %s (sqrt(var(x)) = characteristic length)\n",kernel_type);
          for(int i=0; i<69; i++) printf("-");
          printf("\n");

          for(int i=0; i<dimx; i++) printf("l(x_%i)  = %+0.15e\n",i,1/hyper_parameters[i]);
          printf("std(f)      = %+0.15e\n",hyper_parameters[dimx]);
          printf("std(noise)  = %+0.15e\n",hyper_parameters[dimx+1]);
          for(int i=dimx+2; i<dimh; i++) printf("std(noise_%i)  = %+0.15e\n",i,hyper_parameters[i]);
        }
        else
        {
          int error = 1;
          graceful_exit(error, "Kernel specification is invalid");
        }
        printf("\n");
      }
    }

}; //end gaussian_process_regression


/* functions to use conjugate gradient with gsl framework
   they constrain the variances to being positive by transforming
   sigma -> simga^2                                               */
double gpr_gsl_function(const gsl_vector *x, void *params)
{
   timer tm;
   //params is a pointer to gpr object
    gaussian_process_regression* gpr;
    gpr = (gaussian_process_regression*)params; 
    double xx[gpr->dimh];
    for(int i=0; i<gpr->dimh; i++)
    {
      xx[i] = gpr->minimizer_transformation_x(gsl_vector_get(x,i),i,false);
    }
    gpr->set_pre_calculated_objects(xx);
    return gpr->marginal_probability(xx);
}

void gpr_gsl_gradient(const gsl_vector *x, void *params, gsl_vector* df)
{
  timer tm;
  //params is a pointer to gpr object
  gaussian_process_regression* gpr;
  gpr = (gaussian_process_regression*)params;
  double result[gpr->dimh], xx[gpr->dimh], xs[gpr->dimh];
  for(int i=0; i<gpr->dimh; i++)
  {
    xx[i] = gsl_vector_get(x,i);
    xs[i] = gpr->minimizer_transformation_x(xx[i],i,false);
  }
  gpr->set_pre_calculated_objects(xs);
  gpr->gradient_marginal_probability(xs, result);
  for(int i=0; i<gpr->dimh; i++) {
    if(std::isnan(result[i]) || std::isinf(result[i])) graceful_exit(1,"Error gradient is NaN!");
    gsl_vector_set(df,i,result[i]*gpr->minimizer_transformation_df(xx[i],i));
  }
}

void gpr_gsl_fdf(const gsl_vector* x, void* params, double *f, gsl_vector* df)
{
  timer tm;
  gaussian_process_regression* gpr;
  gpr = (gaussian_process_regression*)params;
  //transformation to constrain to positive values
  double xx[gpr->dimh], xs[gpr->dimh], result[gpr->dimh];
  for(int i=0; i<gpr->dimh; i++)
  {
    xx[i] = gsl_vector_get(x,i);
    xs[i] = gpr->minimizer_transformation_x(xx[i],i,false);
  }
  gpr->set_pre_calculated_objects(xs);
  *f = gpr->marginal_probability_pre_calc(xs);
  gpr->gradient_marginal_probability_pre_calc(xs,result);
  for(int i=0; i<gpr->dimh; i++)
  {
    if(std::isnan(result[i]) || std::isinf(result[i])) graceful_exit(1,"Error gradient is NaN!");
    gsl_vector_set(df,i,result[i]*gpr->minimizer_transformation_df(xx[i],i));
  }
}
#endif
