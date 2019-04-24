/*****************************************************************************
*   Gnome Wave Cleaner Version 0.19
*   Copyright (C) 2001 Jeffrey J. Welty
*   
*   This program is free software; you can redistribute it and/or
*   modify it under the terms of the GNU General Public License
*   as published by the Free Software Foundation; either version 2
*   of the License, or (at your option) any later version.
*   
*   This program is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*   
*   You should have received a copy of the GNU General Public License
*   along with this program; if not, write to the Free Software
*   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*******************************************************************************/

/* denoise.c */

#include "gwc_lib.h"
#include <stdlib.h>
#include <math.h>
#include <float.h>
#include <string.h>

double gain_weiner(double Yk2, double Dk2)
{
    double gain ;
    double Xk2 = Yk2 - Dk2 ;

    if(Yk2 > Dk2)
	gain = (Xk2) / (Xk2+Dk2) ;
    else
	gain = 0.0 ;

    return gain ;
}

double gain_power_subtraction(double Yk2, double Dk2)
{
    double level = MAX(Yk2-Dk2, 0.0) ;

    if(Yk2 > DBL_MIN)
	return level/Yk2 ;
    else
	return 0.0 ;
}

double gain_lorber(double snr)
{
    double snr_db = log10(snr) ;

    double alpha = MAX((3.0 - 0.10*snr_db), 1.0) ;

    double gain = sqrt(1.0 - alpha/(snr+1)) ;

    if(gain > DBL_MIN)
	return gain ;
    else
	return 0.0 ;
}

#define SLOW_EM
#ifdef SLOW_EM
double hypergeom(double theta)
{
    double i0(double), i1(double) ;
    if(theta < 7.389056)
	return exp(-theta/2.0)*(1.0+theta*i0(theta/2.0)+theta*i1(theta/2.0)) ;
    else
	return exp(0.09379 + 0.50447*log(theta)) ;
}

double gain_em(double Rprio, double Rpost, double alpha)
{
    /* Ephraim-Malah classic noise suppression, from 1984 paper */

    double gain = 0.886226925*sqrt(1.0/(1.0+Rpost)*(Rprio/(1.0+Rprio))) ;

    gain *= hypergeom((1.0+Rpost)*(Rprio/(1.0+Rprio))) ;

    return gain ;
}
#else

double gain_em(double Rprio, double Rpost, double alpha)
{
    /* Ephraim-Malah noise suppression, from Godsill and Wolfe 2001 paper */
    double r = MAX(Rprio/(1.0+Rprio),DBL_MIN) ;
    double V = (1.0+Rpost)/r  ;

    return sqrt( (1.0+V)/Rpost * r  ) ;
}
#endif

double blackman(int k, int N)
{
    double p = ((double)(k))/(double)(N-1) ;
    return 0.42-0.5*cos(2.0*M_PI*p) + 0.08*cos(4.0*M_PI*p) ;
}

double hanning(int k, int N)
{
    double p = ((double)(k))/(double)(N-1) ;
    return 0.5 - 0.5 * cos(2.0*M_PI*p) ;
}

double blackman_hybrid (int k, int n_flat, int N)
{
    if(k >= (N-n_flat)/2 && k <= n_flat+(N-n_flat)/2-1) {
	    return 1.0 ;
    } else {
	    double p ;
	    if(k >= n_flat+(N-n_flat)/2-1) k -= n_flat ;
	    p = (double)(k)/(double)(N-n_flat-1) ;
	    return 0.42-0.5*cos(2.0*M_PI*p) + 0.08*cos(4.0*M_PI*p) ;
    }
}

double fft_window (int k, int N, int window_type)
{
    if(window_type == DENOISE_WINDOW_BLACKMAN) {
	return blackman(k, N) ;
    } else if(window_type == DENOISE_WINDOW_BLACKMAN_HYBRID) {
	return blackman_hybrid(k, N-N/4, N) ;
    } else if(window_type == DENOISE_WINDOW_HANNING_OVERLAP_ADD) {
	return hanning(k, N) ;
    }

    return 0.0 ;
}

void fft_remove_noise (kiss_fft_scalar sample[],
                       kiss_fft_scalar noise_min2[],
                       kiss_fft_scalar noise_max2[],
                       rfftw_plan *pFor,
                       rfftw_plan *pBak,
		               DENOISE_STATE *p,
		               int ch)
{
    int k ;
    kiss_fft_scalar windowed[DENOISE_MAX_FFT], noise2[DENOISE_MAX_FFT] ;
    kiss_fft_cpx out[DENOISE_MAX_FFT] ;
    kiss_fft_scalar Y2[DENOISE_MAX_FFT/2+1] ;
    kiss_fft_scalar maskedY2[DENOISE_MAX_FFT/2+1] ;
    kiss_fft_scalar gain_k[DENOISE_MAX_FFT] ;
    static kiss_fft_scalar bsig_prev[2][DENOISE_MAX_FFT],
                           bY2_prev[2][DENOISE_MAX_FFT/2+1],
                           bgain_prev[2][DENOISE_MAX_FFT/2+1] ;
    kiss_fft_scalar *sig_prev,*Y2_prev,*gain_prev ;

    sig_prev = bsig_prev[ch] ;
    Y2_prev = bY2_prev[ch] ;
    gain_prev = bgain_prev[ch] ;

    for(k = 0 ; k < p->FFT_SIZE ; k++) {
	    windowed[k] = sample[k]*p->window_coef[k] ;
	    noise2[k] = noise_max2[k] ;
    }

    kiss_fftr (*pFor, windowed, out);

    for (k = 1; k <= p->FFT_SIZE / 2 ; ++k) {
	    Y2[k] = (k < p->FFT_SIZE / 2) ?  (out[k].r * out[k].r + out[k].i * out[k].i) : (out[k].r * out[k].r) ;
    }

    if(p->noise_suppression_method == DENOISE_LORBER) {
	    for (k = 1; k <= p->FFT_SIZE/2 ; ++k)
	    {
	        double sum = 0 ;
	        double sum_wgts = 0 ;
	        int j ;

	        int j1 = MAX(k-10,1) ;
	        int j2 = MIN(k+10,p->FFT_SIZE/2) ;

	        for(j = j1 ; j <= j2 ; j++) {
		        double d = ABS(j-k)+1.0 ;
		        double wgt = 1./d ;
		        sum += Y2[k]*wgt ;
		        sum_wgts += wgt ;
	        }

	        maskedY2[k] = sum / (sum_wgts+1.e-300) ;
	    }
    }

    for (k = 1; k <= p->FFT_SIZE/2 ; ++k) {
	    if(noise2[k] > DBL_MIN)
	    {
	        double gain, Fk, Gk ;

	        if(p->noise_suppression_method == DENOISE_EM) {
		        double Rpost = MAX(Y2[k]/noise2[k]-1.0, 0.0) ;
		        double alpha = p->dn_gamma ;
		        double Rprio ;

		        if(p->prev_sample[ch] == 1)
		            Rprio = (1.0-alpha)*MAX(Rpost, 0.0)+alpha*gain_prev[k]*gain_prev[k]*Y2_prev[k]/noise2[k] ;
		        else
		            Rprio = MAX(Rpost, 0.0) ;

		        gain = gain_em(Rprio, Rpost, alpha) ;
        /*  		g_print("Rpost:%lg Rprio:%lg gain:%lg gain_prev:%lg y2_prev:%lg\n", Rpost, Rprio, gain, gain_prev[k], Y2_prev[k]) ;  */
		        gain_prev[k] = gain ;
		        Y2_prev[k] = Y2[k] ;
	        } else if(p->noise_suppression_method == DENOISE_LORBER) {
		        double SNRlocal = maskedY2[k]/noise2[k]-1.0 ;
		        double SNRfilt, SNRprio ;

		        if(p->prev_sample[ch] == 1) {
		            SNRfilt = (1.-p->dn_gamma)*SNRlocal + p->dn_gamma*(sig_prev[k]/noise2[k]) ;
		            SNRprio = SNRfilt ; /* note, could use another parameter, like p->dn_gamma, here to compute SNRprio */
		        }else {
		            SNRfilt = SNRlocal ;
		            SNRprio = SNRfilt ;
		        }
		        gain = gain_lorber(SNRprio) ;
		        sig_prev[k] = MAX(Y2[k]*gain,0.0) ;
	        } else if(p->noise_suppression_method == DENOISE_WEINER)
        		gain = gain_weiner(Y2[k], noise2[k]) ;
	        else
        		gain = gain_power_subtraction(Y2[k], noise2[k]) ;

	        Fk = p->amount*(1.0-gain) ;

	        if(Fk < 0.0) Fk = 0.0 ;
	        if(Fk > 1.0) Fk = 1.0 ;

	        Gk =  1.0 - Fk ;

	        out[k].r *= Gk ;
	        if(k < p->FFT_SIZE/2) out[k].i *= Gk ;

	        gain_k[k] = Gk ;
	    }
    }

    /* the inverse fft */
    kiss_fftri (*pBak, out, windowed);

    for(k = 0 ; k < p->FFT_SIZE ; k++)
    	windowed[k] /= (double)(p->FFT_SIZE) ;

    if(p->window_type != DENOISE_WINDOW_HANNING_OVERLAP_ADD) {
	    /* merge results back into sample data based on window function */
	    for(k = 0 ; k < p->FFT_SIZE ; k++) {
	        double w = p->window_coef[k] ;
	        sample[k] = (1.0-w) * sample[k] + windowed[k] ;
	    }
    } else {
	    /* make sure the tails of the sample approach zero magnitude */
	    double offset = windowed[0] ;
	    double hs1 = p->FFT_SIZE/2 - 1 ;

	    if(p->framenum > 1) {
	        for(k = 0 ; k < p->FFT_SIZE/2 ; k++) {
		    double p = (hs1-(double)k)/hs1 ;
		    sample[k] = windowed[k] - offset*p ;
	        }
	    } else {
	        printf("FRAMENUM 0 in fft_denoise\n") ;
	    }

	    offset = windowed[p->FFT_SIZE-1] ;
	    for(k = p->FFT_SIZE/2 ; k < p->FFT_SIZE ; k++) {
	        double p = ((double)k-hs1)/hs1 ;
	        sample[k] = windowed[k] - offset*p ;
	    }
    }

    p->prev_sample[ch] = 1 ;
}


int denoise_start(DENOISE_STATE *p)
{
    int i,k ;

    p->window_coef = (kiss_fft_scalar *) calloc(p->FFT_SIZE, sizeof(kiss_fft_scalar)) ;

    for(i = 0 ; i < p->n_channels ; i++) {
	    p->in[i] = (kiss_fft_scalar *) calloc(p->FFT_SIZE, sizeof(kiss_fft_scalar)) ;
	    p->out[i] = (kiss_fft_scalar *) calloc(p->FFT_SIZE, sizeof(kiss_fft_scalar)) ;
	    p->noise_max[i] = (kiss_fft_scalar *) calloc(p->FFT_SIZE, sizeof(kiss_fft_scalar)) ;
	    p->noise_min[i] = (kiss_fft_scalar *) calloc(p->FFT_SIZE, sizeof(kiss_fft_scalar)) ;
	    p->prev_frame[i] = (kiss_fft_scalar *) calloc(p->FFT_SIZE, sizeof(kiss_fft_scalar)) ;
	    p->prev_sample[i] = 0 ;
    }


    for(k = 0 ; k < p->FFT_SIZE ; k++) {
	    p->window_coef[k] = fft_window(k,p->FFT_SIZE, p->window_type) ;
	    p->noise_max[LC][k] = 0.0 ;
	    p->noise_max[RC][k] = 0.0 ;
	    p->noise_min[LC][k] = 0.0 ;
	    p->noise_min[RC][k] = 0.0 ;
	    p->prev_frame[LC][k] = 0.0 ;
	    p->prev_frame[RC][k] = 0.0 ;
    }

    p->pFor = kiss_fftr_alloc (p->FFT_SIZE, 0, NULL, NULL);
    p->pBak = kiss_fftr_alloc (p->FFT_SIZE, 1, NULL, NULL);

    p->framenum = 0 ;
    p->tot_samples_processed = 0 ;

    p->n_samples_requested = p->FFT_SIZE ;

    if(p->window_type == DENOISE_WINDOW_BLACKMAN)
    	p->stepsize = p->FFT_SIZE/p->smoothness ;
    else
    	p->stepsize = 3*p->FFT_SIZE/4 ;

    return 0 ;
}

int denoise_finish(DENOISE_STATE *p)
{
    int i ;

    free (p->pFor);
    free (p->pBak);

    free (p->window_coef) ;

    for(i = 0 ; i < p->n_channels ; i++) {
	    free (p->in[i]) ;
	    free (p->out[i]) ;
	    free (p->noise_max[i]) ;
	    free (p->noise_min[i]) ;
	    free (p->prev_frame[i]) ;
    }

    kiss_fft_cleanup();

    return 0 ;
}

void denoise_add_noise_snippet(DENOISE_STATE *p)
{
    int k ;

    kiss_fft_cpx tmp[DENOISE_MAX_FFT] ;

    for(k = 0 ; k < p->FFT_SIZE ; k++) {
	    p->in[LC][k] *= p->window_coef[k] ;
	    p->in[RC][k] *= p->window_coef[k] ;
    }

    kiss_fftr (p->pFor, p->in[LC], tmp);

    /* convert noise sample to power spectrum */
    for(k = 1 ; k <= p->FFT_SIZE/2 ; k++) {
	    if(k < p->FFT_SIZE/2) {
	        p->noise_min[LC][k] = MIN(p->noise_min[LC][k], tmp[k].r * tmp[k].r + tmp[k].i * tmp[k].i) ;
	        p->noise_max[LC][k] = MAX(p->noise_max[LC][k], tmp[k].r * tmp[k].r + tmp[k].i * tmp[k].i) ;
	    } else {
	        /* Nyquist Frequency */
	        p->noise_min[LC][k] = MIN(p->noise_min[LC][k], tmp[k].r * tmp[k].r) ;
	        p->noise_max[LC][k] = MAX(p->noise_max[LC][k], tmp[k].r * tmp[k].r) ;
	    }
    }

    kiss_fftr (p->pFor, p->in[RC], tmp);

    /* convert noise sample to power spectrum */
    for(k = 1 ; k <= p->FFT_SIZE/2 ; k++) {
	    if(k < p->FFT_SIZE/2) {
	        p->noise_min[RC][k] = MIN(p->noise_min[RC][k], tmp[k].r * tmp[k].r + tmp[k].i * tmp[k].i) ;
	        p->noise_max[RC][k] = MAX(p->noise_max[RC][k], tmp[k].r * tmp[k].r + tmp[k].i * tmp[k].i) ;
	    } else {
	        /* Nyquist Frequency */
	        p->noise_min[RC][k] = MIN(p->noise_min[RC][k], tmp[k].r * tmp[k].r) ;
	        p->noise_max[RC][k] = MAX(p->noise_max[RC][k], tmp[k].r * tmp[k].r) ;
	    }
    }
}

int denoise_add_noise_snippets(DENOISE_STATE *p, int n_noise_snippets, kiss_fft_scalar *left, kiss_fft_scalar *right, long n_samples)
{
    long i,k ;

    if(n_samples < p->FFT_SIZE) return 1 ;

    /* want n_noise_snippets evenly spaced across the "noise only" region.
     * The first snippets starts at "noise_start", but the first byte of
     * the last noise snippets will be at "noise_end-FFT_SIZE-1"
     * the snippets are FFT_SIZE samples long
    */
    for(i = 0 ; i < n_noise_snippets ; i++)
    {
	    long first = i*((n_samples-1)-p->FFT_SIZE)/n_noise_snippets ;

	    for(k = 0 ; k < p->FFT_SIZE ; k++) {
	        p->in[LC][k] = left[first+k] ;
	        p->in[RC][k] = right[first+k] ;
	    }

	    denoise_add_noise_snippet(p) ;
    }

    return 0 ;
}

long zero_pad(DENOISE_STATE *p, long frame_len)
{
    int ch, k ;
    long n_remaining = p->tot_samples_to_process-p->tot_samples_processed ;

    if(n_remaining < frame_len)
    {
	    /* zero pad the remaining samples */
	    long n_to_pad = frame_len - n_remaining ;

	    fprintf(stderr, "n to pad %ld\n", n_to_pad) ;

	    for(ch = 0 ; ch < p->n_channels ; ch++) {
	        int ch_bit = 1 << ch ;
	        if(p->channel_mask & ch_bit) {
		    for(k = 0 ; k < n_to_pad ; k++)
		        p->in[ch][k+n_remaining] = 0.0 ;
	        }
	    }

	    return 1 ;
    }

    return 0 ;
}

#define MEMCPY(d,s,n) memcpy(d,s,(n)*sizeof(kiss_fft_scalar))

void denoise_frame(DENOISE_STATE *p)
{
    int ch, k ;
    int zero_padded = 0 ;

    if(p->amount > DBL_MIN) {

	if(p->window_type == DENOISE_WINDOW_HANNING_OVERLAP_ADD) {
	    zero_padded = zero_pad(p, p->FFT_SIZE/2) ;

	    for(ch = 0 ; ch < p->n_channels ; ch++) {
		int ch_bit = 1 << ch ;
		if(p->channel_mask & ch_bit) {
		    if(p->framenum == 0) {
#ifdef MEMCPY
			MEMCPY(p->out[ch], p->in[ch], p->FFT_SIZE) ;
#else
			for(k = 0 ; k < p->FFT_SIZE ; k++) {
			    p->out[ch][k] = p->in[ch][k] ;
			}
#endif
		    } else {
#ifdef MEMCPY
			MEMCPY(p->out[ch], p->prev_frame[ch], p->FFT_SIZE/2) ;
			MEMCPY(&p->out[ch][p->FFT_SIZE/2], p->in[ch], p->FFT_SIZE/2) ;
#else
			for(k = 0 ; k < p->FFT_SIZE/2 ; k++) {
			    /* find the first 1/2 of the input frame saved in the first 1/2 of the prev_frame buffer */
			    p->out[ch][k] = p->prev_frame[ch][k] ;
			    /* find the last 1/2 of the input frame on the input buffer */
			    p->out[ch][k+p->FFT_SIZE/2] = p->in[ch][k] ;
			}
#endif
		    }
		}
	    }
	} else {
	    zero_padded = zero_pad(p, p->stepsize) ;

	    for(ch = 0 ; ch < p->n_channels ; ch++) {
		int ch_bit = 1 << ch ;
		if(p->channel_mask & ch_bit) {
#ifdef MEMCPY
		    MEMCPY(p->out[ch], p->prev_frame[ch], (p->FFT_SIZE/2-p->stepsize)) ;
		    MEMCPY(&p->out[ch][p->FFT_SIZE/2-p->stepsize], p->in[ch], p->stepsize) ;
#else
		    for(k = 0 ; k < p->FFT_SIZE-p->stepsize ; k++) {
			/* copy last segment of this output frame to the start*/
			p->out[ch][k] = p->prev_frame[ch][k] ;
		    }

		    for(k = 0 ; k < p->stepsize ; k++) {
			/* gather the new incremental data for this output frame */
			p->out[ch][k+p->FFT_SIZE-p->stepsize] = p->in[ch][k] ;
		    }
#endif
		}
	    }
	}

	p->tot_samples_processed += p->n_samples_requested ;

	if(p->framenum == 0) {
	    printf("FRAME 0\n") ;
	    for(k = 0 ; k < p->FFT_SIZE ; k++) {
		if(k < p->FFT_SIZE/2) {
		    p->window_coef[k] = 1.0 ;
		}
	    }
	    p->framenum = 1 ;
	} else if(p->framenum == 1) {
	    for(k = 0 ; k < p->FFT_SIZE ; k++) {
		p->window_coef[k] = fft_window(k,p->FFT_SIZE, p->window_type) ;
	    }
	    p->framenum = 2 ;
	}

	for(ch = 0 ; ch < p->n_channels ; ch++) {
	    int ch_bit = 1 << ch ;
	    if(p->channel_mask & ch_bit)
		fft_remove_noise(p->out[ch], p->noise_min[ch], p->noise_max[ch],
			      &p->pFor, &p->pBak, p, ch) ;
	}

	if(p->window_type == DENOISE_WINDOW_HANNING_OVERLAP_ADD) {

	    for(ch = 0 ; ch < p->n_channels ; ch++) {
		int ch_bit = 1 << ch ;
		if(p->channel_mask & ch_bit) {
#ifdef MEMCPY
		    /* save the last half of this input frame */
		    if(p->framenum < 2)
			MEMCPY(p->prev_frame[ch], &p->in[ch][p->FFT_SIZE/2], p->FFT_SIZE/2) ;
		    else
			MEMCPY(p->prev_frame[ch], p->in[ch], p->FFT_SIZE/2) ;
#endif
		    for(k = 0 ; k < p->FFT_SIZE/2 ; k++) {
			/* add in the last half of the previous output frame */
			p->out[ch][k] += p->prev_frame[ch][k+p->FFT_SIZE/2] ;

#ifndef MEMCPY
			/* save the last half of this output frame */
			p->prev_frame[ch][k+p->FFT_SIZE/2] = p->out[ch][k+p->FFT_SIZE/2] ;

			/* save the last half of this input frame */
			if(p->framenum < 2)
			    p->prev_frame[ch][k] = p->in[ch][k+p->FFT_SIZE/2] ;
			else
			    p->prev_frame[ch][k] = p->in[ch][k] ;
#endif
		    }
#ifdef MEMCPY
		    MEMCPY(&p->prev_frame[ch][p->FFT_SIZE/2], &p->out[ch][p->FFT_SIZE/2], p->FFT_SIZE/2) ;
#endif
		}
	    }

	    p->n_samples_released = p->FFT_SIZE/2 ;
	    {
		long n_remaining = p->tot_samples_to_process-p->tot_samples_processed ;
		p->n_samples_requested = MIN(p->FFT_SIZE/2,n_remaining) ;
	    }

	} else {

	    for(ch = 0 ; ch < p->n_channels ; ch++) {
		int ch_bit = 1 << ch ;
		if(p->channel_mask & ch_bit) {
#ifdef MEMCPY
		    MEMCPY(p->prev_frame[ch], &p->out[ch][p->FFT_SIZE/2], p->FFT_SIZE/2-p->stepsize) ;
#else
		    for(k = 0 ; k < p->FFT_SIZE-p->stepsize ; k++) {
			/* save the last segment of this output frame */
			p->prev_frame[ch][k] = p->out[ch][k+p->stepsize] ;
		    }
#endif
		}
	    }

	    p->n_samples_released = p->stepsize ;
	    {
		long n_remaining = p->tot_samples_to_process-p->tot_samples_processed ;
		p->n_samples_requested = MIN(p->stepsize,n_remaining) ;
	    }
	}


    } else {
	/* no noise reduction requested, just copy in to out */
	int i, k ;

	for(i = 0 ; i < p->n_channels ; i++)
	    /* this should really be done as a memcpy */
	    for(k = 0 ; k < p->FFT_SIZE ; k++)
		p->out[i][k] = p->in[i][k] ;

	p->n_samples_released = p->FFT_SIZE ;
	p->n_samples_requested = p->FFT_SIZE ;
    }
}
