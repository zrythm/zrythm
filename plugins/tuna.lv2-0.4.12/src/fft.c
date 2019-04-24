/* FFT analysis - spectrogram
 * Copyright (C) 2013, 2019 Robin Gareus <robin@gareus.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <fftw3.h>
#include <pthread.h>
#include <stdio.h>
#include <sys/types.h>

#ifndef MIN
#define MIN(A, B) ((A) < (B) ? (A) : (B))
#endif

static pthread_mutex_t fftw_planner_lock = PTHREAD_MUTEX_INITIALIZER;
static unsigned int    instance_count    = 0;

typedef enum {
	W_HANN = 0,
	W_HAMMMIN,
	W_NUTTALL,
	W_BLACKMAN_NUTTALL,
	W_BLACKMAN_HARRIS,
	W_FLAT_TOP
} window_t;

/******************************************************************************
 * internal FFT abstraction
 */
struct FFTAnalysis {
	uint32_t   window_size;
	window_t   window_type;
	uint32_t   data_size;
	double     rate;
	double     freq_per_bin;
	double     phasediff_step;
	float*     window;
	float*     fft_in;
	float*     fft_out;
	float*     power;
	float*     phase;
	float*     phase_h;
	fftwf_plan fftplan;

	float*   ringbuf;
	uint32_t rboff;
	uint32_t smps;
	uint32_t sps;
	uint32_t step;
	double   phasediff_bin;
};

/* ****************************************************************************
 * windows
 */
static double
ft_hannhamm (float* window, uint32_t n, double a, double b)
{
	double sum = 0.0;
	const double c = 2.0 * M_PI / (n - 1.0);
	for (uint32_t i = 0; i < n; ++i) {
		window[i] = a - b * cos (c * i);
		sum += window[i];
	}
	return sum;
}

static double
ft_bnh (float* window, uint32_t n, double a0, double a1, double a2, double a3)
{
	double sum = 0.0;
	const double c  = 2.0 * M_PI / (n - 1.0);
	const double c2 = 2.0 * c;
	const double c3 = 3.0 * c;

	for (uint32_t i = 0; i < n; ++i) {
		window[i] = a0 - a1 * cos(c * i) + a2 * cos(c2 * i) - a3 * cos(c3 * i);
		sum += window[i];
	}
	return sum;
}

static double
ft_flattop (float* window, uint32_t n)
{
	double sum = 0.0;
	const double c  = 2.0 * M_PI / (n - 1.0);
	const double c2 = 2.0 * c;
	const double c3 = 3.0 * c;
	const double c4 = 4.0 * c;

	const double a0 = 1.0;
	const double a1 = 1.93;
	const double a2 = 1.29;
	const double a3 = 0.388;
	const double a4 = 0.028;

	for (uint32_t i = 0; i < n; ++i) {
		window[i] = a0 - a1 * cos(c * i) + a2 * cos(c2 * i) - a3 * cos(c3 * i) + a4 * cos(c4 * i);
		sum += window[i];
	}
	return sum;
}


/* ****************************************************************************
 * internal private functions
 */
static float*
ft_gen_window (struct FFTAnalysis* ft)
{
	if (ft->window) {
		return ft->window;
	}

	ft->window = (float*)malloc (sizeof (float) * ft->window_size);
	double sum = .0;

	/* https://en.wikipedia.org/wiki/Window_function */
	switch (ft->window_type) {
		default:
		case W_HANN:
			sum = ft_hannhamm (ft->window, ft->window_size, .5, .5);
			break;
		case W_HAMMMIN:
			sum = ft_hannhamm (ft->window, ft->window_size, .54, .46);
			break;
		case W_NUTTALL:
			sum = ft_bnh (ft->window, ft->window_size, .355768, .487396, .144232, .012604);
			break;
		case W_BLACKMAN_NUTTALL:
			sum = ft_bnh (ft->window, ft->window_size, .3635819, .4891775, .1365995, .0106411);
			break;
		case W_BLACKMAN_HARRIS:
			sum = ft_bnh (ft->window, ft->window_size, .35875, .48829, .14128, .01168);
			break;
		case W_FLAT_TOP:
			sum = ft_flattop (ft->window, ft->window_size);
			break;
	}

	const double isum = 2.0 / sum;
	for (uint32_t i = 0; i < ft->window_size; i++) {
		ft->window[i] *= isum;
	}

	return ft->window;
}

static void
ft_analyze (struct FFTAnalysis* ft)
{
	fftwf_execute (ft->fftplan);

	memcpy (ft->phase_h, ft->phase, sizeof (float) * ft->data_size);
	ft->power[0] = ft->fft_out[0] * ft->fft_out[0];
	ft->phase[0] = 0;

#define FRe (ft->fft_out[i])
#define FIm (ft->fft_out[ft->window_size - i])
	for (uint32_t i = 1; i < ft->data_size - 1; ++i) {
		ft->power[i] = (FRe * FRe) + (FIm * FIm);
		ft->phase[i] = atan2f (FIm, FRe);
	}
#undef FRe
#undef FIm
}

/******************************************************************************
 * public API (static for direct source inclusion)
 */
#ifndef FFTX_FN_PREFIX
#define FFTX_FN_PREFIX static
#endif

FFTX_FN_PREFIX
void
fftx_reset (struct FFTAnalysis* ft)
{
	for (uint32_t i = 0; i < ft->data_size; ++i) {
		ft->power[i]   = 0;
		ft->phase[i]   = 0;
		ft->phase_h[i] = 0;
	}
	for (uint32_t i = 0; i < ft->window_size; ++i) {
		ft->ringbuf[i] = 0;
		ft->fft_out[i] = 0;
	}
	ft->rboff = 0;
	ft->smps  = 0;
	ft->step  = 0;
}

FFTX_FN_PREFIX
void
fftx_init (struct FFTAnalysis* ft, uint32_t window_size, double rate, double fps)
{
	ft->rate           = rate;
	ft->window_size    = window_size;
	ft->window_type    = W_HANN;
	ft->data_size      = window_size / 2;
	ft->window         = NULL;
	ft->rboff          = 0;
	ft->smps           = 0;
	ft->step           = 0;
	ft->sps            = (fps > 0) ? ceil (rate / fps) : 0;
	ft->freq_per_bin   = ft->rate / ft->data_size / 2.f;
	ft->phasediff_step = M_PI / ft->data_size;
	ft->phasediff_bin  = 0;

	ft->ringbuf = (float*)malloc (window_size * sizeof (float));
	ft->fft_in  = (float*)fftwf_malloc (sizeof (float) * window_size);
	ft->fft_out = (float*)fftwf_malloc (sizeof (float) * window_size);
	ft->power   = (float*)malloc (ft->data_size * sizeof (float));
	ft->phase   = (float*)malloc (ft->data_size * sizeof (float));
	ft->phase_h = (float*)malloc (ft->data_size * sizeof (float));

	fftx_reset (ft);

	pthread_mutex_lock (&fftw_planner_lock);
	ft->fftplan = fftwf_plan_r2r_1d (window_size, ft->fft_in, ft->fft_out, FFTW_R2HC, FFTW_MEASURE);
	++instance_count;
	pthread_mutex_unlock (&fftw_planner_lock);
}

FFTX_FN_PREFIX
void
fftx_set_window (struct FFTAnalysis* ft, window_t type)
{
	if (ft->window_type == type) {
		return;
	}
	ft->window_type = type;
	free (ft->window);
	ft->window = NULL;
}

FFTX_FN_PREFIX
void
fftx_free (struct FFTAnalysis* ft)
{
	if (!ft) {
		return;
	}
	pthread_mutex_lock (&fftw_planner_lock);
	fftwf_destroy_plan (ft->fftplan);
	if (instance_count > 0) {
		--instance_count;
	}
#ifdef WITH_STATIC_FFTW_CLEANUP
	/* use this only when statically linking to a local fftw!
	 *
	 * "After calling fftw_cleanup, all existing plans become undefined,
	 *  and you should not attempt to execute them nor to destroy them."
	 * [http://www.fftw.org/fftw3_doc/Using-Plans.html]
	 *
	 * If libfftwf is shared with other plugins or the host this can
	 * cause undefined behavior.
	 */
	if (instance_count == 0) {
		fftwf_cleanup ();
	}
#endif
	pthread_mutex_unlock (&fftw_planner_lock);
	free (ft->window);
	free (ft->ringbuf);
	fftwf_free (ft->fft_in);
	fftwf_free (ft->fft_out);
	free (ft->power);
	free (ft->phase);
	free (ft->phase_h);
	free (ft);
}

static int
_fftx_run (struct FFTAnalysis* ft,
           const uint32_t n_samples, float const* const data)
{
	assert (n_samples <= ft->window_size);

	float* const f_buf = ft->fft_in;
	float* const r_buf = ft->ringbuf;

	const uint32_t n_off = ft->rboff;
	const uint32_t n_siz = ft->window_size;
	const uint32_t n_old = n_siz - n_samples;

	for (uint32_t i = 0; i < n_samples; ++i) {
		r_buf[(i + n_off) % n_siz] = data[i];
		f_buf[n_old + i]           = data[i];
	}

	ft->rboff = (ft->rboff + n_samples) % n_siz;
#if 1
	ft->smps += n_samples;
	if (ft->smps < ft->sps) {
		return -1;
	}
	ft->step = ft->smps;
	ft->smps = 0;
#else
	ft->step = n_samples;
#endif

	/* copy samples from ringbuffer into fft-buffer */
	const uint32_t p0s = (n_off + n_samples) % n_siz;
	if (p0s + n_old >= n_siz) {
		const uint32_t n_p1 = n_siz - p0s;
		const uint32_t n_p2 = n_old - n_p1;
		memcpy (f_buf, &r_buf[p0s], sizeof (float) * n_p1);
		memcpy (&f_buf[n_p1], &r_buf[0], sizeof (float) * n_p2);
	} else {
		memcpy (&f_buf[0], &r_buf[p0s], sizeof (float) * n_old);
	}

	/* apply window function */
	float const* const window = ft_gen_window (ft);
	for (uint32_t i = 0; i < ft->window_size; i++) {
		ft->fft_in[i] *= window[i];
	}

	/* ..and analyze */
	ft_analyze (ft);

	ft->phasediff_bin = ft->phasediff_step * (double)ft->step;
	return 0;
}

FFTX_FN_PREFIX
int
fftx_run (struct FFTAnalysis* ft,
          const uint32_t n_samples, float const* const data)
{
	if (n_samples <= ft->window_size) {
		return _fftx_run (ft, n_samples, data);
	}

	int      rv = -1;
	uint32_t n  = 0;
	while (n < n_samples) {
		uint32_t step = MIN (ft->window_size, n_samples - n);
		if (!_fftx_run (ft, step, &data[n])) {
			rv = 0;
		}
		n += step;
	}
	return rv;
}

FFTX_FN_PREFIX
void
fa_analyze_dsp (struct FFTAnalysis* ft,
                void (*run) (void*, uint32_t, float*), void* handle)
{
	float* buf = ft->fft_in;

	/* pre-run 8K samples... (re-init/flush effect) */
	uint32_t prerun_n_samples = 8192;
	while (prerun_n_samples > 0) {
		uint32_t n_samples = MIN (prerun_n_samples, ft->window_size);
		memset (buf, 0, sizeof (float) * n_samples);
		run (handle, n_samples, buf);
		prerun_n_samples -= n_samples;
	}

	/* delta impulse */
	memset (buf, 0, sizeof (float) * ft->window_size);
	*buf = 1.0;
	/* call plugin's run() function -- in-place processing */
	run (handle, ft->window_size, buf);
	ft->step = ft->window_size;
	/* ..and analyze */
	ft_analyze (ft);
}

/* ***************************************************************************
 * convenient access functions
 */

FFTX_FN_PREFIX
uint32_t
fftx_bins (struct FFTAnalysis* ft)
{
	return ft->data_size;
}

FFTX_FN_PREFIX
inline float
fast_log2 (float val)
{
	union {
		float f;
		int   i;
	} t;
	t.f                = val;
	int* const exp_ptr = &t.i;
	int        x       = *exp_ptr;
	const int  log_2   = ((x >> 23) & 255) - 128;
	x &= ~(255 << 23);
	x += 127 << 23;
	*exp_ptr = x;
	val      = ((-1.0f / 3) * t.f + 2) * t.f - 2.0f / 3;
	return (val + log_2);
}

FFTX_FN_PREFIX
inline float
fast_log (const float val)
{
	return (fast_log2 (val) * 0.69314718f);
}

FFTX_FN_PREFIX
inline float
fast_log10 (const float val)
{
	return fast_log2 (val) / 3.312500f;
}

FFTX_FN_PREFIX
inline float
fftx_power_to_dB (float a)
{
	/* 10 instead of 20 because of squared signal -- no sqrt(powerp[]) */
	return a > 1e-12 ? 10.0 * fast_log10 (a) : -INFINITY;
}

FFTX_FN_PREFIX
float
fftx_power_at_bin (struct FFTAnalysis* ft, const int b)
{
	return (fftx_power_to_dB (ft->power[b]));
}

FFTX_FN_PREFIX
float
fftx_freq_at_bin (struct FFTAnalysis* ft, const int b)
{
	/* calc phase: difference minus expected difference */
	float phase = ft->phase[b] - ft->phase_h[b] - (float)b * ft->phasediff_bin;
	/* clamp to -M_PI .. M_PI */
	int over = phase / M_PI;
	over += (over >= 0) ? (over & 1) : -(over & 1);
	phase -= M_PI * (float)over;
	/* scale according to overlap */
	phase *= (ft->data_size / ft->step) / M_PI;
	return ft->freq_per_bin * ((float)b + phase);
}
