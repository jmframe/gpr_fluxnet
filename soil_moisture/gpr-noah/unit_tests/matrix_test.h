#ifndef MATRIX_TESTS_H 
#define MATRIX_TESTS_H
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>
#include <math.h>
#include "layout.h"
#include "scalapack_matrix.h"
class TestMatrix : public CPPUNIT_NS::TestFixture
{ 
  double epsilon = 1e-15;
  layout* ly;
  
  CPPUNIT_TEST_SUITE(TestMatrix);
  CPPUNIT_TEST(testEqual);
  CPPUNIT_TEST(testIdentity);
  CPPUNIT_TEST(testTranspose);
  CPPUNIT_TEST(testMatrixMatrixMultiply);
  CPPUNIT_TEST(testTransposeAXB);
  CPPUNIT_TEST(testClear);
  CPPUNIT_TEST(testCholeskiLowerDecomposition);
  CPPUNIT_TEST(testInverseCholeski);
  CPPUNIT_TEST(testSolveCholeskiSystem);
  CPPUNIT_TEST_SUITE_END();

  public:
  void setUp() { 
     ly = new layout(32,32,2); 
  }

  void tearDown() { 
     delete ly; 
  }

  /* definition of tests */
  void testEqual() {
    matrix<double> m(10,10,*ly);
    matrix<double> n(10,10,*ly);
    double x = -1;
    m.uniformRandom();
    n = m;
    for(int i=0; i<m.rows; i++)
    for(int j=0; j<m.cols; j++)
    {
      x = m(i,j)-n(i,j);
      CPPUNIT_ASSERT_DOUBLES_EQUAL(0.,x,epsilon);
    }
  }

  void testIdentity() {
    matrix<double> m(10,10,*ly);
    double x;
    m.Id();
    for(int i=0; i<m.rows; i++)
    for(int j=0; j<m.cols; j++)
    {
      (i==j) ? x = m(i,j)-1.0 : x = m(i,j);
      CPPUNIT_ASSERT_DOUBLES_EQUAL(0.,x,epsilon);
    }
  }
  
  void testTranspose() {
    int nx = 10;
    int ny = 10;
    double x = -1;
    matrix<double> m(nx,ny,*ly);
    matrix<double> n(ny,nx,*ly);
    m.uniformRandom();
    m.transpose(n);
    for(int i=0; i<m.rows; i++)
    for(int j=0; j<m.cols; j++)
    {
      x = m(i,j) - n(j,i);
      CPPUNIT_ASSERT_DOUBLES_EQUAL(0,x,epsilon);
    }
  }
  
  void testMatrixMatrixMultiply() {
    int n = 10;
    matrix<double> l(n,n,*ly);
    matrix<double> m(n,n,*ly);
    matrix<double> p(n,n,*ly);
    double x = -1;
    for(int i=0; i<n; i++)
    for(int j=0; j<n; j++)
    {
      l(i,j) = i+j;
      m(i,j) = i-j;
    }
    p = l*m;
    for(int i=0; i<n; i++)
    for(int j=0; j<n; j++)
    {
      x = p(i,j);
      p(i,j) -= 1.0/6.0*n*(-(1.0 + 3.0*j - 2.0*n)*(-1.0 + n) + i*(-3.0 - 6.0*j + 3*n));
      CPPUNIT_ASSERT_DOUBLES_EQUAL(0.,p(i,j),fabs(x*epsilon));
    }
  }
  
  void testTransposeAXB() {
    int n = 10;
    matrix<double> l(n,n,*ly);
    matrix<double> m(n,n,*ly);
    matrix<double> p(n,n,*ly);
    double x = -1;
    for(int i=0; i<n; i++)
    for(int j=0; j<n; j++)
    {
      l(i,j) = 2.0*i+j;
      m(i,j) = i-j;
    }
    p.transposeAxB(l,m,p);;
    for(int i=0; i<n; i++)
    for(int j=0; j<n; j++)
    {
      x = p(i,j);
      p(i,j) -= 1.0/6.0*n*(-2.0*(1.0 + 3.0*j - 2.0*n)*(-1.0 + n) + i*(-3.0 - 6.0*j + 3.0*n));
      CPPUNIT_ASSERT_DOUBLES_EQUAL(0.,p(i,j),fabs(x*epsilon));
    }
  }
  
  void testClear() {
    int nx = 20;
    matrix<double> m(nx,nx,*ly);
    m.uniformRandom();
    m.clear();
    for(int i=0; i<nx; i++)
    for(int j=0; j<nx; j++)
      CPPUNIT_ASSERT_EQUAL(0.,m(i,j));

  }
  
  void testCholeskiLowerDecomposition() {
    int nx = 20;
    matrix<double> l(nx,nx,*ly);
    matrix<double> m(nx,nx,*ly);
    matrix<double> n(nx,nx,*ly);
    l.uniformRandom();
    l.transpose(m);
    n = l*m;
    l = n; //create symmetric matrix
    l.choleski_lower();
    l.transpose(m);
    l = l*m;
    m = l - n;
    for(int i=0; i<nx; i++)
    for(int j=0; j<nx; j++)
      CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, m(i,j),epsilon*10);
  }
  
  void testInverseCholeski() {
    int nx = 5;
    double x = -1;
    matrix<double> l(nx,nx,*ly);
    matrix<double> m(nx,nx,*ly);
    matrix<double> n(nx,nx,*ly);
    l.uniformRandom();
    l.transpose(m);
    n = l*m;
    l = n; //create symmetric matrix
    m = l;
    l.choleski_lower();
    l.inverse_choleski();
    n = l*m;
    for(int i=0; i<nx; i++)
    for(int j=0; j<nx; j++)
    {
      (i==j) ? x = n(i,j)-1.0 : x = n(i,j);
      CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0,x,1e-8);
    }
  }

  void testSolveCholeskiSystem() {
    int nx = 20;
    matrix<double> l(nx,nx,*ly);
    matrix<double> m(nx,nx,*ly);
    matrix<double> n(nx,nx,*ly);
    matrix<double> u(nx,1,*ly);
    matrix<double> v(nx,1,*ly);
    l.uniformRandom();
    u.uniformRandom();
    l.transpose(m);
    n = l*m;
    l = n; //create symmetric matrix
    l.choleski_lower();
    solve_choleski_system(l,u,v);
    l.inverse_choleski();
    v = n*u-v;
  }
  
}; //end test

/* add this suite to the registry */
CPPUNIT_TEST_SUITE_REGISTRATION(TestMatrix);
#endif
