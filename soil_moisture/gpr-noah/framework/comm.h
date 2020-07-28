#ifndef COMM_H
#define COMM_H

#ifndef MPI_DISABLED
#include "mpi.h"
#endif
#include <stdio.h>
#include <stdlib.h>
#include <vector>

typedef int rank_t;
/****************************************************
  communication routines
*****************************************************/
//communication basci information
#ifndef MPI_DISABLED
template<typename T> MPI_Datatype get_mpi_type();
#endif
void barrier();
int get_rank();
int get_comm_size();
void graceful_exit(int error, const char* msg);

extern "C"
{
//start/stop mpi world
void start_parallel(int &argc, char **&argv);
void end_parallel();
}

//global sum updates local_data (overwritten)...  
template <typename T> void global_sum(T& local_data);
template <typename T> void global_sum(T* local_data, int len);
template <typename T> void global_prod(T& local_data);
template <typename T> void broadcast(T* buffer, size_t size);
#endif //end header guard
