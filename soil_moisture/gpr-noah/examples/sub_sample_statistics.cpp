#include <stdio.h>
#include "comm.h"
#include "options.h"
#include "sampling_utilities.h"
using namespace std;

#define print0 if(get_rank()==0) printf
int main(int argc, char** argv)
{
  /* set up and read in options */
  char iInputs[256];
  char iTargets[256];
  char fInputs[256];
  char fTargets[256];
  int  iNum, fNum, dimX;
  
  start_parallel(argc,argv);
  if(get_comm_size() != 1) fprintf(stderr,"This utilitiy can only be run with one process");

  options opt;
  opt.add("initial_input_filename", iInputs);
  opt.add("initial_target_filename", iTargets);
  opt.add("final_input_filename", fInputs);
  opt.add("final_target_filename", fTargets);
  opt.add("initial_num_samples", iNum);
  opt.add("final_num_samples", fNum);
  opt.add("num_parameters", dimX);
  opt.read_options(argc, argv);
  
  double* inInputs, *inTargets, *fiInputs, *fiTargets;
  inInputs = new double[iNum*dimX];
  inTargets = new double[iNum];
  fiInputs = new double[fNum*dimX];
  fiTargets = new double[fNum];

  FILE* fp = NULL;
  fp = fopen(iInputs,"r");
  if(fp==NULL) 
  {
    fprintf(stderr,"Could not read %s\n",iInputs);
    exit(1);
  }
  fread(inInputs,sizeof(double),iNum*dimX,fp);
 
  fp = NULL; 
  fp = fopen(iTargets,"r");
  if(fp==NULL) 
  {
    fprintf(stderr,"Could not read %s\n",iTargets);
    exit(1);
  }
  fread(inTargets,sizeof(double),iNum,fp);

  spaceFillingSample(inInputs, inTargets, fiInputs, fiTargets, dimX, iNum, fNum);
  
  fp = fopen(fInputs,"w");
  if(fp==NULL) 
  {
    fprintf(stderr,"Could not read %s\n",fInputs);
    exit(1);
  }
  fwrite(fiInputs,sizeof(double),fNum*dimX,fp);
 
  fp = NULL; 
  fp = fopen(fTargets,"w");
  if(fp==NULL) 
  {
    fprintf(stderr,"Could not read %s\n",fTargets);
    exit(1);
  }
  fwrite(fiTargets,sizeof(double),fNum,fp);

  delete [] inInputs;
  delete [] inTargets;
  delete [] fiInputs;
  delete [] fiTargets;
  
  end_parallel();
  return 0;
}
