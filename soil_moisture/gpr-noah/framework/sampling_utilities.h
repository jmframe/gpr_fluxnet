#ifndef H_SAMPLING_UTILITIES
#define H_SAMPLING_UTILITIES
#include <limits>
#include <iostream>
#include <iomanip>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <algorithm>

template<typename T>
void spaceFillingSample(T* inputs, T* targets, T* newInputs, T* newTargets, int dimX, int nSamples, int nNewSamples, int nBins=-1, bool isOverwrite=true, bool isVerbose=true)
{
  /* set bins by sturges */
  if(nNewSamples < 1) return;
  int nbins;
  (nBins==-1) ? nbins = 1 + 3.332*log10(nNewSamples) : nbins = nBins;
  T deltas[dimX];
  T min[dimX],max[dimX];
  std::vector<int> counts(nbins*dimX);
  int idx[dimX];
  
  std::vector<int> scores(nSamples); 
  std::vector<std::vector<double>> in(nSamples,std::vector<double>(dimX));
  std::vector<double> out(nSamples);
  for(int i=0; i<nSamples; i++)
  {
    out[i] = targets[i];
    for(int j=0; j<dimX; j++)
      in[i][j] = inputs[j+dimX*i];
  }

  /* dx for each parameter */ 
  for(int j=0; j<dimX; j++)
  {
    min[j]= std::numeric_limits<T>::max();
    max[j]= std::numeric_limits<T>::min();
    for(int i=0; i<nSamples; i++)
    {
       if(inputs[j+dimX*i] < min[j]) min[j] = inputs[j+dimX*i];
       if(inputs[j+dimX*i] > max[j]) max[j] = inputs[j+dimX*i];
    }
    deltas[j] = (max[j] - min[j]) / nbins;
  }

  std::fill(counts.begin(),counts.end(),0);
  std::vector<int>::iterator it;
  for(int n = 0; n <nNewSamples; n++) 
  {
    /* compte scores */
    int cnt = -1;
    for(auto &x : in)
    {
      cnt++;
      scores[cnt] = 0;
      for(int k=0; k<dimX; k++) 
      {
        (deltas[k] < std::numeric_limits<T>::min()) ? idx[k] = 0 : idx[k] = std::floor((x[k]-min[k])/deltas[k]);
        if(idx[k] == nbins) idx[k] = nbins - 1;
        if(idx[k] < 0) idx[k] = 0;
        scores[cnt] += counts[idx[k]+k*nbins];
      }
    }
   
    /* find min score and add element to the set */
    it = std::min_element(scores.begin(), scores.begin()+cnt);
    int pos = std::distance(scores.begin(), it);
    
    for(int k=0; k<dimX; k++) newInputs[k+n*dimX] = in[pos][k];
    newTargets[n] = out[pos];

    /* remove from set and update counter */
    for(int k=0; k<dimX; k++) 
   {
      (deltas[k] < std::numeric_limits<T>::min()) ? idx[k] = 0 : idx[k] = std::floor((in[pos][k]-min[k])/deltas[k]);
      if(idx[k] == nbins) idx[k] = nbins - 1;
      if(idx[k] < 0) idx[k] = 0;
      counts[idx[k]+k*nbins] += 1;
    }
    in.erase(in.begin()+pos);
    out.erase(out.begin()+pos);
  }

  if(isOverwrite)
  {
    for(int i=0; i<nSamples-nNewSamples; i++)
    {
      targets[i] = out[i];
      for(int j=0; j<dimX; j++)
        inputs[j+dimX*i] = in[i][j];
    }
  }

  if(isVerbose)
  { 
    if(get_rank()==0)
    {
      printf("\nParameter bin counts (nbins = %i).\n",nbins);
      for(int i = 0; i < (log10(nNewSamples)+4)*nbins+7; i++) std::cout << "-";
      std::cout << "\n"; 
      for(int i = 0; i<dimX; i++)
      {
        std::cout << "x[" << std::setw(4) << i << "]";
        for(int j = 0; j<nbins; j++)
          std::cout << std::setw(log10(nNewSamples)+4) << counts[j+nbins*i];
        printf("\n");
      }
    }
  }
}
#endif
