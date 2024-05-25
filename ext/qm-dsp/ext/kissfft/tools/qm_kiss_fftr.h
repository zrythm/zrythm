#ifndef KISS_FTR_H
#define KISS_FTR_H

#include "qm_kiss_fft.h"
#ifdef __cplusplus
extern "C" {
#endif

    
/* 
 
 Real optimized version can save about 45% cpu time vs. complex fft of a real seq.

 
 
 */

typedef struct qm_kiss_fftr_state *qm_kiss_fftr_cfg;


qm_kiss_fftr_cfg qm_kiss_fftr_alloc(int nfft,int inverse_fft,void * mem, size_t * lenmem);
/*
 nfft must be even

 If you don't care to allocate space, use mem = lenmem = NULL 
*/


void qm_kiss_fftr(qm_kiss_fftr_cfg cfg,const qm_kiss_fft_scalar *timedata,qm_kiss_fft_cpx *freqdata);
/*
 input timedata has nfft scalar points
 output freqdata has nfft/2+1 complex points
*/

void qm_kiss_fftri(qm_kiss_fftr_cfg cfg,const qm_kiss_fft_cpx *freqdata,qm_kiss_fft_scalar *timedata);
/*
 input freqdata has  nfft/2+1 complex points
 output timedata has nfft scalar points
*/

#define qm_kiss_fftr_free free

#ifdef __cplusplus
}
#endif
#endif
