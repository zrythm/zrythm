/*****************************************************************************
*   Gnome Wave Cleaner Libs, Version 0.02
*   Copyright (C) 2001, 2002, 2003 Jeffrey J. Welty
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

/* gwc_lib.h */

#ifndef GWC_LIBS_HEADER
#define GWC_LIBS_HEADER

extern "C" {
  #include "../kissfft/kiss_fft.h"
  #include "../kissfft/kiss_fftr.h"
}

#define GWC_LIB_VERSION_MAJOR 0
#define GWC_LIB_VERSION_MINOR 1
#define GWC_LIB_VERSION "0.1"

/* defs for declicking results */
#define SINGULAR_MATRIX 0
#define REPAIR_SUCCESS 1
#define REPAIR_CLIPPED 2
#define REPAIR_FAILURE 3
#define DETECT_ONLY 4

/* defs for denoise */
#define DENOISE_MAX_FFT 32768

#define DENOISE_WINDOW_BLACKMAN 0
#define DENOISE_WINDOW_BLACKMAN_HYBRID 1
#define DENOISE_WINDOW_HANNING_OVERLAP_ADD 2

#define DENOISE_WEINER 0
#define DENOISE_POWER_SPECTRAL_SUBTRACT 1
#define DENOISE_EM 2
#define DENOISE_LORBER 3
#define DENOISE_MAGNITUDE_SPECTRAL_SUBTRACT 4

#define LC 0
#define RC 1
#define GWC_LEFT_CHANNEL 1
#define GWC_RIGHT_CHANNEL 2
#define N_CHANNELS 2

typedef struct {
    /***** VARIABLES YOU NEED TO DEAL WITH ******/
    int n_channels ; /* number of channels of audio to process */
    long tot_samples_to_process ; /* total number of samples you want to process */

    long n_samples_requested ; /* filled in by gwc-libs, the number of samples you need to supply
				 to the input buffer */
    long n_samples_released ; /* number of samples available from the output buffer, take them if this is
			        a positive number! */

    kiss_fft_scalar *in[N_CHANNELS] ; /* input buffer, you will load sample data here */
    kiss_fft_scalar *out[N_CHANNELS] ; /* output buffer, you will get processed samples here */

    int FFT_SIZE ; /* make this a power of 2, up to DENOISE_MAX_FFT */
    int channel_mask ; /* GWC_LEFT_CHANNEL, GWC_RIGHT_CHANNEL or (GWC_LEFT_CHANNEL|GWC_RIGHT_CHANNEL) */
    int window_type ; /* one of the DENOISE_WINDOW_* types above */
    int noise_suppression_method ; /* one of the DENOISE_* types above */
    int smoothness ; /* 1 to any number, 11 is as high as you need in any case */
    double amount ; /* amount of noise reduction to apply on each frame, 1=all, 0=none */
    double dn_gamma ; /* gamma value for Lorber, Ephram denoise methods .5 to .95 */

    /****** THESE VARIABLES ARE FOR GWC INTERNAL USE ONLY ******/
    kiss_fftr_cfg pFor, pBak; /* don't fiddle with this */
    int prev_sample[N_CHANNELS] ; /* don't fiddle with this */
    int framenum ; /* don't fiddle with this */
    int stepsize ; /* don't fiddle with this */
    long tot_samples_processed ; /* don't fiddle with this */

    kiss_fft_scalar *noise_max[N_CHANNELS] ; /* internal structs for denoising, don't fiddle with this */
    kiss_fft_scalar *noise_min[N_CHANNELS] ; /* internal structs for denoising, don't fiddle with this */
    kiss_fft_scalar *prev_frame[N_CHANNELS] ; /* internal structs for denoising, don't fiddle with this */
    kiss_fft_scalar *window_coef ; /* internal structs for denoising, don't fiddle with this */
} DENOISE_STATE ;

#define MAX(a,b) (a > b ? (a) : (b))
#define MIN(a,b) (a < b ? (a) : (b))
#define ABS(a) (a < 0.0 ? -(a) : (a))

void denoise_add_noise_snippet (DENOISE_STATE *p);
int denoise_add_noise_snippets (DENOISE_STATE *p,
                                int n_noise_snippets,
                                kiss_fft_scalar *left,
                                kiss_fft_scalar *right,
                                long n_samples);
int denoise_finish (DENOISE_STATE *p);
void denoise_frame (DENOISE_STATE *p);
int denoise_start (DENOISE_STATE *p);

#endif
