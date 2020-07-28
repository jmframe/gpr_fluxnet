/*
craig pelissier
October 2017
Craig.s.pelissier@nasa.gov
*/
#include <stdio.h>
#include <stdlib.h>
#include "comm.h"
#include <stdexcept>
#include <string.h>
#include <complex.h>
using namespace std;
/****************************************************************
  MPI communication routines
 ****************************************************************/
void barrier() { }

template<typename T>
void broadcast(T* buffer, size_t size) { }
template void broadcast<double>(double* buffer, size_t size);
template void broadcast<complex<double> >(complex<double>* buffer, size_t size);
template void broadcast<float>(float* buffer, size_t size);
template void broadcast<complex<float> >(complex<float>* buffer, size_t size);
template void broadcast<int>(int* buffer, size_t size);
template void broadcast<short>(short* buffer, size_t size);
template void broadcast<char>(char* buffer, size_t size);

template<typename T>
void global_sum(T& local_data) { }
template void global_sum<complex<double> >(complex<double>& local_data);
template void global_sum<complex<float> >(complex<float>& local_data);
template void global_sum<double>(double& local_data);
template void global_sum<float>(float& local_data);
template void global_sum<int>(int& local_data);

template<typename T>
void global_sum(T* local_data, int size) { }
template void global_sum<complex<double> >(complex<double>* local_data, int size);
template void global_sum<complex<float> >(complex<float>* local_data, int size);
template void global_sum<double>(double* local_data, int size);
template void global_sum<float>(float* local_data, int size);
template void global_sum<int>(int* local_data, int size);

template<typename T>
void global_prod(T& local_data) {}
template void global_prod<complex<double> >(complex<double>& local_data);
template void global_prod<complex<float> >(complex<float>& local_data);
template void global_prod<double>(double& local_data);
template void global_prod<float>(float& local_data);
template void global_prod<int>(int& local_data);

int get_comm_size() {
  return 1;
}

rank_t get_rank() {
  return 0;
}

static void time_stamp(const char *msg)
{
  time_t time_stamp;

  if(get_rank()==0)
  {
    time(&time_stamp);
    fprintf(stderr, "%s: %s\n", msg, ctime(&time_stamp));
    fflush(stderr);
  }
}

void start_parallel(int &argc, char **&argv) {
  if(get_rank() == 0) fprintf(stderr, "succesfully started %i processes\n", 1);
}

void graceful_exit(int error, const char* msg)
{
  broadcast(&error,1);
  if (error != 0) {
    if(get_rank() == 0) fprintf(stderr,"gpr error: %s\n", msg);
  }
}

void end_parallel() 
{
  time_stamp("Exit");
  fflush(stderr);
}

