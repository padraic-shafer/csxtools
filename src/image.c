/*
 * Copyright (c) 2014, Brookhaven Science Associates, Brookhaven        
 * National Laboratory. All rights reserved.                            
 *                                                                      
 * Redistribution and use in source and binary forms, with or without   
 * modification, are permitted provided that the following conditions   
 * are met:                                                             
 *                                                                      
 * * Redistributions of source code must retain the above copyright     
 *   notice, this list of conditions and the following disclaimer.      
 *                                                                      
 * * Redistributions in binary form must reproduce the above copyright  
 *   notice this list of conditions and the following disclaimer in     
 *   the documentation and/or other materials provided with the         
 *   distribution.                                                      
 *                                                                      
 * * Neither the name of the Brookhaven Science Associates, Brookhaven  
 *   National Laboratory nor the names of its contributors may be used  
 *   to endorse or promote products derived from this software without  
 *   specific prior written permission.                                 
 *                                                                      
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS  
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT    
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS    
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE       
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,           
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES   
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR   
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)   
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,  
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OTHERWISE) ARISING   
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE   
 * POSSIBILITY OF SUCH DAMAGE.                                          
 *
 */

#include <omp.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <stdint.h>

#include "image.h"

void rotate90(data_t *in, data_t *out, int ndims, index_t *dims, int sense){
  index_t nimages = dims[0];
  index_t M = dims[ndims-1];
  index_t N = dims[ndims-2];
  index_t imsize = N*M;

  int x;
  for(x=1;x<(ndims-2);x++){
    nimages = nimages * dims[x];
  }   

  index_t *map = malloc(sizeof(index_t) * imsize);
  if(!map){
    return;
  }

#pragma omp parallel shared(in,out,map,N,M,imsize,nimages)
  {
    index_t i;
#pragma omp for private(i) 
    for(i=0;i<imsize;i++){
      index_t *mapp = map + i;
      index_t a, b;
      if(sense){
        a = M - 1 - (i / N);
        b = i % N;
      } else {
        a = i / N;
        b = N - 1 - (i % N);
      }
      *mapp = M*b + a;
    }

    index_t n;
#pragma omp for private(n) schedule(dynamic,imsize)
    for(n=0;n<(nimages*imsize);n++){
      data_t *inp = in + (imsize * (n / imsize));
      data_t *outp = out + n;
      index_t *mapp = map + (n % imsize);
      *outp = inp[*mapp];
    }
  }
  free(map);
}


int stackmean(data_t *in, data_t *mout, long int *nout, int ndims, index_t *dims, int norm){
  index_t nimages = dims[0];
  index_t M = dims[ndims-1];
  index_t N = dims[ndims-2];
  index_t imsize = N*M;

  int retval = 0;

  long int **nvalues;
  data_t **mean;

  int num_threads;

  int x;
  for(x=1;x<(ndims-2);x++){
    nimages = nimages * dims[x];
  }

  // Get the maximum threads 

  int max_threads = omp_get_max_threads(); 
  if(!(nvalues = malloc(sizeof(long int *) * max_threads))){
    return 1;
  }
  if(!(mean = malloc(sizeof(data_t *) * max_threads))){
    free(nvalues);
    return 1;
  }

#pragma omp parallel shared(nvalues, mean, num_threads, imsize, in, retval)
  {
    // Allocate both a result array and an array for the number of values

#pragma omp single
    num_threads = omp_get_num_threads();

    int thread_num = omp_get_thread_num();

    data_t *_mean = calloc(imsize, sizeof(data_t));
    long int *_nvalues = calloc(imsize, sizeof(long int));
    mean[thread_num] = _mean;
    nvalues[thread_num] = _nvalues;

    // Test if we have the memory allocated
    if((_mean != NULL) && (_nvalues != NULL)){

      // Now do the actual mean calculation
      int i;
#pragma omp for private(i) 
      for(i=0;i<nimages;i++){
        int j;
        for(j=0;j<imsize;j++){
          data_t ival = in[(i * imsize + j)];
          if(!isnan(ival)){
            _mean[j] = _mean[j] + ival;
            _nvalues[j]++;
          }
        }
      }
    }

  } // pragma omp paralell

  // Now calculate the mean 

  int n,i;
  for(n=1;n<num_threads;n++){
    for(i=0;i<imsize;i++){
      mean[0][i] += mean[n][i];
      nvalues[0][i] += nvalues[n][i];
    }
  }

  for(i=0;i<imsize;i++){
    nout[i] = nvalues[0][i];
    if(norm){
      if(nvalues[0][i]){
        mout[i] = mean[0][i] / nvalues[0][i];
      } else {
        mout[i] = 0.0;
        nout[i] = 0;
      }
    } else {
      mout[i] = mean[0][i];
    }
  }

  // free up all memory
  
  for(n=0;n<num_threads;n++){
    if(mean[n]) {
      free(mean[n]);
    } else {
      retval = 2; 
    }
    if(nvalues[n]) {
      free(nvalues[n]);
    } else {
      retval = 2;
    }
  }

  free(mean);
  free(nvalues);

  return retval;
}