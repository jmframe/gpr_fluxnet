#include <stdio.h>
#include <iostream>
#include <string>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/TestListener.h>
#include <cppunit/BriefTestProgressListener.h>
#include <cppunit/CompilerOutputter.h>
#include <cppunit/TestResult.h>
#include <cppunit/TestResultCollector.h>
#include <cppunit/TestRunner.h>
#include "matrix_test.h"
#include "sampling_utilities_tests.h"

/* note test are run in the "reverse" order that the headers are incldued */
using namespace std;

int main(int argc, char** argv)
{
  start_parallel(argc,argv);
  /* Create the event manager and test controller */
  CPPUNIT_NS::TestResult controller;

  /* Add a listener that colllects test result */
  CPPUNIT_NS::TestResultCollector result;
  controller.addListener( &result );

  // Add a listener that prints dots as test run.
  CPPUNIT_NS::BriefTestProgressListener progress;
  controller.addListener( &progress );

  /* set up runner for registered test */
  CppUnit::TextTestRunner runner;
  CppUnit::TestFactoryRegistry &registry = CppUnit::TestFactoryRegistry::getRegistry();
  runner.addTest(registry.makeTest());

  cout << "Running IITM unit test" << endl;
  cout << "Note: within a test fixture later test can depend on routines being tested earlier." << endl;
  cout << "Fix the test in order, as that may fix routines tested later on." << endl << endl;
  runner.run(controller);
  
  cout << endl;
  (result.testFailures()==0) ? cout << "All test passed!" << endl : cerr << "Test failures detected!" << endl;
  for(int i=0; i<24; i++) cout << "-";
  cout << endl;
  cout << "There were " << result.runTests() << " tests run with " << result.testFailures() << " failures" << endl << endl;
  end_parallel();
  return (result.testFailuresTotal() == 0) ? 0 : 1;
}
