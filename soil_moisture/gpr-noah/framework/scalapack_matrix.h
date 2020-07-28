#ifndef MATRIX_H
#define MATRIX_H
#include <string>
#include <string.h>
#include <iostream>
#include <algorithm>
#include "layout.h"
#define CHOP(a) (fabs(a)<1e-9) ? 0.0 : a


#define MATRIXERROR \
if(ly.rank==0){\
std::cerr << (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)\
<< ":" << __LINE__ <<" ERROR: matrices incompatible" << std::endl;\
exit(1);\
}

template<typename T>
class matrix
{
  public:
    int rows, cols;
    T* pMat;
    layout& ly;
    int lrows, lcols;
    int *desc;
    double anorm = 0;
    double kappa = 0;
    matrix(int rows, int cols, layout& ly) : rows(rows), cols(cols), ly(ly)
  {
    lrows = ly.numrocRow(rows);
    lcols = ly.numrocCol(cols);
    desc = new int[9];
    ly.descinit(desc, rows, cols);
    pMat = new T[lrows*lcols];
  }
    void Id();
    void diagonal(T *diag);
    void copy(matrix<T>& a, matrix<T>& b);
    void printLocal();
    void print();
    void clear();
    void uniformRandom();
    void copyLocalToGlobal(T* g);
    void scalapack_pdgemm(const matrix<T>& a, const matrix<T>& b, matrix<T>& c, T calpha, T cbeta, char * transa, char* transb);
    double scalapack_choleski_lower(matrix<T>& a, bool det = false);
    double choleski_lower(bool det = false);
    void inverse_choleski(T kernel_nugget = 0);
    void transposeAxB(const matrix<T>& a, const matrix<T>& b, matrix<T>& result);
    void AxtransposeB(const matrix<T>& a, const matrix<T>& b, matrix<T>& result);
    void transpose(matrix<T>& result);
    T log_determinant();
    T trace();
    T scalar_product(matrix<T>& v1);
    void write(const char *filename, const char* type="w");
    void write(FILE* fp);
    void read(FILE* fp);

    T& operator()(int r, int c)
    {
      return this->pMat[r+lrows*c];
    }

    matrix<T>& operator=(const matrix<T>& rhs)
    {
      if(rows != rhs.rows || cols != rhs.cols) { MATRIXERROR }
      anorm = rhs.anorm;
      kappa = rhs.kappa;
      for(int i=0; i<lrows*lcols; i++)
        this->pMat[i] = rhs.pMat[i];
      return *this;
    }

    matrix<T> operator*(const matrix<T>& rhs)
    {
      if(cols != rhs.rows) { MATRIXERROR }
      matrix<T> tmp(rows, rhs.cols, ly);
      T alpha = double(1);
      T beta  = double(0);
      char no[2] = "n";
      scalapack_pdgemm(*this, rhs, tmp, alpha,beta, no, no); 
      return tmp;
    }
    
    matrix<T>& operator*=(const matrix<T>& rhs)
    {
      if(rows != rhs.rows || cols != rhs.cols) { MATRIXERROR }
      T alpha = double(1);
      T beta  = double(0);
      matrix<T> tmp(rows, rhs.cols, ly);
      char n[2] = "n";
      scalapack_pdgemm(*this, rhs, tmp, alpha, beta, n, n); 
      *this = tmp;
      return *this;
    }

    matrix<T> operator+(const matrix<T>& rhs)
    {
      if(rows != rhs.rows || cols != rhs.cols) { MATRIXERROR }
      matrix<T> tmp(rows, cols, ly);
      for(int i=0; i<lrows*lcols; i++)
        tmp.pMat[i] = this->pMat[i] + rhs.pMat[i];
      return tmp;
    }
    
    matrix<T>& operator+=(const matrix<T>& rhs)
    {
      if(rows != rhs.rows || cols != rhs.cols) { MATRIXERROR }
      for(int i=0; i<lrows*lcols; i++)
        this->pMat[i] += rhs.pMat[i];
      return *this;
    }
    
    matrix<T> operator-(const matrix<T>& rhs)
    {
      if(rows != rhs.rows || cols != rhs.cols) { MATRIXERROR }
      matrix<T> tmp(rows, cols, ly);
      for(int i=0; i<lrows*lcols; i++)
        tmp.pMat[i] = this->pMat[i] - rhs.pMat[i];
      return tmp;
    }
    
    matrix<T>& operator-=(const matrix<T>& rhs)
    {
      if(rows != rhs.rows || cols != rhs.cols) { MATRIXERROR }
      for(int i=0; i<lrows*lcols; i++)
        this->pMat[i] -= rhs.pMat[i];
      return *this;
    }

    template<typename R> 
    matrix<T>& operator*=(const R& value) 
    {
      for(int i=0; i<lrows*lcols; ++i)
        this->pMat[i] *= value;
      return *this;
    }

    template<typename R> 
    matrix<T> operator*(const R& value) 
    {
      matrix<T> result(rows, cols, ly);
      for(int i=0; i<lrows*lcols; ++i)
        result.pMat[i] = this->pMat[i] * value;
      return result;
    }

    ~matrix()
    {
      delete [] pMat; 
      delete [] desc; 
    }

};

template<typename T, typename R>
matrix<T> operator*(R value, const matrix<T>& mat)
{
  matrix<T> result(mat.rows,mat.cols, mat.ly);
  for(int i=0; i<mat.lrows*mat.lcols; ++i)
    result.pMat[i] = mat.pMat[i] * value;
  return result;
}

bool   equilibriate(matrix<double>& mat, double *s);
void   apply_equilibriate(matrix<double>& mat, double *s, bool isInverse);
void   diag_multiply(double* diag, matrix<double>& m, matrix<double>& result, bool isInverse = false);
void   diag_multiply(matrix<double>& diag, matrix<double>& m, matrix<double>& result, bool isInverse = false);
void   diag_spgp(matrix<double>& A, matrix<double>& B, double* diag);
void   dotKNM_multiply_spgp_vec(matrix<double>& dotKNMcol, double alpha, matrix<double>& B, matrix<double>& result, int col);
void   dotKMN_multiply_spgp_vec(matrix<double>& dotKNMcol, double alpha, matrix<double>& B, matrix<double>& result, int col);
double dotKMN_multiply_spgp_vec_trace(matrix<double>& dotKNMcol, double alpha, matrix<double>& B,  int col);
void   diag_spgp2(matrix<double>& A, matrix<double>& B, matrix<double>& diag);
double diag_spgp3(matrix<double>& A, matrix<double>& B, matrix<double>& C, int col);
void   diag_spgp_dotKNM(matrix<double>& dotKNM, matrix<double>& B, int col, double* diag, double alpha);
void   diag_spgp_dotKNM(matrix<double>& dotKNM, matrix<double>& B, int col, matrix<double>& res, double alpha);
double trace(matrix<double>& A, matrix<double>& B);
double scalar_product(matrix<double>& A, matrix<double>& B, matrix<double>& C);
void   diag_spgp4(matrix<double>& A, matrix<double>& B,  matrix<double>& res);
double trace(matrix<double>& A, matrix<double>& B, matrix<double>& C);
double trace_invC(matrix<double>& A, matrix<double>& B, matrix<double>& C);
double trace_vmv(matrix<double>& A, matrix<double>& B, matrix<double>& C);
double trace_vmv(matrix<double>& A, matrix<double>& B, matrix<double>& C, int col);
double trace_vvv(matrix<double>& A, matrix<double>& B, matrix<double>& C);
double trace(matrix<double>& A, matrix<double>& B, int col);
double trace(matrix<double>& A, matrix<double>& B, matrix<double>& C, int col);
double trace_invC(matrix<double>& A, matrix<double>& B, matrix<double>& C, int col);
double trace_vv(matrix<double>& A, matrix<double>& B);
double trace_vmv(matrix<double>& A, matrix<double>& B, double* C, int col);
double trace_vmv(matrix<double>& A, matrix<double>& B, double* C);
/* linear solvers */
void solve_choleski_system(matrix<double>& choleski, matrix<double>& src, matrix<double>& result, int nrhs = 1, double kernel_nugget = 0);
#undef MATRIXERROR
#endif
