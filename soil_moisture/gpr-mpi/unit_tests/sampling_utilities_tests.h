#ifndef SAMPLINT_UTILITIES_TESTS_H 
#define SAMPLINT_UTILITIES_TESTS_H
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>
#include <math.h>
#include <sampling_utilities.h>

class TestSamplingUtilities : public CPPUNIT_NS::TestFixture
{ 
  double epsilon = 1e-15;
  double *inputs;
  double *targets;
  
  CPPUNIT_TEST_SUITE(TestSamplingUtilities);
  CPPUNIT_TEST(testSpaceFillingSample);
  CPPUNIT_TEST_SUITE_END();

  public:
  void setUp() { 
    inputs  = new double[34*5000]; 
    targets = new double[5000]; 
    FILE* fp = NULL;
   
    fp = fopen("./unit_tests/sampling_test_inputs.bin","r");
    if(fp == NULL) {
      fprintf(stderr,"\nCould not read sampling_test_inputs.bin.\n");
      exit(1);
    }
    fread(inputs,sizeof(double),34*5000,fp);
   
    fp = NULL;
    fp = fopen("./unit_tests/sampling_test_targets.bin","r");
    if(fp == NULL) {
      fprintf(stderr,"\nCould not read sampling_test_targets.bin.\n");
      exit(1);
    }
    fread(targets,sizeof(double),5000,fp);
  }

  void tearDown() { 
    delete [] targets; 
    delete [] inputs; 
  }

  /* definition of tests */
  void testSpaceFillingSample() {
    int nSamples = 1000;;
    int dimX     = 34;
    double x[dimX*nSamples];
    double y[5000];
    double tmp[5000];
    for(int i=0; i<5000; i++) tmp[i] = targets[i];
    spaceFillingSample(inputs,targets,x,y+0*1000,dimX,5000,nSamples,-1,true,false);
    spaceFillingSample(inputs,targets,x,y+1*1000,dimX,4000,nSamples,-1,true,false);
    spaceFillingSample(inputs,targets,x,y+2*1000,dimX,3000,nSamples,-1,true,false);
    spaceFillingSample(inputs,targets,x,y+3*1000,dimX,2000,nSamples,-1,true,false);
    spaceFillingSample(inputs,targets,x,y+4*1000,dimX,1000,nSamples,-1,true,false);
    std::sort(tmp, tmp+5000);
    std::sort(y  , y  +5000);
    for(int i=0; i<5000; i++) CPPUNIT_ASSERT_EQUAL(tmp[i],y[i]);
  }
}; //end test

/* add this suite to the registry */
CPPUNIT_TEST_SUITE_REGISTRATION(TestSamplingUtilities);
#endif
