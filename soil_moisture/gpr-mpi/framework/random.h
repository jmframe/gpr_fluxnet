#ifndef H_RANDOM
#define H_RANDOM
#include "gsl/gsl_rng.h"
#include "scalapack_matrix.h"
#include "layout.h"
#include "comm.h"
#include "gsl/gsl_randist.h"

struct random_generator
{
  layout& ly;
  gsl_rng** rng_seeds;
  int size;
  int lsize;
  random_generator(int size, layout& ly, int seed=0) : ly(ly), size(size) 
  {
    matrix<double> tmp(size, 1, ly);
    lsize = tmp.lrows*tmp.lcols;
    rng_seeds = new gsl_rng*[lsize];
    for(int i=0; i<lsize; i++)
     rng_seeds[i] = gsl_rng_alloc(gsl_rng_ranlxd2); 

    //set seeds
    for(int j=0; j<tmp.lcols; j++)
    for(int i=0; i<tmp.lrows; i++)
    {
      int gi = ly.localToGlobalRow(i); 
      int gj = ly.localToGlobalCol(j);
      int lidx = i+tmp.lrows*j;
      unsigned int long seed_idx = gi + tmp.rows*gj + seed + 1;
      gsl_rng_set(rng_seeds[lidx], seed_idx); 
    }
  }

  ~random_generator() 
  {
    for(int i=0; i<lsize; i++)
     gsl_rng_free(rng_seeds[i]); 
  }
};
  

void pseudo_random_vector(random_generator& rand, matrix<double>& result)
{
  for(int j=0; j<result.lcols; j++)
    for(int i=0; i<result.lrows; i++)
    {
      int lidx = i+result.lrows*j;
      result(i,j) = gsl_rng_uniform(rand.rng_seeds[lidx]);
    }
}


struct multivariate_normal_distriubtion
{
  matrix<double>& mu;
  matrix<double>& sigma;
  random_generator rand;
  matrix<double> choleski;
  multivariate_normal_distriubtion(matrix<double>& mu, matrix<double>& sigma, int seed=0) : mu(mu), sigma(sigma), 
  rand(sigma.rows, sigma.ly, seed), choleski(sigma.rows,sigma.cols,sigma.ly)
  {
    choleski = sigma;
    choleski.choleski_lower();
  }
 
  void unit_covariance_gaussian_vector(matrix<double>& result)
  {
    for(int j=0; j<result.lcols; j++)
    for(int i=0; i<result.lrows; i++)
    {
      int lidx = i+result.lrows*j;
      result(i,j) = gsl_ran_ugaussian(rand.rng_seeds[lidx]);
    }
  } 

  void get(matrix<double>& result)
  {
    unit_covariance_gaussian_vector(result);
    result = choleski*result;
    result = result + mu;
  }
  ~multivariate_normal_distriubtion() {}

};
  
#endif
