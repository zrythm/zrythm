/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    Rubber Band Library
    An audio time-stretching and pitch-shifting library.
    Copyright 2007-2024 Particular Programs Ltd.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.

    Alternatively, if you have a valid commercial licence for the
    Rubber Band Library obtained by agreement with the copyright
    holders, you may redistribute and/or modify it under the terms
    described in that licence.

    If you wish to distribute code using the Rubber Band Library
    under terms other than those of the GNU General Public License,
    you must obtain a valid commercial licence before doing so.
*/

#ifndef BOOST_TEST_DYN_LINK
#define BOOST_TEST_DYN_LINK
#endif
#include <boost/test/unit_test.hpp>

#include "../common/FFT.h"

#include <iostream>

#include <cstdio>
#include <cmath>

using namespace RubberBand;

BOOST_AUTO_TEST_SUITE(TestFFT)

#define DEFINE_EPS(fft) \
    float epsf = 1e-6f; \
    double eps; \
    if (fft.getSupportedPrecisions() & FFT::DoublePrecision) { \
        eps = 1e-14; \
    } else { \
        eps = epsf; \
    } \
    (void)epsf; (void)eps;

#define USING_FFT(n) \
    FFT fft(n); \
    DEFINE_EPS(fft);

#define COMPARE(a, b) BOOST_CHECK_SMALL(a-b, eps) 
#define COMPARE_F(a, b) BOOST_CHECK_SMALL(a-b, epsf) 

#define COMPARE_ZERO(a) BOOST_CHECK_SMALL(a, eps)
#define COMPARE_ZERO_F(a) BOOST_CHECK_SMALL(a, epsf)

#define COMPARE_ALL(a, x) \
    for (int cmp_i = 0; cmp_i < (int)(sizeof(a)/sizeof(a[0])); ++cmp_i) { \
        BOOST_CHECK_SMALL(a[cmp_i] - x, eps); \
    }
#define COMPARE_ALL_F(a, x) \
    for (int cmp_i = 0; cmp_i < (int)(sizeof(a)/sizeof(a[0])); ++cmp_i) { \
        BOOST_CHECK_SMALL(a[cmp_i] - x, epsf); \
    }
#define COMPARE_ARR(a, b, n) \
    for (int cmp_i = 0; cmp_i < n; ++cmp_i) { \
        BOOST_CHECK_SMALL(a[cmp_i] - b[cmp_i], eps); \
    }
#define COMPARE_SCALED(a, b, s)						\
    for (int cmp_i = 0; cmp_i < (int)(sizeof(a)/sizeof(a[0])); ++cmp_i) { \
        BOOST_CHECK_SMALL(a[cmp_i]/s - b[cmp_i], eps); \
    }
#define COMPARE_SCALED_N(a, b, n, s) \
    for (int cmp_i = 0; cmp_i < n; ++cmp_i) { \
        BOOST_CHECK_SMALL(a[cmp_i]/s - b[cmp_i], eps); \
    }
#define COMPARE_SCALED_F(a, b, s)						\
    for (int cmp_i = 0; cmp_i < (int)(sizeof(a)/sizeof(a[0])); ++cmp_i) { \
        BOOST_CHECK_SMALL(a[cmp_i]/s - b[cmp_i], epsf); \
    }

#define ONE_IMPL_AUTO_TEST_CASE(name, impl) \
    BOOST_AUTO_TEST_CASE(name##_##impl) \
    { \
        std::set<std::string> impls = FFT::getImplementations(); \
        if (impls.find(#impl) == impls.end()) return; \
        FFT::setDefaultImplementation(#impl); \
        performTest_##name(); \
        FFT::setDefaultImplementation(""); \
    }

// If you add an implementation in FFT.cpp, add it also to
// ALL_IMPL_AUTO_TEST_CASE and all_implementations[] below

#define ALL_IMPL_AUTO_TEST_CASE(name) \
    void performTest_##name (); \
    ONE_IMPL_AUTO_TEST_CASE(name, ipp); \
    ONE_IMPL_AUTO_TEST_CASE(name, vdsp); \
    ONE_IMPL_AUTO_TEST_CASE(name, fftw); \
    ONE_IMPL_AUTO_TEST_CASE(name, kissfft); \
    ONE_IMPL_AUTO_TEST_CASE(name, builtin); \
    ONE_IMPL_AUTO_TEST_CASE(name, dft); \
    void performTest_##name ()

std::string all_implementations[] = {
    "ipp", "vdsp", "fftw", "kissfft", "builtin", "dft"
};

BOOST_AUTO_TEST_CASE(showImplementations)
{
    std::set<std::string> impls = FFT::getImplementations();
    BOOST_TEST_MESSAGE("The following implementations are compiled in and will be tested:");
    for (int i = 0; i < int(sizeof(all_implementations)/sizeof(all_implementations[0])); ++i) {
        if (impls.find(all_implementations[i]) != impls.end()) {
            BOOST_TEST_MESSAGE(" +" << all_implementations[i]);
        }
    }
    BOOST_TEST_MESSAGE("The following implementations are NOT compiled in and will not be tested:");
    for (int i = 0; i < int(sizeof(all_implementations)/sizeof(all_implementations[0])); ++i) {
        if (impls.find(all_implementations[i]) == impls.end()) {
            BOOST_TEST_MESSAGE(" -" << all_implementations[i]);
        }
    }
}


/*
 * 1a. Simple synthetic signals, transforms to separate real/imag arrays,
 *     double-precision
 */

ALL_IMPL_AUTO_TEST_CASE(dc)
{
    // DC-only signal. The DC bin is purely real
    double in[] = { 1, 1, 1, 1 };
    double re[3], im[3];
    USING_FFT(4);
    fft.forward(in, re, im);
    COMPARE(re[0], 4.0);
    COMPARE_ZERO(re[1]);
    COMPARE_ZERO(re[2]);
    COMPARE_ALL(im, 0.0);
    double back[4];
    fft.inverse(re, im, back);
    COMPARE_SCALED(back, in, 4);
}

ALL_IMPL_AUTO_TEST_CASE(sine)
{
    // Sine. Output is purely imaginary
    double in[] = { 0, 1, 0, -1 };
    double re[3], im[3];
    USING_FFT(4);
    fft.forward(in, re, im);
    COMPARE_ALL(re, 0.0);
    COMPARE_ZERO(im[0]);
    COMPARE(im[1], -2.0);
    COMPARE_ZERO(im[2]);
    double back[4];
    fft.inverse(re, im, back);
    COMPARE_SCALED(back, in, 4);
}

ALL_IMPL_AUTO_TEST_CASE(sine_8)
{
    // Longer sine. With only 4 elements, the real transform only
    // needs to get the DC and Nyquist bins right for its two complex
    // sub-transforms. We need a longer test to check the real
    // transform is working properly.
    double cospi4 = 0.5 * sqrt(2.0);
    double in[] = { 0, cospi4, 1.0, cospi4, 0.0, -cospi4, -1.0, -cospi4 };
    double re[5], im[5];
    USING_FFT(8);
    fft.forward(in, re, im);
    COMPARE_ALL(re, 0.0);
    COMPARE_ZERO(im[0]);
    COMPARE(im[1], -4.0);
    COMPARE_ZERO(im[2]);
    COMPARE_ZERO(im[3]);
    COMPARE_ZERO(im[4]);
    double back[8];
    fft.inverse(re, im, back);
    COMPARE_SCALED(back, in, 8);
}

ALL_IMPL_AUTO_TEST_CASE(cosine)
{
    // Cosine. Output is purely real
    double in[] = { 1, 0, -1, 0 };
    double re[3], im[3];
    USING_FFT(4);
    fft.forward(in, re, im);
    COMPARE_ZERO(re[0]);
    COMPARE(re[1], 2.0);
    COMPARE_ZERO(re[2]);
    COMPARE_ALL(im, 0.0);
    double back[4];
    fft.inverse(re, im, back);
    COMPARE_SCALED(back, in, 4);
}

ALL_IMPL_AUTO_TEST_CASE(cosine_8)
{
    // Longer cosine.
    double cospi4 = 0.5 * sqrt(2.0);
    double in[] = { 1.0, cospi4, 0.0, -cospi4, -1.0, -cospi4, 0.0, cospi4 };
    double re[5], im[5];
    USING_FFT(8);
    fft.forward(in, re, im);
    COMPARE_ALL(im, 0.0);
    COMPARE_ZERO(re[0]);
    COMPARE(re[1], 4.0);
    COMPARE_ZERO(re[2]);
    COMPARE_ZERO(re[3]);
    COMPARE_ZERO(re[4]);
    double back[8];
    fft.inverse(re, im, back);
    COMPARE_SCALED(back, in, 8);
}
	
ALL_IMPL_AUTO_TEST_CASE(sineCosine)
{
    // Sine and cosine mixed
    double in[] = { 0.5, 1, -0.5, -1 };
    double re[3], im[3];
    USING_FFT(4);
    fft.forward(in, re, im);
    COMPARE_ZERO(re[0]);
    COMPARE(re[1], 1.0);
    COMPARE_ZERO(re[2]);
    COMPARE_ZERO(im[0]);
    COMPARE(im[1], -2.0);
    COMPARE_ZERO(im[2]);
    double back[4];
    fft.inverse(re, im, back);
    COMPARE_SCALED(back, in, 4);
}

ALL_IMPL_AUTO_TEST_CASE(nyquist)
{
    double in[] = { 1, -1, 1, -1 };
    double re[3], im[3];
    USING_FFT(4);
    fft.forward(in, re, im);
    COMPARE_ZERO(re[0]);
    COMPARE_ZERO(re[1]);
    COMPARE(re[2], 4.0);
    COMPARE_ALL(im, 0.0);
    double back[4];
    fft.inverse(re, im, back);
    COMPARE_SCALED(back, in, 4);
}

ALL_IMPL_AUTO_TEST_CASE(dirac)
{
    double in[] = { 1, 0, 0, 0 };
    double re[3], im[3];
    USING_FFT(4);
    fft.forward(in, re, im);
    COMPARE(re[0], 1.0);
    COMPARE(re[1], 1.0);
    COMPARE(re[2], 1.0);
    COMPARE_ALL(im, 0.0);
    double back[4];
    fft.inverse(re, im, back);
    COMPARE_SCALED(back, in, 4);
}


/*
 * 1b. Simple synthetic signals, transforms to separate real/imag arrays,
 *     single-precision (i.e. single-precision version of 1a)
 */

ALL_IMPL_AUTO_TEST_CASE(dcF)
{
    // DC-only signal. The DC bin is purely real
    float in[] = { 1, 1, 1, 1 };
    float re[3], im[3];
    USING_FFT(4);
    fft.forward(in, re, im);
    COMPARE_F(re[0], 4.0f);
    COMPARE_ZERO_F(re[1]);
    COMPARE_ZERO_F(re[2]);
    COMPARE_ALL_F(im, 0.0f);
    float back[4];
    fft.inverse(re, im, back);
    COMPARE_SCALED_F(back, in, 4);
}

ALL_IMPL_AUTO_TEST_CASE(sineF)
{
    // Sine. Output is purely imaginary
    float in[] = { 0, 1, 0, -1 };
    float re[3], im[3];
    USING_FFT(4);
    fft.forward(in, re, im);
    COMPARE_ALL_F(re, 0.0f);
    COMPARE_ZERO_F(im[0]);
    COMPARE_F(im[1], -2.0f);
    COMPARE_ZERO_F(im[2]);
    float back[4];
    fft.inverse(re, im, back);
    COMPARE_SCALED_F(back, in, 4);
}

ALL_IMPL_AUTO_TEST_CASE(cosineF)
{
    // Cosine. Output is purely real
    float in[] = { 1, 0, -1, 0 };
    float re[3], im[3];
    USING_FFT(4);
    fft.forward(in, re, im);
    COMPARE_ZERO_F(re[0]);
    COMPARE_F(re[1], 2.0f);
    COMPARE_ZERO_F(re[2]);
    COMPARE_ALL_F(im, 0.0f);
    float back[4];
    fft.inverse(re, im, back);
    COMPARE_SCALED_F(back, in, 4);
}
	
ALL_IMPL_AUTO_TEST_CASE(sineCosineF)
{
    // Sine and cosine mixed
    float in[] = { 0.5, 1, -0.5, -1 };
    float re[3], im[3];
    USING_FFT(4);
    fft.forward(in, re, im);
    COMPARE_ZERO_F(re[0]);
    COMPARE_F(re[1], 1.0f);
    COMPARE_ZERO_F(re[2]);
    COMPARE_ZERO_F(im[0]);
    COMPARE_F(im[1], -2.0f);
    COMPARE_ZERO_F(im[2]);
    float back[4];
    fft.inverse(re, im, back);
    COMPARE_SCALED_F(back, in, 4);
}

ALL_IMPL_AUTO_TEST_CASE(nyquistF)
{
    float in[] = { 1, -1, 1, -1 };
    float re[3], im[3];
    USING_FFT(4);
    fft.forward(in, re, im);
    COMPARE_ZERO_F(re[0]);
    COMPARE_ZERO_F(re[1]);
    COMPARE_F(re[2], 4.0f);
    COMPARE_ALL_F(im, 0.0f);
    float back[4];
    fft.inverse(re, im, back);
    COMPARE_SCALED_F(back, in, 4);
}

ALL_IMPL_AUTO_TEST_CASE(diracF)
{
    float in[] = { 1, 0, 0, 0 };
    float re[3], im[3];
    USING_FFT(4);
    fft.forward(in, re, im);
    COMPARE_F(re[0], 1.0f);
    COMPARE_F(re[1], 1.0f);
    COMPARE_F(re[2], 1.0f);
    COMPARE_ALL_F(im, 0.0f);
    float back[4];
    fft.inverse(re, im, back);
    COMPARE_SCALED_F(back, in, 4);
}


/*
 * 2a. Subset of synthetic signals, testing different output formats
 *     (interleaved complex, polar, magnitude-only, and our weird
 *     cepstral thing), double-precision
 */ 

ALL_IMPL_AUTO_TEST_CASE(interleaved)
{
    // Sine and cosine mixed, test output format
    double in[] = { 0.5, 1, -0.5, -1 };
    double out[6];
    USING_FFT(4);
    fft.forwardInterleaved(in, out);
    COMPARE_ZERO(out[0]);
    COMPARE_ZERO(out[1]);
    COMPARE(out[2], 1.0);
    COMPARE(out[3], -2.0);
    COMPARE_ZERO(out[4]);
    COMPARE_ZERO(out[5]);
    double back[4];
    fft.inverseInterleaved(out, back);
    COMPARE_SCALED(back, in, 4);
}

ALL_IMPL_AUTO_TEST_CASE(sinePolar)
{
    double in[] = { 0, 1, 0, -1 };
    double mag[3], phase[3];
    USING_FFT(4);
    fft.forwardPolar(in, mag, phase);
    COMPARE_ZERO(mag[0]);
    COMPARE(mag[1], 2.0);
    COMPARE_ZERO(mag[2]);
    // No meaningful tests for phase[i] where mag[i]==0 (phase
    // could legitimately be anything)
    COMPARE(phase[1], -M_PI/2.0);
    double back[4];
    fft.inversePolar(mag, phase, back);
    COMPARE_SCALED(back, in, 4);
}

ALL_IMPL_AUTO_TEST_CASE(cosinePolar)
{
    double in[] = { 1, 0, -1, 0 };
    double mag[3], phase[3];
    USING_FFT(4);
    fft.forwardPolar(in, mag, phase);
    COMPARE_ZERO(mag[0]);
    COMPARE(mag[1], 2.0);
    COMPARE_ZERO(mag[2]);
    // No meaningful tests for phase[i] where mag[i]==0 (phase
    // could legitimately be anything)
    COMPARE_ZERO(phase[1]);
    double back[4];
    fft.inversePolar(mag, phase, back);
    COMPARE_SCALED(back, in, 4);
}

ALL_IMPL_AUTO_TEST_CASE(magnitude)
{
    // Sine and cosine mixed
    double in[] = { 0.5, 1, -0.5, -1 };
    double out[3];
    USING_FFT(4);
    fft.forwardMagnitude(in, out);
    COMPARE_ZERO(out[0]);
    COMPARE_F(float(out[1]), sqrtf(5.0));
    COMPARE_ZERO(out[2]);
}

ALL_IMPL_AUTO_TEST_CASE(cepstrum)
{
    double in[] = { 1, 0, 0, 0, 1, 0, 0, 0 };
    double mag[5];
    USING_FFT(8);
    fft.forwardMagnitude(in, mag);
    double cep[8];
    fft.inverseCepstral(mag, cep);
    BOOST_CHECK_SMALL(cep[1], 1e-9);
    BOOST_CHECK_SMALL(cep[2], 1e-9);
    BOOST_CHECK_SMALL(cep[3], 1e-9);
    BOOST_CHECK_SMALL(cep[5], 1e-9);
    BOOST_CHECK_SMALL(cep[6], 1e-9);
    BOOST_CHECK_SMALL(cep[7], 1e-9);
    BOOST_CHECK_SMALL(-6.561181 - cep[0]/8, 0.000001);
    BOOST_CHECK_SMALL( 7.254329 - cep[4]/8, 0.000001);
}


/*
 * 2b. Subset of synthetic signals, testing different output formats
 *     (interleaved complex, polar, magnitude-only, and our weird
 *     cepstral thing), single-precision (i.e. single-precision
 *     version of 2a)
 */ 
	
ALL_IMPL_AUTO_TEST_CASE(interleavedF)
{
    // Sine and cosine mixed, test output format
    float in[] = { 0.5, 1, -0.5, -1 };
    float out[6];
    USING_FFT(4);
    fft.forwardInterleaved(in, out);
    COMPARE_ZERO_F(out[0]);
    COMPARE_ZERO_F(out[1]);
    COMPARE_F(out[2], 1.0f);
    COMPARE_F(out[3], -2.0f);
    COMPARE_ZERO_F(out[4]);
    COMPARE_ZERO_F(out[5]);
    float back[4];
    fft.inverseInterleaved(out, back);
    COMPARE_SCALED_F(back, in, 4);
}

ALL_IMPL_AUTO_TEST_CASE(cosinePolarF)
{
    float in[] = { 1, 0, -1, 0 };
    float mag[3], phase[3];
    USING_FFT(4);
    fft.forwardPolar(in, mag, phase);
    COMPARE_ZERO_F(mag[0]);
    COMPARE_F(mag[1], 2.0f);
    COMPARE_ZERO_F(mag[2]);
    // No meaningful tests for phase[i] where mag[i]==0 (phase
    // could legitimately be anything)
    COMPARE_ZERO_F(phase[1]);
    float back[4];
    fft.inversePolar(mag, phase, back);
    COMPARE_SCALED_F(back, in, 4);
}

ALL_IMPL_AUTO_TEST_CASE(sinePolarF)
{
    float in[] = { 0, 1, 0, -1 };
    float mag[3], phase[3];
    USING_FFT(4);
    fft.forwardPolar(in, mag, phase);
    COMPARE_ZERO_F(mag[0]);
    COMPARE_F(mag[1], 2.0f);
    COMPARE_ZERO_F(mag[2]);
    // No meaningful tests for phase[i] where mag[i]==0 (phase
    // could legitimately be anything)
    COMPARE_F(phase[1], -float(M_PI)/2.0f);
    float back[4];
    fft.inversePolar(mag, phase, back);
    COMPARE_SCALED_F(back, in, 4);
}

ALL_IMPL_AUTO_TEST_CASE(magnitudeF)
{
    // Sine and cosine mixed
    float in[] = { 0.5, 1, -0.5, -1 };
    float out[3];
    USING_FFT(4);
    fft.forwardMagnitude(in, out);
    COMPARE_ZERO_F(out[0]);
    COMPARE_F(float(out[1]), sqrtf(5.0f));
    COMPARE_ZERO_F(out[2]);
}

ALL_IMPL_AUTO_TEST_CASE(cepstrumF)
{
    float in[] = { 1, 0, 0, 0, 1, 0, 0, 0 };
    float mag[5];
    USING_FFT(8);
    fft.forwardMagnitude(in, mag);
    float cep[8];
    fft.inverseCepstral(mag, cep);
    COMPARE_ZERO_F(cep[1]);
    COMPARE_ZERO_F(cep[2]);
    COMPARE_ZERO_F(cep[3]);
    COMPARE_ZERO_F(cep[5]);
    COMPARE_ZERO_F(cep[6]);
    COMPARE_ZERO_F(cep[7]);
    BOOST_CHECK_SMALL(-6.561181 - cep[0]/8, 0.000001);
    BOOST_CHECK_SMALL( 7.254329 - cep[4]/8, 0.000001);
}


/*
 * 4. Bounds checking, double-precision and single-precision
 */

ALL_IMPL_AUTO_TEST_CASE(forwardArrayBounds)
{
    double in[] = { 1, 1, -1, -1 };

    // Initialise output bins to something recognisable, so we can
    // tell if they haven't been written
    double re[] = { 999, 999, 999, 999, 999 };
    double im[] = { 999, 999, 999, 999, 999 };
    
    USING_FFT(4);
    fft.forward(in, re+1, im+1);
    
    // Check we haven't overrun the output arrays
    COMPARE(re[0], 999.0);
    COMPARE(im[0], 999.0);
    COMPARE(re[4], 999.0);
    COMPARE(im[4], 999.0);
}

ALL_IMPL_AUTO_TEST_CASE(inverseArrayBounds)
{
    // The inverse transform is only supposed to refer to the first
    // N/2+1 bins and synthesise the rest rather than read them - so
    // initialise the next one to some value that would mess up the
    // results if it were used
    double re[] = { 0, 1, 0, 456 };
    double im[] = { 0, -2, 0, 456 };

    // Initialise output bins to something recognisable, so we can
    // tell if they haven't been written
    double out[] = { 999, 999, 999, 999, 999, 999 };
    
    USING_FFT(4);
    fft.inverse(re, im, out+1);

    // Check we haven't overrun the output arrays
    COMPARE(out[0], 999.0);
    COMPARE(out[5], 999.0);

    // And check the results are as we expect, i.e. that we haven't
    // used the bogus final bin
    COMPARE(out[1] / 4, 0.5);
    COMPARE(out[2] / 4, 1.0);
    COMPARE(out[3] / 4, -0.5);
    COMPARE(out[4] / 4, -1.0);
}

ALL_IMPL_AUTO_TEST_CASE(forwardArrayBoundsF)
{
    float in[] = { 1, 1, -1, -1 };
    
    // Initialise output bins to something recognisable, so we can
    // tell if they haven't been written
    float re[] = { 999, 999, 999, 999, 999 };
    float im[] = { 999, 999, 999, 999, 999 };
    
    USING_FFT(4);
    fft.forward(in, re+1, im+1);
    
    // Check we haven't overrun the output arrays
    COMPARE_F(re[0], 999.0f);
    COMPARE_F(im[0], 999.0f);
    COMPARE_F(re[4], 999.0f);
    COMPARE_F(im[4], 999.0f);
}

ALL_IMPL_AUTO_TEST_CASE(inverseArrayBoundsF)
{
    // The inverse transform is only supposed to refer to the first
    // N/2+1 bins and synthesise the rest rather than read them - so
    // initialise the next one to some value that would mess up the
    // results if it were used
    float re[] = { 0, 1, 0, 456 };
    float im[] = { 0, -2, 0, 456 };

    // Initialise output bins to something recognisable, so we can
    // tell if they haven't been written
    float out[] = { 999, 999, 999, 999, 999, 999 };
    
    USING_FFT(4);
    fft.inverse(re, im, out+1);
    
    // Check we haven't overrun the output arrays
    COMPARE_F(out[0], 999.0f);
    COMPARE_F(out[5], 999.0f);

    // And check the results are as we expect, i.e. that we haven't
    // used the bogus final bin
    COMPARE_F(out[1] / 4.0f, 0.5f);
    COMPARE_F(out[2] / 4.0f, 1.0f);
    COMPARE_F(out[3] / 4.0f, -0.5f);
    COMPARE_F(out[4] / 4.0f, -1.0f);
}


/*
 * 6. Slightly longer transforms of pseudorandom data.
 */

ALL_IMPL_AUTO_TEST_CASE(random_precalc_16)
{
    double in[] = {
        -0.24392125308057722, 0.03443898163344272, 0.3448145656738877,
        -0.9625837464603908, 3.366568317669671, 0.9947191221586653,
        -1.5038984435999945, 1.3859898682581235, -1.1230576306688778,
        -1.6757487116512024, -1.5874436867863229, -2.0794018781307155,
        -0.5450152775818973, 0.7530907176983748, 1.0743170685904255,
        3.1787609811018775
    };
    double expected_re[] = {
        1.41162899482, 7.63975551593, -1.20622641052, -1.77829578443,
        3.12678465246, -2.84220463109, -7.17083743716, 0.497290409945,
        -1.84690167439,
    };
    double expected_im[] = {
        0.0, -4.67826048083, 8.58829211964, 4.96449646815,
        1.41626511493, -3.77219223978, 6.96219662744, 2.23138519225,
        0.0,
    };
    double re[9], im[9];
    USING_FFT(16);
    if (eps < 1e-11) {
        eps = 1e-11;
    }
    fft.forward(in, re, im);
    COMPARE_ARR(re, expected_re, 9);
    COMPARE_ARR(im, expected_im, 9);
    double back[16];
    fft.inverse(re, im, back);
    COMPARE_SCALED(back, in, 16);
}

/* This one has data from a PRNG, with a fixed seed. Must pass two
 * tests: (i) same as DFT; (ii) inverse produces original input (after
 * scaling) */
ALL_IMPL_AUTO_TEST_CASE(random)
{
    const int n = 64;
    double *in = new double[n];
    double *re = new double[n/2 + 1];
    double *im = new double[n/2 + 1];
    double *re_compare = new double[n/2 + 1];
    double *im_compare = new double[n/2 + 1];
    double *back = new double[n];
    srand(0);
    for (int i = 0; i < n; ++i) {
        in[i] = (double(rand()) / double(RAND_MAX)) * 4.0 - 2.0;
    }
    USING_FFT(n);
    if (eps < 1e-11) {
        eps = 1e-11;
    }
    fft.forward(in, re, im);
    fft.inverse(re, im, back);
    FFT::setDefaultImplementation("dft");
    fft.forward(in, re_compare, im_compare);
    COMPARE_ARR(re, re_compare, n/2 + 1);
    COMPARE_ARR(im, im_compare, n/2 + 1);
    COMPARE_SCALED_N(back, in, n, n);
    delete[] back;
    delete[] im_compare;
    delete[] re_compare;
    delete[] im;
    delete[] re;
    delete[] in;
}

BOOST_AUTO_TEST_SUITE_END()
