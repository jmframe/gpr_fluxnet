/* Craig Pelissier
 * January 2015
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

   ~layout() {
     int isFinalized;
     MPI_Finalized(&isFinalized);
     if(!isFinalized) Cblacs_gridexit(context); 
   }

   void minimizeSV(double ratio)
   {
     int ix, iy;
     if(layout_dimension==1)
     {
       prows = commSize;
       pcols = 1;
     }
     else if(layout_dimension==2)
     {
       prows = commSize;
       pcols = 1;
       ix = 1;
      double stv = pcols+prows/ratio;
      double stv2 = 0;
      while(ix <= commSize)
      {
        if((commSize % ix) == 0) 
        {
          iy = commSize / ix;
          stv2 = iy + ix/ratio;
          if(stv2 < stv)
          {
            prows = ix;
            pcols = iy;
            stv = stv2;
          }
        }
        ix++;
       }
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
     int zero;
     zero = 0;
     char Row[4] = "Row";
     isrcproc = 0;
     minimizeSV(ratio);
     Cblacs_get(zero,zero,&context);
     Cblacs_gridinit(&context, Row, prows, pcols);
     blacs_gridinfo_(&context, &prows, &pcols, &myrow, &mycol);
     if(rank==0 && verbose) printf("%i X %i process grid created\n",prows,pcols);
   }

   int numrocRow(int rows) {
     int r = rows;
     int zero = 0;
     return numroc_(&r, &nbrows, &myrow, &zero, &prows);
   }
   
   int numrocCol(int cols) {
     int c = cols;
     int zero = 0;
     return numroc_(&c, &nbcols, &mycol, &zero, &pcols);
   }
   
   void descinit(int* desc, int rows, int cols) {
     int zero, info;
     zero = 0;
     int r = rows;
     int c = cols;
     int lda = std::max(1,numroc_(&r, &nbrows, &myrow, &isrcproc, &prows));
     descinit_(desc, &r, &c, &nbrows, &nbcols, &zero, &zero, &context, &lda, &info);
   }

   int localToGlobalRow(int localIdx)
   {
     int i = localIdx + 1;
     return indxl2g_(&i,&nbrows,&myrow,&isrcproc,&prows)-1;
   }

   int localToGlobalCol(int localIdx)
   {
     int i = localIdx + 1;
     return indxl2g_(&i,&nbcols,&mycol,&isrcproc,&pcols)-1;
   }
   int globalToProcessRow(int globalIdx)
   {
       int i = globalIdx + 1;
       return indxg2p_(&i, &nbrows, &myrow, &isrcproc, &prows);
   }
   
   int globalToProcessCol(int globalIdx)
   {
       int i = globalIdx + 1;
       return indxg2p_(&i, &nbcols, &mycol, &isrcproc, &pcols);
   }
   
   int numRowsOnProcess(int rows, int iRow)
   {
     int r = rows;
     int iarow = iRow;
     return numroc_(&r, &nbrows, &myrow, &iarow, &prows);
   }
   
   int numColsOnProcess(int cols, int iCol)
   {
     int c = cols;
     int iacol = iCol;
     return numroc_(&c, &nbcols, &mycol, &iacol, &pcols);
   }

};
#endif
