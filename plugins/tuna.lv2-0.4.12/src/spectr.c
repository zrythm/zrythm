/* Bandpass filter
 *
 * Copyright (C) 2013 Robin Gareus <robin@gareus.org>
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

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>
#include <assert.h>

#if __cplusplus >= 201103L || ((defined __APPLE__ || defined __FreeBSD__) && defined __cplusplus)
# include <complex>
# define csqrt(XX) std::sqrt(XX)
# define creal(XX) std::real(XX)
# define cimag(XX) std::imag(XX)

# ifdef __cpp_lib_complex_udls
    using namespace std::literals::complex_literals;
# endif

# if defined __clang_major__ && __clang_major__ > 4
#  define _I (std::complex<double>(0.0,1.0))
# else
#  define _I ((complex_t)(1i))
# endif

  typedef std::complex<double> complex_t;
#else
# include <complex.h>
# define _I I
  typedef _Complex double complex_t;
#endif

enum filterCoeff {a0 = 0, a1, a2, b0, b1, b2};
enum filterState {z1 = 0, z2};

#define NODENORMAL (1e-12)
#define MAXORDER (6)

struct Filter {
	double W[MAXORDER];
	double z[2];
};

struct FilterBank {
	struct Filter f[MAXORDER];
	uint32_t filter_stages;
	bool ac;
};

static inline double
proc_one(struct Filter * const f, const double in)
{

	const double y = f->W[b0] * in + f->z[z1];
	f->z[z1]       = f->W[b1] * in - f->W[a1] * y + f->z[z2];
	f->z[z2]       = f->W[b2] * in - f->W[a2] * y;
	return y;
}

static inline float
bandpass_process(struct FilterBank * const fb, const float in)
{
	fb->ac = !fb->ac;
	double out = in + ((fb->ac) ? NODENORMAL : -NODENORMAL);
	for (uint32_t i = 0; i < fb->filter_stages; ++i) {
		out = proc_one(&fb->f[i], out);
	}
	return out;
}

static void
bandpass_setup(struct FilterBank *fb,
		double rate,
		double freq,
		double band,
		int    order
		) {

	/* must be an even number for the algorithm below */
	fb->filter_stages = order;

	assert (order > 0 && (order%2) == 0 && order <= MAXORDER);
	assert (band > 0);

	for (uint32_t i = 0; i < fb->filter_stages; ++i) {
		fb->f[i].z[z1] = fb->f[i].z[z2] = 0;
	}

	const double _wc = 2. * M_PI * freq / rate;
	const double _ww = 2. * M_PI * band / rate;

	double wl = _wc - (_ww / 2.);
	double wu = _wc + (_ww / 2.);

	if (wu > M_PI - 1e-9) {
		/* limit band to below nyquist */
		wu = M_PI - 1e-9;
		fprintf(stderr, "tuna.lv2: band f:%9.2fHz (%.2fHz -> %.2fHz) exceeds nysquist (%.0f/2)\n",
				freq, freq-band/2, freq+band/2, rate);
		fprintf(stderr, "tuna.lv2: shifted to f:%.2fHz (%.2fHz -> %.2fHz)\n",
				rate * (wu + wl) *.25 / M_PI,
				rate * wl * .5 / M_PI,
				rate * wu * .5 / M_PI);
	}
	if (wl < 1e-9) {
		wl = 1e-9;
		fprintf(stderr, "tuna.lv2: band f:%9.2fHz (%.2fHz -> %.2fHz) contains sub-bass frequencies\n",
				freq, freq-band/2, freq+band/2);
		fprintf(stderr, "tuna.lv2: shifted to f:%.2fHz (%.2fHz -> %.2fHz)\n",
				rate * (wu + wl) *.25 / M_PI,
				rate * wl * .5 / M_PI,
				rate * wu * .5 / M_PI);
	}

	wu *= .5; wl *= .5;
	assert (wu > wl);

	const double c_a =      cos (wu + wl) / cos (wu - wl);
	const double c_b = 1. / tan (wu - wl);
	const double w   = 2. * atan (sqrt (tan (wu) * tan(wl)));

	const double c_a2 = c_a * c_a;
	const double c_b2 = c_b * c_b;
	const double ab_2 = 2. * c_a * c_b;

	/* bilinear transform coefficients into z-domain */
	for (uint32_t i = 0; i < fb->filter_stages / 2; ++i) {
		const double omega =  M_PI_2 + (2 * i + 1) * M_PI / (2. * (double)fb->filter_stages);
		complex_t p = cos (omega) +  _I * sin (omega);

		const complex_t c = (1. + p) / (1. - p);
		const complex_t d = 2 * (c_b - 1) * c + 2 * (1 + c_b);
		complex_t v;

		v = (4 * (c_b2 * (c_a2 - 1) + 1)) * c;
		v += 8 * (c_b2 * (c_a2 - 1) - 1);
		v *= c;
		v += 4 * (c_b2 * (c_a2 - 1) + 1);
		v = csqrt (v);

		const complex_t u0 = ab_2 + creal(v * -1.) + ab_2 * creal(c) + _I * (cimag(v * -1.) + ab_2 * cimag(c));
		const complex_t u1 = ab_2 + creal( v) + ab_2 * creal(c) + _I * (cimag( v) + ab_2 * cimag(c));

#define ASSIGN_BP(FLT, PC, odd) \
	{ \
		const complex_t P = PC; \
		(FLT).W[a0] = 1.; \
		(FLT).W[a1] = -2 * creal(P); \
		(FLT).W[a2] = creal(P) * creal(P) + cimag(P) * cimag(P); \
		(FLT).W[b0] = 1.; \
		(FLT).W[b1] = (odd) ? -2. : 2.; \
		(FLT).W[b2] = 1.; \
	}
		ASSIGN_BP(fb->f[2*i],   u0/d, 0);
		ASSIGN_BP(fb->f[2*i+1], u1/d, 1);
#undef ASSIGN_BP
	}

	/* normalize */
	const double cos_w = cos (-w);
	const double sin_w = sin (-w);
	const double cos_w2 = cos (-2. * w);
	const double sin_w2 = sin (-2. * w);
	complex_t ch = 1;
	complex_t cb = 1;
	for (uint32_t i = 0; i < fb->filter_stages; ++i) {
		ch *= ((1 + fb->f[i].W[b1] * cos_w) + cos_w2)
			  + _I * ((fb->f[i].W[b1] * sin_w) + sin_w2);
		cb *= ((1 + fb->f[i].W[a1] * cos_w) + fb->f[i].W[a2] * cos_w2)
			  + _I * ((fb->f[i].W[a1] * sin_w) + fb->f[i].W[a2] * sin_w2);
	}

	const complex_t scale = cb / ch;
	fb->f[0].W[b0] *= creal(scale);
	fb->f[0].W[b1] *= creal(scale);
	fb->f[0].W[b2] *= creal(scale);

#ifdef DEBUG_SPECTR
	printf("CFG SR:%f FQ:%f BW:%f O:%d\n", rate, freq, band, order);
	printf("SCALE (%g,  %g)\n", creal(scale), cimag(scale));
	for (uint32_t i = 0; i < fb->filter_stages; ++i) {
		struct Filter *flt = &fb->f[i];
		printf("%d: %g %g %g  %+g %+g %+g\n", i,
				flt->W[a0], flt->W[a1], flt->W[a2],
				flt->W[b0], flt->W[b1], flt->W[b2]);
	}
#endif
}
