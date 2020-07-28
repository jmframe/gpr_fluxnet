#include <functional>
#include <random>
#include <assert.h>
#include "scalapack_matrix.h"
#include "timer.h"
#include "comm.h"

class randDouble
{
  public:
    randDouble(double low, double high)
      :r(std::bind(std::uniform_real_distribution<>(low,high),std::default_random_engine(std::random_device{}()))){}

    double operator()(){ return r(); }

  private:
    std::function<double()> r;
};

using namespace std;
template<typename T>
void matrix<T>::Id()
{
  for(int j=0; j<lcols; j++)
    for(int i=0; i<lrows; i++)
    {
      int gi = ly.localToGlobalRow(i);
      int gj = ly.localToGlobalCol(j);
      (gi == gj) ? pMat[i+lrows*j] = double(1) : pMat[i+lrows*j] = double(0);

    }
}
template void matrix<double>::Id();

template<typename T>
void matrix<T>::printLocal()
{
  for(int i=0; i<lrows; i++)
  {
    for(int j=0; j<lcols; j++)
    {
      printf("%0.3e ", CHOP((*this)(i,j)));
    }
    printf("\n");
  }
}
template void matrix<double>::printLocal();
   
template<typename T>
void matrix<T>::print()
{
  T *g = new T[rows*cols];
  memset(g,0,sizeof(T)*rows*cols);
  copyLocalToGlobal(g);
  if(ly.rank==0)
  {
    for(int i=0; i<rows; i++)
    {
      for(int j=0; j<cols; j++)
      {
        printf("%0.6e ", CHOP(g[j+cols*i]));
      }
      printf("\n");
    }
  }
  fflush(stdout);
  barrier();
  delete [] g;
}
template void matrix<double>::print();
    

template<typename T>
void matrix<T>::clear()
{
  for(int i=0; i<lrows*lcols; i++) pMat[i] =  double(0);
}
template void matrix<double>::clear();
   
template<typename T>
void matrix<T>::uniformRandom()
{
  randDouble rd(0.2,1.0);
  for(int j=0; j<lcols; ++j)
    for(int i=0; i<lrows; ++i)
    {
      pMat[i+lrows*j]=rd();
    }
}
template void matrix<double>::uniformRandom();

template<typename T>    
void matrix<T>::copyLocalToGlobal(T* g)
{
  //put this in row major format
  for(int j=0; j<lcols; j++)
    for(int i=0; i<lrows; i++)
    {
      int gi = ly.localToGlobalRow(i);
      int gj = ly.localToGlobalCol(j);
      //switch to row major
      g[gj+cols*gi] = pMat[i+lrows*j];
      //printf("( %i, %i ) -> %lf %lf\n", gi,gj, pMat[j+lcols*i].real(), pMat[j+lcols].imag());
    }
  for(int i=0; i<rows*cols; i++) global_sum(g[i]);
}
template void matrix<double>::copyLocalToGlobal(double* g);
    
template<typename T>
#ifndef MPI_DISABLED
void matrix<T>::scalapack_pdgemm(const matrix<T>& a, const matrix<T>& b, matrix<T>& c, T calpha, T cbeta, char * transa, char* transb)
{
  int one = 1.0;
  int m = c.rows;
  int n = c.cols;
  int k = a.cols;
  T* ap = (double*)(a.pMat);
  T* bp = (double*)(b.pMat);
  T* cp = (double*)(c.pMat);
  T *alpha = (double*)&calpha;
  T *beta = (double*)&cbeta;
  if(strcmp(transa,"t") == 0 || strcmp(transa,"c")==0) { k=a.rows;}
  pdgemm_(transa, transb, &m,&n,&k,alpha,ap,&one,&one,a.desc,bp,&one,&one,b.desc,beta,cp,&one,&one,c.desc);
} 
#else
void matrix<T>::scalapack_pdgemm(const matrix<T>& a, const matrix<T>& b, matrix<T>& c, T calpha, T cbeta, char * transa, char* transb)
{
  int m = c.rows;
  int n = c.cols;
  int k = a.cols;
  int lda = a.rows;
  int ldb = b.rows;
  T* ap = (double*)(a.pMat);
  T* bp = (double*)(b.pMat);
  T* cp = (double*)(c.pMat);
  T *alpha = (double*)&calpha;
  T *beta = (double*)&cbeta;
  if(strcmp(transa,"t") == 0 || strcmp(transa,"c")==0) { k=a.rows;}
  dgemm_(transa, transb, &m,&n,&k,alpha,ap,&lda,bp,&ldb,beta,cp,&m);
} 
#endif
template  void matrix<double>::scalapack_pdgemm(const matrix<double>& a, const matrix<double>& b, matrix<double>& c, 
    double calpha, double cbeta, char * transa, char* transb);

#ifndef MPI_DISABLED
bool equilibriate(matrix<double>& mat, double* s)
{   
  int n = mat.rows;
  double scond, amax;
  int info;
  int one = 1;
  bool isEquilibriate;
  scond = 0.0;
  
  pdpoequ_(&n,mat.pMat,&one,&one,mat.desc,s,(s+mat.lrows),&scond, &amax, &info);
  //printf("%0.15e %0.15e %0.15e\n",rowcnd,colcnd,amax);
  if( !(info==0) ) { 
    isEquilibriate = false;
    fprintf(stderr, "Matrix Equilibriation failed\n");
    for(int i=0; i<mat.lrows+mat.lcols; i++) s[i] = 1.0; //in case I force equilibriation do nothing
  }   
  else {
    isEquilibriate = ((scond < 0.1));
    printf("amax %0.15e %0.15e\n",amax,scond);
  }   
  return isEquilibriate;
}
#else
bool equilibriate(matrix<double>& mat, double* s)
{   
  int n = mat.rows;
  double scond, amax;
  int info;
  bool isEquilibriate;
  scond = 0.0;
  dpoequ_(&n,mat.pMat,&n,s,&scond,&amax,&info);
  //printf("%0.15e %0.15e %0.15e\n",rowcnd,colcnd,amax);
  if( !(info==0) ) { 
    isEquilibriate = false;
    fprintf(stderr, "Matrix Equilibriation failed\n");
    for(int i=0; i<n; i++) s[i] = 1.0; //in case I force equilibriation do nothing
  }   
  else {
    isEquilibriate = ((scond < 0.1));
    //printf("amax %0.15e %0.15e\n",amax,scond);
  }   
  return isEquilibriate;
}
#endif

void apply_equilibriate(matrix<double>& mat, double* s, bool isInverse)
{
  if(isInverse)
  {
    for(int i=0; i<mat.lrows; i++)
    for(int j=0; j<mat.lcols; j++) 
      mat(i,j) = (s[i+mat.lrows]*s[j])*mat(i,j);
  }
  else
  {
    for(int i=0; i<mat.lrows; i++)
    for(int j=0; j<mat.lcols; j++) 
      mat(i,j) = s[i] * mat(i,j) * s[j+mat.lrows];
  }
}

template<typename T>
void matrix<T>::inverse_choleski(T kernel_nugget)
{
  char uplo[2] = "L";
  int info;
#ifndef MPI_DISABLED
  int one = 1;
#endif
  matrix<T> choleski(rows,cols,ly);
  int n = choleski.rows;

//turned of condition checking
#if 0
  /* computing conditioning number */
  int lwork, liwork; 
  /* query optimal sizes */
#ifndef MPI_DISABLED
  int qiwork;
  lwork = liwork = -1;
  double qwork;
  //pdpocon_(uplo, &cols, pMat, &one, &one, desc, &anorm, &kappa, &qwork, &lwork, &qiwork, &liwork, &info);
  lwork = (int)qwork;
  liwork = (int)qiwork;
#else
  lwork = 3*rows;
  liwork = rows;
#endif  
  
  double work[lwork];
  int iwork[liwork];

#ifndef MPI_DISABLED
  //pdpocon_(uplo, &cols, pMat, &one, &one, desc, &anorm, &kappa, work, &lwork, iwork, &liwork, &info);
#else
  dpocon_(uplo, &rows, pMat, &rows, &anorm, &kappa, work, iwork, &info);
#endif
#endif
 
  if(std::isnan(kappa) || std::isinf(kappa)) 
  {
    graceful_exit(1,"Error kappa is NaN!");
  }
  
  //throw a warning if poorly conditioned
  if(log10(kappa) > 10)
  {
    if(ly.rank == 0)
      fprintf(stderr, "\x1b[31mWarning the choleski decomposed matrix is poorly conditioned log k(A) = %0.15e\n"
                      "There may be significant numerical errors if computing the inverse.\n\x1b[0m", kappa);
  }

  choleski = *this;
  this->Id();
#ifndef MPI_DISABLED
  pdpotrs_(uplo, &n, &n, choleski.pMat, &one, &one, choleski.desc, this->pMat, &one, &one, this->desc, &info);
#else
  dpotrs_(uplo, &n, &n, choleski.pMat, &n,this->pMat, &this->rows, &info);
#endif
}
template void matrix<double>::inverse_choleski(double kernel_nugget);


template<typename T>
#ifndef MPI_DISABLED
double matrix<T>::scalapack_choleski_lower(matrix<T>& a, bool det)
{
  char uplo[2] = "l";
  int n = a.rows;
  T* ap = a.pMat;
  int one = 1;
  int info;
 
  /* computing the infinity norm to get the conditioning number */
  char norm[2] = "I";
  int iarow = ly.globalToProcessRow(0);
  int size = ly.numRowsOnProcess(rows,iarow);
  double awork[size]; 
  anorm = pdlange_(norm, &rows, &cols, pMat, &one, &one, desc, awork);

  /* choleski */
  pdpotrf_(uplo, &n, ap, &one, &one, a.desc, &info);
  if(info != 0) 
  {
    a.write("failed.ck");
    graceful_exit(1,"choleski_lower() failed");
  }
  
  /* zero upper triangle */
  for(int j=0; j<a.lcols; j++)
  for(int i=0; i<a.lrows; i++)
  {
    int gi = a.ly.localToGlobalRow(i);
    int gj = a.ly.localToGlobalCol(j);
    if(gj > gi) a(i,j) = double(1);
  }

  /* compute determinant if turned on */  
  T determinant = double(1);
  for(int j=0; j<a.lcols; j++)
  for(int i=0; i<a.lrows; i++)
  {
    int gi = a.ly.localToGlobalRow(i);
    int gj = a.ly.localToGlobalCol(j);
    if(gj>gi) (*this)(i,j) = double(0);
    if(det) 
      if(gj == gi) determinant *= a(i,j);
  }
  if(det) global_prod(determinant);

  return determinant*determinant;
}
#else
double matrix<T>::scalapack_choleski_lower(matrix<T>& a, bool det)
{
  char uplo[2] = "l";
  int n = a.rows;
  T* ap = a.pMat;
  int info;

  /* computing the infinity norm to get the conditioning number */
  char norm[2] = "I";
  int iarow = ly.globalToProcessRow(0);
  int size = ly.numRowsOnProcess(rows,iarow);
  double awork[size]; 
  anorm = dlange_(norm, &rows, &cols, pMat, &rows, awork);
  
  /* choleski decomposition */
  dpotrf_(uplo, &n, ap, &n, &info);
  
  /* computing conditioning number */
  double kappa;
  double work[3*rows];
  int iwork[rows];
  dpocon_(uplo, &rows, pMat, &rows, &anorm, &kappa, work, iwork, &info);
  kappa = double(1)/kappa;
  
  //throw a warning if poorly conditioned
  if(log10(kappa) > 10)
  {
    if(ly.rank == 0)
      fprintf(stderr, "\x1b[31mWarning the choleski decomposed matrix is poorly conditioned log k(A) = %0.15e\n"
                      "There may be significant numerical errors if computing the inverse.\n\x1b[0m", kappa);
    exit(1);
  }

  /* compute determinant if turned on */  
  T determinant = double(1);
  for(int j=0; j<a.lcols; j++)
  for(int i=0; i<a.lrows; i++)
  {
    int gi = a.ly.localToGlobalRow(i);
    int gj = a.ly.localToGlobalCol(j);
    if(gj>gi) (*this)(i,j) = double(0);
    if(det) 
      if(gj == gi) determinant *= a(i,j);
  }
  if(det) global_prod(determinant);

  return determinant*determinant;
}
#endif
template double matrix<double>::scalapack_choleski_lower(matrix<double>& a, bool det);

template<typename T>
double matrix<T>::choleski_lower(bool det)
{
     return scalapack_choleski_lower(*this, det);
}
template double matrix<double>::choleski_lower(bool det);
   
template<typename T>
void matrix<T>::transposeAxB(const matrix<T>& a, const matrix<T>& b, matrix<T>& result)
{ 
  T alpha = double(1);
  T beta = double(0);
  char t[2] = "t";
  char n[2] = "n";
  scalapack_pdgemm(a,b,result, alpha, beta, t, n);
}
template void matrix<double>::transposeAxB(const matrix<double>& a, const matrix<double>& b, matrix<double>& result);

template<typename T>
void matrix<T>::AxtransposeB(const matrix<T>& a, const matrix<T>& b, matrix<T>& result)
{ 
  T alpha = double(1);
  T beta = double(0);
  char t[2] = "t";
  char n[2] = "n";
  scalapack_pdgemm(a,b,result, alpha, beta, n, t);
}
template void matrix<double>::AxtransposeB(const matrix<double>& a, const matrix<double>& b, matrix<double>& result);

template<typename T>
T matrix<T>::scalar_product(matrix<T>& v1)
{
   T sum = double(0);
   for(int j=0; j<v1.lcols*v1.lrows; j++)
     sum += v1.pMat[j]*pMat[j];
   global_sum(sum);
   return sum;
}
template double matrix<double>::scalar_product(matrix<double>& v1);

template<typename T>
#ifndef MPI_DISABLED
void matrix<T>::transpose(matrix<T>& result)
{
  T* pd = (double*)pMat;
  T* pres = (double*)result.pMat;
  int one = 1;
  T cone = double(1);
  T czero = double(0);
  char t[2] = "t";
  pdgeadd_(t, &result.rows, &result.cols, &cone, pd, &one, &one, this->desc, &czero, pres, &one,&one, result.desc);
}
#else
void matrix<T>::transpose(matrix<T>& result)
{
  assert(rows == cols);
  for(int j=0; j<cols; j++)
  for(int i=0; i<rows; i++)
    result.pMat[i+lrows*j] = pMat[j+lrows*i];
}
#endif
template void matrix<double>::transpose(matrix<double>& result);
    
template<typename T>
void matrix<T>::write(const char *filename, const char* type)
{
  T* g = new T[rows*cols];
  memset(g,0,sizeof(T)*rows*cols);
  copyLocalToGlobal(g);
  if(ly.rank==0)
  {
    FILE* fp = fopen(filename,type);
    fwrite(g, sizeof(T), rows*cols, fp);
    fclose(fp);
  }
  delete [] g;
}
template void matrix<double>::write(const char *filename, const char* type);

template<typename T>
void matrix<T>::write(FILE* fp)
{
  T* g = new T[rows*cols];
  memset(g,0,sizeof(T)*rows*cols);
  copyLocalToGlobal(g);
  if(ly.rank==0)
    fwrite(g, sizeof(T), rows*cols, fp);
  delete [] g;
}
template void matrix<double>::write(FILE* fp);

template<typename T>
void matrix<T>::read(FILE* fp)
{
  int gi, gj;
  T* g = new T[rows*cols];
  if(ly.rank == 0)
    fread(g, sizeof(T), rows*cols, fp);
  broadcast(g, rows*cols);
  for(int j=0; j<lcols; j++)
  for(int i=0; i<lrows; i++)
  {
      gi = ly.localToGlobalRow(i);
      gj = ly.localToGlobalCol(j);
       pMat[i+lrows*j] = g[gj+lcols*gi];
  };
  delete [] g;
}
template void matrix<double>::read(FILE* fp);
 
template<typename T>
T matrix<T>::log_determinant()
{
  T log_determinant = T(0);
  T z;
  int gi, gj;
  for(int j=0; j<lcols; j++)
  for(int i=0; i<lrows; i++)
  {
    gi = ly.localToGlobalRow(i);
    gj = ly.localToGlobalCol(j);
    z = (*this)(i,j);
    if( gi == gj &&  z > 0) log_determinant += log(std::abs((*this)(i,j)));
  }
  global_sum(log_determinant);
  log_determinant *= double(2);
  return log_determinant;
}
template double matrix<double>::log_determinant();

template<typename T>
T matrix<T>::trace()
{

  int gi, gj;
  T trace = T(0);
  for(int j=0; j<lcols; j++)
  for(int i=0; i<lrows; i++)
  {
    gi = ly.localToGlobalRow(i);
    gj = ly.localToGlobalCol(j);
    if( gi == gj ) trace += ((*this)(i,j));
  }
  global_sum(trace);
  return trace;
}
template double matrix<double>::trace();

template<typename T>
void matrix<T>::diagonal(T *diag)
{
  int gi, gj;
  int len = (rows < cols) ? rows : cols;
  for(int i=0; i<len; i++) diag[i] = double(0);

  for(int j=0; j<lcols; j++)
  {
    gj = ly.localToGlobalCol(j);
    for(int i=0; i<lrows; i++)
    {
      gi = ly.localToGlobalRow(i);
      if( gi == gj ) diag[gi] = (*this)(i,j);
    }
  }
  global_sum(diag,len);
}
template void matrix<double>::diagonal(double * diag);

#ifndef MPI_DISABLED
void solve_choleski_system(matrix<double>& choleski, matrix<double>& src, matrix<double>& result, int nrhs, double kernel_nugget)
{
  char uplo[2] = "L";
  int n = choleski.rows;
  int one = 1;
  int info;
  result = src;
  pdpotrs_(uplo, &n, &nrhs, choleski.pMat, &one, &one, choleski.desc, result.pMat, &one, &one, result.desc, &info);

}
#else
void solve_choleski_system(matrix<double>& choleski, matrix<double>& src, matrix<double>& result, int nrhs, double kernel_nugget)
{
  char uplo[2] = "L";
  int n = choleski.rows;
  int one = 1;
  int info;
  result = src;
  dpotrs_(uplo, &n, &one, choleski.pMat, &n,result.pMat, &result.rows, &info);
}
#endif

void diag_multiply(double* diag, matrix<double>& m, matrix<double>& result, bool isInverse)
{
  double tmp[m.lrows];
  for(int i=0; i<m.lrows; i++) tmp[i] = diag[m.ly.localToGlobalRow(i)];

  if(isInverse)
  {
    for(int j=0; j<m.lcols; j++)
    for(int i=0; i<m.lrows; i++)
       result(i,j) = double(1)/(tmp[i])*m(i,j);
  }
  else
  {
    for(int j=0; j<m.lcols; j++)
    for(int i=0; i<m.lrows; i++)
      result(i,j) = tmp[i]*m(i,j);
  }
}

void diag_multiply(matrix<double>& diag, matrix<double>& m, matrix<double>& result, bool isInverse)
{
  if(isInverse)
  {
    for(int j=0; j<m.lcols; j++)
    for(int i=0; i<m.lrows; i++)
       result(i,j) = (double(1)/diag(i,0))*m(i,j);
  }
  else
  {
    for(int j=0; j<m.lcols; j++)
    for(int i=0; i<m.lrows; i++)
      result(i,j) = diag(i,0)*m(i,j);
  }
}
void dotKNM_multiply_spgp_vec(matrix<double>& dotKNM, double alpha, matrix<double>& B, matrix<double>& result, int col)
{
  result.clear();
  for(int i=0; i<dotKNM.lrows; i++)
    result(i,0) += alpha*dotKNM(i,col)*B(col,0);
}

void dotKMN_multiply_spgp_vec(matrix<double>& dotKNMcol, double alpha, matrix<double>& B, matrix<double>& result, int col)
{
  int gi;
  double z = double(0);
  result.clear();
  for(int i=0; i<dotKNMcol.lrows; i++)
      z += alpha*dotKNMcol(i,col)*B(i,0);
  global_sum(z);
  for(int i=0; i<result.lrows; i++)
  {
    gi = result.ly.localToGlobalRow(i);
    if(gi == col) 
      result(i,0) = z;
    else
      result(i,0) = double(0);
  }
}

double scalar_product(matrix<double>& A, matrix<double>& B, matrix<double>& C)
{
  double res = double(0);
  for(int j=0; j<A.lcols; j++)
  for(int i=0; i<A.lrows; i++)
    res += A(i,0)*B(i,0)*C(i,0);
  global_sum(res);
  return res;
}

double dotKMN_multiply_spgp_vec_trace(matrix<double>& dotKNMcol, double alpha, matrix<double>& B,  int col)
{
  double z = double(0);
  for(int i=0; i<B.lrows; i++)
    z += alpha*dotKNMcol(i,col)*B(i,col);
  global_sum(z);
  return z;
}

void diag_spgp_dotKNM(matrix<double>& dotKNM, matrix<double>& B, int col, double* diag, double alpha)
{
  int gi;
  for(int i=0; i<dotKNM.lrows; i++)
  {
    gi = dotKNM.ly.localToGlobalRow(i);
    diag[gi] += alpha*dotKNM(i,col)*B(i,col);
  }
}

void diag_spgp_dotKNM(matrix<double>& dotKNM, matrix<double>& B, int col, matrix<double>& res, double alpha)
{
    for(int i=0; i<dotKNM.lrows; i++)
      res(i,0) += alpha*dotKNM(i,col)*B(i,col);
}

void diag_spgp4(matrix<double>& A, matrix<double>& B,  matrix<double>& res)
{
  res.clear();
  for(int j=0; j<A.lcols; j++)
  for(int i=0; i<A.lrows; i++)
      res(i,0) += A(i,j)*B(i,j);
}

void diag_spgp2(matrix<double>& A, matrix<double>& B, matrix<double>& diag)
{
  diag.clear(); 
  for(int i=0; i<A.lrows; i++)
  for(int j=0; j<A.lcols; j++)
  {
    diag(i,0) += A(i,j)*B(i,j);
  }
}

double diag_spgp3(matrix<double>& A, matrix<double>& B, matrix<double>& C, int col)
{
  double tr = double(0); 
  for(int i=0; i<A.lrows; i++)
    tr += C(i,0)*A(i,col)*B(i,col);
  global_sum(tr);
  return tr;
}

double trace(matrix<double>& A, matrix<double>& B, matrix<double>& C)
{
  double tr = double(0); 
  for(int j=0; j<A.lcols; j++)
  for(int i=0; i<A.lrows; i++)
    tr += C(i,0)*A(i,j)*B(i,j);
//  global_sum(tr);
  return tr;
}

double trace_invC(matrix<double>& A, matrix<double>& B, matrix<double>& C)
{
  double tr = double(0); 
  for(int j=0; j<A.lcols; j++)
  for(int i=0; i<A.lrows; i++)
    tr += A(i,j)*B(i,j)/C(i,0);
//  global_sum(tr);
  return tr;
}

double trace(matrix<double>& A, matrix<double>& B, matrix<double>& C, int col)
{
  double tr = double(0); 
  for(int i=0; i<A.lrows; i++)
    tr += C(i,0)*A(i,col)*B(i,col);
//  global_sum(tr);
  return tr;
}

double trace_invC(matrix<double>& A, matrix<double>& B, matrix<double>& C, int col)
{
  double tr = double(0); 
  for(int i=0; i<A.lrows; i++)
    tr += A(i,col)*B(i,col)/C(i,0);
//  global_sum(tr);
  return tr;
}

double trace(matrix<double>& A, matrix<double>& B)
{
  double res = double(0);
  for(int j=0; j<A.lcols; j++)
  for(int i=0; i<A.lrows; i++)
      res += A(i,j)*B(i,j);
 // global_sum(res);
  return res;
}

double trace(matrix<double>& A, matrix<double>& B, int col)
{
  double res = double(0);
  for(int i=0; i<A.lrows; i++)
      res += A(i,col)*B(i,col);
//  global_sum(res);
  return res;
}

//this only works if matrix is square  
double trace_vmv(matrix<double>& A, matrix<double>& B, matrix<double>& C)
{
  double res = double(0);
  for(int j=0; j<B.lcols; j++)
  for(int i=0; i<B.lrows; i++)
      res += A(i,0)*B(i,j)*C(j,0);
  //global_sum(res);
  return res;
}

double trace_vmv(matrix<double>& A, matrix<double>& B, double* C)
{
  double res = double(0);
  for(int j=0; j<B.lcols; j++)
  for(int i=0; i<B.lrows; i++)
      res += A(i,0)*B(i,j)*C[j];
  //global_sum(res);
  return res;
}

//this only works if matrix is square  
double trace_vmv(matrix<double>& A, matrix<double>& B, matrix<double>& C, int col)
{
  double res = double(0);
  for(int i=0; i<B.lrows; i++)
      res += A(i,0)*B(i,col)*C(col,0);
//  global_sum(res);
  return res;
}

double trace_vmv(matrix<double>& A, matrix<double>& B, double* C, int col)
{
  double res = double(0);
  for(int i=0; i<B.lrows; i++)
      res += A(i,0)*B(i,col)*C[col];
//  global_sum(res);
  return res;
}


double trace_vvv(matrix<double>& A, matrix<double>& B, matrix<double>& C)
{
  double res = double(0);
  for(int i=0; i<A.lrows; i++)
      res += A(i,0)*B(i,0)*C(i,0);
 // global_sum(res);
  return res;
}

double trace_vv(matrix<double>& A, matrix<double>& B)
{
  double res = double(0);
  for(int i=0; i<A.lrows; i++)
      res += A(i,0)*B(i,0);
 // global_sum(res);
  return res;
}
