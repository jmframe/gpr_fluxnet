/*
craig pelissier
April 2013
Craig.s.pelissier@nasa.gov
*/
#include "mpi.h"
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
void barrier() {
  MPI_Barrier(MPI_COMM_WORLD);
}

//template the MPI_Datatypes to allow template functions of MPI calls
template<typename T> MPI_Datatype get_mpi_type() {return 0;}
template <> MPI_Datatype get_mpi_type<char>()    {return MPI_CHAR;}
template <> MPI_Datatype get_mpi_type<int>()     {return MPI_INT;}
template <> MPI_Datatype get_mpi_type<short>()   {return MPI_SHORT;}
template <> MPI_Datatype get_mpi_type<float>()   {return MPI_FLOAT;}
template <> MPI_Datatype get_mpi_type<double>()  {return MPI_DOUBLE;}
template <> MPI_Datatype get_mpi_type<complex<double> >()  {return MPI::DOUBLE_COMPLEX;}
template <> MPI_Datatype get_mpi_type<complex< float> >()  {return MPI::COMPLEX;}

/*some simple communication routines...if more are needed
  should move to antoher file */

template<typename T>
void broadcast(T* buffer, size_t size)
{
  MPI_Bcast(buffer, size, get_mpi_type<T>(), 0, MPI_COMM_WORLD);
}
template void broadcast<double>(double* buffer, size_t size);
template void broadcast<complex<double> >(complex<double>* buffer, size_t size);
template void broadcast<float>(float* buffer, size_t size);
template void broadcast<complex<float> >(complex<float>* buffer, size_t size);
template void broadcast<int>(int* buffer, size_t size);
template void broadcast<short>(short* buffer, size_t size);
template void broadcast<char>(char* buffer, size_t size);

template<typename T>
void global_sum(T& local_data)
{
  T work; //dummy to store result  MPI_Allreduce(&local_data, &work,
  MPI_Allreduce(&local_data, &work, 1, get_mpi_type<T>(), MPI_SUM, MPI_COMM_WORLD);
  local_data = work; //update
}
template void global_sum<complex<double> >(complex<double>& local_data);
template void global_sum<complex<float> >(complex<float>& local_data);
template void global_sum<double>(double& local_data);
template void global_sum<float>(float& local_data);
template void global_sum<int>(int& local_data);

template<typename T>
void global_sum(T* local_data, int len)
{
  T work[len]; //dummy to store result  MPI_Allreduce(&local_data, &work,
  MPI_Allreduce(local_data, work, len, get_mpi_type<T>(), MPI_SUM, MPI_COMM_WORLD);
  for(int i=0; i<len; i++) local_data[i] = work[i]; //update
}
template void global_sum<complex<double> >(complex<double>* local_data, int len);
template void global_sum<complex<float> >(complex<float>* local_data, int len);
template void global_sum<double>(double* local_data, int len);
template void global_sum<float>(float* local_data, int len);
template void global_sum<int>(int* local_data, int len);

template<typename T>
void global_prod(T& local_data)
{
  T work; //dummy to store result  MPI_Allreduce(&local_data, &work,
  MPI_Allreduce(&local_data, &work, 1, get_mpi_type<T>(), MPI_PROD, MPI_COMM_WORLD);
  local_data = work; //update
}
template void global_prod<complex<double> >(complex<double>& local_data);
template void global_prod<complex<float> >(complex<float>& local_data);
template void global_prod<double>(double& local_data);
template void global_prod<float>(float& local_data);
template void global_prod<int>(int& local_data);

static void mpi_error(MPI_Comm *comm, int *stat, ...) 
{ 
  int len; 
  char err_string[MPI_MAX_ERROR_STRING]; 

  fprintf(stderr, "MPI error: %d\n", *stat); 
  MPI_Error_string(*stat, err_string, &len); 
  fprintf(stderr, "%s\n", err_string); 
  abort(); 
} 

int get_comm_size()
{
  int size;
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  return size;
}

rank_t get_rank()
{
  rank_t rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  return rank;
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

extern "C"
{
void start_parallel(int &argc, char **&argv)
{
  int flag, size;
  MPI_Comm comm;
  MPI_Errhandler errhandler;

  flag = MPI_Init(&argc, &argv);
  comm = MPI_COMM_WORLD;
  if (flag) mpi_error(&comm, &flag);
  flag =  MPI_Comm_create_errhandler(mpi_error, &errhandler);
  if (flag) mpi_error(&comm, &flag);
  flag =  MPI_Comm_set_errhandler(MPI_COMM_WORLD, errhandler);
  if (flag) mpi_error(&comm, &flag);

  time_stamp("Start");
  fflush(stdout);
  MPI_Comm_size(comm, &size);
  if(get_rank() == 0) fprintf(stderr, "succesfully started %i processes\n", size);
}
}

void graceful_exit(int error, const char* msg)
{
  broadcast(&error,1);
  if (error != 0) {
    if(get_rank() == 0) fprintf(stderr,"gpr error: %s\n", msg);
    MPI_Finalize();
    exit(error);
  }
}

extern "C"
{
void end_parallel() 
{
  time_stamp("Exit");
  fflush(stderr);
  MPI_Finalize();
}
}
