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

#include "../common/Resampler.h"

#include <stdexcept>
#include <vector>
#include <cmath>
#include <iostream>

using namespace RubberBand;

using std::cerr;
using std::endl;

using std::vector;

BOOST_AUTO_TEST_SUITE(TestResampler)

#define LEN(a) (int(sizeof(a)/sizeof(a[0])))

static vector<float>
sine(double samplerate, double frequency, int nsamples)
{
    vector<float> v(nsamples, 0.f);
    for (int i = 0; i < nsamples; ++i) {
        v[i] = sin ((i * 2.0 * M_PI * frequency) / samplerate);
    }
    return v;
}

#define COMPARE_N(a, b, n)                    \
    for (int cmp_i = 0; cmp_i < n; ++cmp_i) { \
        BOOST_CHECK_SMALL((a)[cmp_i] - (b)[cmp_i], 1e-4f);      \
    }

static const float guard_value = -999.f;

BOOST_AUTO_TEST_CASE(interpolated_sine_1ch_interleaved)
{
    // Interpolating a sinusoid should give us a sinusoid, once we've
    // dropped the first few samples
    vector<float> in = sine(8, 2, 1000); // 2Hz wave at 8Hz: [ 0, 1, 0, -1 ] etc
    vector<float> expected = sine(16, 2, 2000);
    vector<float> out(in.size() * 2 + 1, guard_value);
    Resampler r(Resampler::Parameters(), 1);
    int returned = r.resampleInterleaved
        (out.data(), out.size(), in.data(), in.size(), 2, true);

    // because final was true, we expect to have exactly the right
    // number of samples back
    BOOST_CHECK_EQUAL(returned, int(in.size() * 2));
    BOOST_CHECK_NE(out[returned-1], guard_value);
    BOOST_CHECK_EQUAL(out[returned], guard_value);

    // and they should match the expected data, at least in the middle
    const float *outf = out.data() + 200, *expectedf = expected.data() + 200;
    COMPARE_N(outf, expectedf, 600);
}

BOOST_AUTO_TEST_CASE(interpolated_sine_1ch_noninterleaved)
{
    // Interpolating a sinusoid should give us a sinusoid, once we've
    // dropped the first few samples
    vector<float> in = sine(8, 2, 1000); // 2Hz wave at 8Hz: [ 0, 1, 0, -1 ] etc
    vector<float> expected = sine(16, 2, 2000);
    vector<float> out(in.size() * 2 + 1, guard_value);
    const float *in_data = in.data();
    float *out_data = out.data();
    Resampler r(Resampler::Parameters(), 1);
    int returned = r.resample
        (&out_data, out.size(), &in_data, in.size(), 2, true);

    // because final was true, we expect to have exactly the right
    // number of samples back
    BOOST_CHECK_EQUAL(returned, int(in.size() * 2));
    BOOST_CHECK_NE(out[returned-1], guard_value);
    BOOST_CHECK_EQUAL(out[returned], guard_value);

    // and they should match the expected data, at least in the middle
    const float *outf = out.data() + 200, *expectedf = expected.data() + 200;
    COMPARE_N(outf, expectedf, 600);
}

BOOST_AUTO_TEST_CASE(overrun_interleaved)
{
    // Check that the outcount argument is correctly used: any samples
    // already in the out buffer beyond outcount*channels must be left
    // untouched. We test this with short buffers (likely to be
    // shorter than the resampler filter length) and longer ones, with
    // resampler ratios that reduce, leave unchanged, and raise the
    // sample rate, and with all quality settings.

    // Options are ordered from most normal/likely to least.
    
    int channels = 2;

    int lengths[] = { 6000, 6 };
    int constructionBufferSizes[] = { 0, 1000 };
    double ratios[] = { 1.0, 10.0, 0.1 };
    Resampler::Quality qualities[] = {
        Resampler::FastestTolerable, Resampler::Best, Resampler::Fastest
    };

    bool failed = false;
    
    for (int li = 0; li < LEN(lengths); ++li) {
        for (int cbi = 0; cbi < LEN(constructionBufferSizes); ++cbi) {
            for (int ri = 0; ri < LEN(ratios); ++ri) {
                for (int qi = 0; qi < LEN(qualities); ++qi) {
                    
                    int length = lengths[li];
                    double ratio = ratios[ri];
                    Resampler::Parameters parameters;
                    parameters.quality = qualities[qi];
                    parameters.maxBufferSize = constructionBufferSizes[cbi];
                    Resampler r(parameters, channels);

                    float *inbuf = new float[length * channels];
                    for (int i = 0; i < length; ++i) {
                        for (int c = 0; c < channels; ++c) {
                            inbuf[i*channels+c] =
                                sinf((i * 2.0 * M_PI * 440.0) / 44100.0);
                        }
                    }

                    int outcount = int(round(length * ratio));
                    outcount -= 10;
                    if (outcount < 1) outcount = 1;
                    int outbuflen = outcount + 10;
                    float *outbuf = new float[outbuflen * channels];
                    for (int i = 0; i < outbuflen; ++i) {
                        for (int c = 0; c < channels; ++c) {
                            outbuf[i*channels+c] = guard_value;
                        }
                    }
/*
                    cerr << "\nTesting with length = " << length << ", ratio = "
                         << ratio << ", outcount = " << outcount << ", final = false"
                         << endl;
*/                    
                    int returned = r.resampleInterleaved
                        (outbuf, outcount, inbuf, length, ratio, false);

                    BOOST_CHECK_LE(returned, outcount);
                    
                    for (int i = returned; i < outbuflen; ++i) {
                        for (int c = 0; c < channels; ++c) {
                            BOOST_CHECK_EQUAL(outbuf[i*channels+c], guard_value);
                            if (outbuf[i*channels+c] != guard_value) {
                                failed = true;
                            }
                        }
                    }

                    if (failed) {
                        cerr << "Test failed, abandoning remaining loop cycles"
                             << endl;
                        break;
                    }
/*
                    cerr << "\nContinuing with length = " << length << ", ratio = "
                         << ratio << ", outcount = " << outcount << ", final = true"
                         << endl;
*/
                    returned = r.resampleInterleaved
                        (outbuf, outcount, inbuf, length, ratio, true);

                    BOOST_CHECK_LE(returned, outcount);

                    for (int i = returned; i < outbuflen; ++i) {
                        for (int c = 0; c < channels; ++c) {
                            BOOST_CHECK_EQUAL(outbuf[i*channels+c], guard_value);
                            if (outbuf[i*channels+c] != guard_value) {
                                failed = true;
                            }
                        }
                    }

                    delete[] outbuf;
                    delete[] inbuf;

                    if (failed) {
                        cerr << "Test failed, abandoning remaining loop cycles"
                             << endl;
                        break;
                    }
                }

                if (failed) break;
            }
            if (failed) break;
        }
        if (failed) break;
    }
}

BOOST_AUTO_TEST_SUITE_END()

