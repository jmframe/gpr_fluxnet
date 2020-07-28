/* Craig Pelissier
 * October 2017
 * This layout contains the abstract layout associated
 * with the distributed matrices
 ***********************************************************/

#include "comm.h"
#include "scalapack.h"

#ifndef LAYOUT_H
#define LAYOUT_H

struct layout
{
   int rank;
   int commSize;
   int context;
   int prows, pcols;
   int myrow, mycol;
   int nbrows, nbcols;
   int isrcproc;
   int layout_dimension; //dimension of layout 1D  = 1  2D = 2
   double ratio = 1;
   bool verbose;
   layout(int nbrows, int nbcols, int layout_dimension, double ratio = 1, bool verbose=true) :  
     nbrows(nbrows), nbcols(nbcols), layout_dimension(layout_dimension), ratio(ratio), verbose(verbose) {
     init();
   }

   ~layout() { }

   void minimizeSV()
   {
     if(layout_dimension==1) {
       prows = pcols = 1.0;
     }
     else if(layout_dimension==2) {
       prows = pcols = 1.0;
     }
     else
     {
       if(rank == 0)
         fprintf(stderr, "layout_dimension %i not supported\n", layout_dimension);
       exit(1);
     }
   }

   void init()
   {
     rank = get_rank();
     commSize = get_comm_size();
     isrcproc = 0;
     minimizeSV();
     if(rank==0 && verbose) printf("%i X %i process grid created\n",prows,pcols);
   }
   int numrocRow(int rows) {
     return rows;
   }
   
   int numrocCol(int cols) {
     return cols;
   }
   
   void descinit(int* desc, int rows, int cols) { }


   int localToGlobalRow(int localIdx) {
     return localIdx;
   }

   int localToGlobalCol(int localIdx) {
     return localIdx;
   }

   int globalToProcessRow(int globalIdx) {
     return globalIdx; 
   }
   
   int globalToProcessCol(int globalIdx) {
     return globalIdx; 
   }
   
   int numRowsOnProcess(int rows, int iRow) {
     return rows;
   }
   
   int numColsOnProcess(int cols, int iCol)
   {
     return cols;
   }

};
#endif
