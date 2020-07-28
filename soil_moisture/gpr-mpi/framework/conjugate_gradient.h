#ifndef H_CONJUGATE_GRADIENT
#define H_CONJUGATE_GRADIENT

#include <vector>
#include "gpr.h"
#include "gsl/gsl_multimin.h"
#include "gsl/gsl_vector.h"
#include "gsl/gsl_blas.h"
#include "timer.h"
#include "comm.h"

#define print0 if(get_rank()==0) printf

double multi_function_minimizer(gsl_multimin_function_fdf* func, double* x0, double stepsize, int maxiter, double tol, double tol_gradient, timer wc, 
    double max_time,  bool verbose = 1)
{
  bool is_overtime = false;
  int dim = func->n;
  const gsl_multimin_fdfminimizer_type *T;
  gsl_multimin_fdfminimizer *s;
  T = gsl_multimin_fdfminimizer_conjugate_pr;
  s = gsl_multimin_fdfminimizer_alloc(T, dim);
  
  //diagnostic containers
  gsl_vector *g1, *g2;
  double d1, d2;
  g1 = gsl_vector_alloc(dim);
  g2 = gsl_vector_alloc(dim);
  
  //inital guess
  gsl_vector *x;
  x = gsl_vector_alloc(dim);
  for(int i=0; i<dim; i++)
  {
    //print0("initial %0.15e\n", x0[i]);
    gsl_vector_set(x,i,x0[i]);
  }

  gsl_multimin_fdfminimizer_set(s, func, x, stepsize, tol);  
  int iter = 0;
  int status;
  gsl_vector_memcpy(g1,gsl_multimin_fdfminimizer_gradient(s));
  gsl_blas_ddot(g1,g1,&d2);
  if(verbose)
  {
    print0("iter. ");
    for(int i=0; i<dim; i++){
      print0("x[%i]",i);
      for(int i =0; i<14; i++) print0(" ");
    }
    print0("f(x)");
    for(int i =0; i<14; i++) print0(" ");
    print0("|gradient|\n");
    for(int i=0; i<18*(dim+2)+5; i++) print0("-");
    print0("\n");
  }
  do
  {
    iter++;

    if(verbose)
    {
      print0("%05i ", iter);
      for(int i=0; i<dim; i++)
        print0("%+0.10e ", gsl_vector_get(s->x,i));
       // print0("x[%i]=%+0.10e dx[%i]=%+0.10e ", i, gsl_vector_get(s->x,i), i, gsl_vector_get(s->dx, i));
      print0("%+0.10e ", s->f);
      print0("%+0.10e\n", sqrt(d2));
      fflush(stdout);
    }
    else
      print0("iter %05i: |gradient| = %+0.10e\n", iter, sqrt(d2));
    
    //check status
    status = gsl_multimin_fdfminimizer_iterate(s);
    gsl_vector_memcpy(g2,gsl_multimin_fdfminimizer_gradient(s));
    d1 =  gsl_blas_dnrm2(g1);

    if(status)
    {
      print0("error: %s |gradient| = %0.15e\n", gsl_strerror(status), d1);
      gsl_multimin_fdfminimizer_restart(s);
      break;
    }

    gsl_blas_ddot(g1,g2,&d1);
    gsl_blas_ddot(g2,g2,&d2);
    if(d1 > 0.1*d2)
    {
      //print0("iter %i: restarting ... insufficent orthogonality in the gradient\n", iter);
      //gsl_multimin_fdfminimizer_restart(s);
    }
    gsl_vector_memcpy(g1,g2);
    

//   print0("iter %i: norm of gradient = %0.15e\n", iter, sqrt(d2));
    status = gsl_multimin_test_gradient(s->gradient, tol_gradient);

    if (status == GSL_SUCCESS)
    {
      print0 ("iter %i: Minimum found: |gradient| = %0.15e\n", iter, sqrt(d2));
    }

    /* check if you are over the allowed time */
    if(max_time > 0)
    {
      wc.stop();
      if(wc.time > max_time) is_overtime = true;
    }
  }
  while (status == GSL_CONTINUE && iter < maxiter && is_overtime == false);

 if(get_rank() == 0 )
 { 
  if(iter == maxiter)
   fprintf(stderr, "Reached max number of iterations without convergence\n");

  if(is_overtime)
    fprintf(stderr, "Reached max time of %0.5e [s] without convergence\n", max_time);
 }

  for(int i=0; i<dim; i++)
    x0[i] = gsl_vector_get(s->x,i);
    
  gsl_multimin_fdfminimizer_free(s);
  gsl_vector_free(x);
  gsl_vector_free(g1);
  gsl_vector_free(g2);

  return s->f;

}

#undef print0
#endif
