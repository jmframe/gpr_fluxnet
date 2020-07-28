#ifndef H_SCALAPACK
#define H_SCALAPACK

extern "C" {
#ifndef MPI_DISABLED
int numroc_(int*, int*, int*, int*, int*);
void sl_init_(int*, int*, int*);
void descinit_(int*, int*, int*, int*, int*, int*, int*, int*, int*, int*);
void blacs_gridinfo_(int*, int*, int*, int*, int*);
void blacs_pinfo_( int*, int*);
int indxl2g_( int* indxloc, int* nb, int* iproc, int* isrcproc, int* nproc);
void Cblacs_get(int,int, int*);
void Cblacs_gridinit(int*,char*,int,int);
void Cblacs_gridexit(int);
void blacs_pinfo_(int*,int*);
int indxl2g_(int* indxloc, int* nb, int* iproc, int* isrcproc, int* nproc);
int indxg2p_(int* INDXGLOB, int* NB, int* IPROC, int* ISRCPROC, int *NPROCS);
void pdgemm_( char*  TRANSA, char*  TRANSB, int *   M, int *   N, int *   K, double *  ALPHA, double *  A, int *   IA,
    int *   JA,
    int *   DESCA,
    double *  B,
    int *   IB,
    int *   JB,
    int *   DESCB,
    double *  BETA,
    double *  C,
    int *   IC,
    int *   JC,
    int *   DESCC 
    );
void pdgetrf_(int*, int*, double*, int*,int*,int*, int*, int*);
void pdgetri_( int*, double*, int*, int*, int*, int*, double*, int*,int*, int*, int*);
void pdtranu_(int* m, int *n, double *alpha, double *a, int *ia, int *ja, int *desca, double *beta, double *c, int *ic, int *jc, int *descc);
void blacs_gridexit_(int *);
void pdgeadd_(char* TRANS, int *M, int *N, double *ALPHA, double *A, int *IA, int *JA, int *DESCA, double *BETA, double *C, int *IC, int *JC, int *DESCC);
void pdgemr2d_(int *m, int *n, double *A, int *IA, int *JA, int *descA, double *B, int *IB, int *JB, int *descB, int *gcontext);
void pdpotrf_( char* UPLO, int* N, double* A, int *IA, int* JA, int* DESCA, int *INFO );
void pdgecon_( char* norm, int* N, double* A, int* IA, int* JA, int* DESCA, double* ANORM, double* RCOND, double* WORK, int* LWORK, int* IWORK, int *LIWORK, int* INFO );
double pdlange_(char* norm, int* M, int* N, double* A, int* IA, int* JA, int* DESCA, double* WORK );
void pdpotrs_( char* uplo, int *N, int* NRHS, double* A, int* IA, int* JA, int* DESCA, double* B, int* IB, int* JB, int* DESCB, int* INFO);
void pdpotri_(char* uplo, int* N, double*  A, int* IA, int* JA, int* DESCA, int* INFO );
void pdpocon_( char* uplo,int* N, double* A,int* IA,int* JA, int* DESCA, double* ANORM, double* RCOND, double* WORK,int* LWORK, int* IWORK, int* LIWORK, int* INFO);
void pdpoequ_(int* n, double* ap, int* ia, int* ja, int* desca, double* sr, double* sc, double* scond, double* amax, int* info);
#else
/* no MPI */
void dgemm_( char*  TRANSA, char*  TRANSB, int *   M, int *   N, int *   K, double *  ALPHA, double *  A, int*  lda,
    double *  B,
    int *   ldb,
    double *  BETA,
    double *  C,
    int *   ldc
    );
double  dlange_(char* norm, int* M, int* N, double* A, int* lda, double* WORK );
void dgeadd_(char* TRANS, int *M, int *N, double *ALPHA, double *A, int *lda, double *BETA, double *C, int *ldc);
void dpotrs_(char* uplo, int *N, int* NRHS, double* A, int* lda, double* B, int* ldb, int* INFO);
void dpotri_(char* uplo, int* N, double*  A, int* lda, int* INFO );
void dpotrf_(char* UPLO, int* N, double* A, int *lda, int *INFO );
void dpocon_(char* uplo,int* n, double* a,int* lda, double* anorm, double* rcond, double* work, int* iwork, int* info);
void dgetrf_(int* m, int* n, double* ap, int* lda,int* ipiv,int* info);
void dpoequ_(int *n, double *ap, int* lda, double* s, double* scond, double* amax, int* info);
#endif
}
#endif
