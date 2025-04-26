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

#include "../../rubberband/RubberBandStretcher.h"

#include <iostream>

#include <cmath>

using namespace RubberBand;

using std::vector;
using std::cerr;
using std::endl;

namespace tt = boost::test_tools;

BOOST_AUTO_TEST_SUITE(TestStretcher)

BOOST_AUTO_TEST_CASE(engine_version)
{
    RubberBandStretcher s2(44100, 1, RubberBandStretcher::OptionEngineFaster);
    BOOST_TEST(s2.getEngineVersion() == 2);
    BOOST_TEST(s2.getProcessSizeLimit() == 524288u);
    RubberBandStretcher s3(44100, 1, RubberBandStretcher::OptionEngineFiner);
    BOOST_TEST(s3.getEngineVersion() == 3);
    BOOST_TEST(s3.getProcessSizeLimit() == 524288u);
}

BOOST_AUTO_TEST_CASE(sinusoid_unchanged_offline_faster)
{
    int n = 10000;
    float freq = 440.f;
    int rate = 44100;
    RubberBandStretcher stretcher
        (rate, 1, RubberBandStretcher::OptionEngineFaster);

    vector<float> in(n), out(n);
    for (int i = 0; i < n; ++i) {
        in[i] = sinf(float(i) * freq * M_PI * 2.f / float(rate));
    }
    float *inp = in.data(), *outp = out.data();

    stretcher.setMaxProcessSize(n);
    stretcher.setExpectedInputDuration(n);
    BOOST_TEST(stretcher.available() == 0);

    stretcher.study(&inp, n, true);
    BOOST_TEST(stretcher.available() == 0);

    stretcher.process(&inp, n, true);
    BOOST_TEST(stretcher.available() == n);

    BOOST_TEST(stretcher.getStartDelay() == 0u); // offline mode
    
    size_t got = stretcher.retrieve(&outp, n);
    BOOST_TEST(got == size_t(n));
    BOOST_TEST(stretcher.available() == -1);

    // We now have n samples of a simple sinusoid with stretch factor
    // 1.0; obviously we expect the output to be essentially the same
    // thing. It will have lower precision for a while at the start
    // and end because of windowing factors, so we check those with a
    // threshold of 0.1; in the middle we expect better
    // precision. Note that these are relative tolerances, not
    // absolute, i.e. 0.001 means 0.001x the smaller value - so they
    // are tighter than they appear.

    // This syntax for comparing containers with a certain tolerance
    // using BOOST_TEST is just bonkers. I can't find the << syntax to
    // combine manipulators documented anywhere other than in a
    // release note, but it does work. Well, sort of - it works this
    // way around but not as per_element << tolerance. And
    // tolerance(0.1) doesn't do what you'd expect if the things
    // you're comparing are floats (it sets the tolerance for doubles,
    // leaving float comparison unchanged). Clever... too clever.
    
    BOOST_TEST(out == in,
               tt::tolerance(0.1f) << tt::per_element());
    
    BOOST_TEST(vector<float>(out.begin() + 1024, out.begin() + n - 1024) ==
               vector<float>(in.begin() + 1024, in.begin() + n - 1024),
               tt::tolerance(0.001f) << tt::per_element());
}

BOOST_AUTO_TEST_CASE(sinusoid_unchanged_offline_finer)
{
    int n = 10000;
    float freq = 440.f;
    int rate = 44100;

    RubberBandStretcher stretcher
        (rate, 1, RubberBandStretcher::OptionEngineFiner);
    
    vector<float> in(n), out(n);
    for (int i = 0; i < n; ++i) {
        in[i] = sinf(float(i) * freq * M_PI * 2.f / float(rate));
    }
    float *inp = in.data(), *outp = out.data();

    stretcher.setMaxProcessSize(n);
    stretcher.setExpectedInputDuration(n);
    BOOST_TEST(stretcher.available() == 0);

    stretcher.study(&inp, n, true);
    BOOST_TEST(stretcher.available() == 0);

    stretcher.process(&inp, n, true);
    BOOST_TEST(stretcher.available() == n);

    BOOST_TEST(stretcher.getStartDelay() == 0u); // offline mode
    
    size_t got = stretcher.retrieve(&outp, n);
    BOOST_TEST(got == size_t(n));
    BOOST_TEST(stretcher.available() == -1);

    // The R3 engine is actually less precise than R2 here because of
    // its different windowing design, though see the note above about
    // what these tolerances mean
    
    BOOST_TEST(out == in,
               tt::tolerance(0.35f) << tt::per_element());
    
    BOOST_TEST(vector<float>(out.begin() + 1024, out.begin() + n - 1024) ==
               vector<float>(in.begin() + 1024, in.begin() + n - 1024),
               tt::tolerance(0.01f) << tt::per_element());

//    std::cout << "ms\tV" << std::endl;
//    for (int i = 0; i < n; ++i) {
//        std::cout << i << "\t" << out[i] - in[i] << std::endl;
//    }
}

BOOST_AUTO_TEST_CASE(sinusoid_2x_offline_finer)
{
    int n = 10000;
    float freq = 441.f; // so a cycle is an exact number of samples
    int rate = 44100;

    RubberBandStretcher stretcher
        (rate, 1, RubberBandStretcher::OptionEngineFiner);

    stretcher.setTimeRatio(2.0);
    
    vector<float> in(n), out(n*2);
    for (int i = 0; i < n*2; ++i) {
        out[i] = sinf(float(i) * freq * M_PI * 2.f / float(rate));
        if (i < n) {
            in[i] = out[i];
        }
    }
    float *inp = in.data(), *outp = out.data();

    stretcher.setMaxProcessSize(n);
    stretcher.setExpectedInputDuration(n);
    BOOST_TEST(stretcher.available() == 0);

    stretcher.study(&inp, n, true);
    BOOST_TEST(stretcher.available() == 0);

    stretcher.process(&inp, n, true);
    BOOST_TEST(stretcher.available() == n*2);

    BOOST_TEST(stretcher.getStartDelay() == 0u); // offline mode
    
    size_t got = stretcher.retrieve(&outp, n*2);
    BOOST_TEST(got == size_t(n)*2);
    BOOST_TEST(stretcher.available() == -1);

    int period = -1;
    for (int i = 1000; i < 2000; ++i) {
        if (period >= 0) ++period;
        if (out[i] <= 0.f && out[i+1] > 0.f) {
            if (period == -1) period = 0;
            else break;
        }
    }
    BOOST_TEST(period == 100);
    
    int offset = 0;
    for (int i = 0; i < 200; ++i) {
        if (out[i] <= 0.f && out[i+1] > -0.01f) {
            offset = i + 1;
            break;
        }
    }

    // overall
    
    double rms = 0.0;
    for (int i = 0; i < n - offset; ++i) {
        double diff = out[i + offset] - in[i];
        rms += diff * diff;
    }
    rms = sqrt(rms / double(n - offset));
    BOOST_TEST(rms < 0.2);

    // steady state
    
    rms = 0.0;
    for (int i = 1500; i < n - offset - 3000; ++i) {
        double diff = out[i + offset + 1500] - in[i + 1500];
        rms += diff * diff;
    }
    rms = sqrt(rms / double(n - offset - 3000));
    BOOST_TEST(rms < 0.1);
}

static vector<float> process_realtime(RubberBandStretcher &stretcher,
                                      const vector<float> &in,
                                      int nOut,
                                      int bs,
                                      bool roundUpProcessSize,
                                      bool printDebug)
{
    int n = in.size();
    vector<float> out(nOut, 0.f);

    // Prime the start
    {
        float *source = out.data(); // just reuse out because it's silent
        stretcher.process(&source, stretcher.getPreferredStartPad(), false);
    }

    int toSkip = stretcher.getStartDelay();
    
    int inOffset = 0, outOffset = 0;

    while (outOffset < nOut) {

        // Obtain a single block of size bs, simulating realtime
        // playback. The following might be the content of a
        // sound-producing callback function

        int needed = std::min(bs, nOut - outOffset);
        int obtained = 0;

        while (obtained < needed) {

            int available = stretcher.available();

            if (available < 0) { // finished
                for (int i = obtained; i < needed; ++i) {
                    out[outOffset++] = 0.f;
                }
                break;

            } else if (available == 0) { // need to provide more input
                int required = stretcher.getSamplesRequired();
                BOOST_TEST(required > 0); // because available == 0
                int toProcess = required;
                if (roundUpProcessSize) {
                    // We sometimes want to explicitly test passing
                    // large blocks to process, longer than
                    // getSamplesRequired indicates
                    toProcess = std::max(required, bs);
                }
                bool final = false;
                if (toProcess >= n - inOffset) {
                    toProcess = n - inOffset;
                    final = true;
                }
                const float *const source = in.data() + inOffset;
//                cerr << "toProcess = " << toProcess << ", inOffset = " << inOffset << ", n = " << n << ", required = " << required << ", outOffset = " << outOffset << ", obtained = " << obtained << ", bs = " << bs << ", final = " << final << endl;
                stretcher.process(&source, toProcess, final);
                inOffset += toProcess;
                BOOST_TEST(stretcher.available() > 0);
                continue;

            } else if (toSkip > 0) { // available > 0 && toSkip > 0
                float *target = out.data() + outOffset;
                int toRetrieve = std::min(toSkip, available);
                int retrieved = stretcher.retrieve(&target, toRetrieve);
                BOOST_TEST(retrieved == toRetrieve);
                toSkip -= retrieved;
                
            } else { // available > 0
                float *target = out.data() + outOffset;
                int toRetrieve = std::min(needed - obtained, available);
                int retrieved = stretcher.retrieve(&target, toRetrieve);
                BOOST_TEST(retrieved == toRetrieve);
                obtained += retrieved;
                outOffset += retrieved;
            }
        }
    }

    if (printDebug) {
        // The initial # is to allow grep on the test output
        std::cout << "#sample\tV" << std::endl;
        for (int i = 0; i < nOut; ++i) {
            std::cout << "#" << i << "\t" << out[i] << std::endl;
        }
    }

    return out;
}

static void sinusoid_realtime(RubberBandStretcher::Options options,
                              double timeRatio,
                              double pitchScale,
                              int bs = 512,
                              bool roundUpProcessSize = false,
                              bool printDebug = false)
{
    int n = (timeRatio < 1.0 ? 80000 : 40000);
    if (n < bs * 2) n = bs * 2;
    int nOut = int(ceil(n * timeRatio));
    float freq = 441.f;
    int rate = 44100;

    // This test simulates block-by-block realtime processing with
    // latency compensation, and checks that the output is all in the
    // expected place

    RubberBandStretcher stretcher(rate, 1, options, timeRatio, pitchScale);

    if (printDebug) {
        stretcher.setDebugLevel(2);
    }

    stretcher.setMaxProcessSize(bs);
    
    // The input signal is a fixed frequency sinusoid that steps up in
    // amplitude every 1/10 of the total duration - from 0.1 at the
    // start, via increments of 0.1, to 1.0 at the end
    
    vector<float> in(n);
    for (int i = 0; i < n; ++i) {
        float amplitude = float((i / (n/10)) + 1) / 10.f;
        float sample = amplitude *
            sinf(float(i) * freq * M_PI * 2.f / float(rate));
        in[i] = sample;
    }

    vector<float> out = process_realtime(stretcher, in, nOut, bs,
                                         roundUpProcessSize, printDebug);
        
    // Step through the output signal in chunk of 1/20 of its duration
    // (i.e. a rather arbitrary two per expected 0.1 increment in
    // amplitude) and for each chunk, verify that the frequency is
    // right and the amplitude is what we expect at that point

    for (int chunk = 0; chunk < 20; ++chunk) {

//        cerr << "chunk " << chunk << " of 20" << endl;
        
        int i0 = (nOut * chunk) / 20;
        int i1 = (nOut * (chunk + 1)) / 20;

        // frequency

        int positiveCrossings = 0;
        for (int i = i0; i + 1 < i1; ++i) {
            if (out[i] <= 0.f && out[i+1] > 0.f) {
                ++positiveCrossings;
            }
        }

        int expectedCrossings = int(round((freq * pitchScale *
                                           double(i1 - i0)) / rate));

        bool highSpeedPitch =
            ! ((options & RubberBandStretcher::OptionPitchHighQuality) ||
               (options & RubberBandStretcher::OptionPitchHighConsistency));

        // The check here has to depend on whether we are in Finer or
        // Faster mode. In Finer mode, we expect to be generally exact
        // but in the first and last chunks we can be out by one
        // crossing if slowing, more if speeding up. In Faster mode we
        // need to cut more slack

        int slack = 0;

        if (options & RubberBandStretcher::OptionEngineFiner) {
            if (chunk == 19) {
                slack = 5;
            } else if (options & RubberBandStretcher::OptionWindowShort) {
                slack = 2;
            } else if (chunk == 0 || highSpeedPitch) {
                slack = 1;
            }
        } else {
            slack = 1;
            if (chunk == 0) {
                slack = (timeRatio < 1.0 ? 3 : 2);
            } else if (chunk == 19) {
                // all bets are off, practically
                slack = expectedCrossings / 2;
            } else {
                slack = 1;
            }
        }

        BOOST_TEST(positiveCrossings <= expectedCrossings + slack);
        BOOST_TEST(positiveCrossings >= expectedCrossings - slack);
        
        // amplitude
        
        double rms = 0.0;
        for (int i = i0; i < i1; ++i) {
            rms += out[i] * out[i];
        }
        rms = sqrt(rms / double(i1 - i0));

        double expected = (chunk/2 + 1) * 0.05 * sqrt(2.0);

        double maxOver = 0.01;
        double maxUnder = 0.1;

        if (!(options & RubberBandStretcher::OptionEngineFiner) ||
            (options & RubberBandStretcher::OptionWindowShort)) {
            maxUnder = 0.2;
        } else if (chunk == 19) {
            maxUnder = 0.15;
        }
        
        BOOST_TEST(rms - expected < maxOver);
        BOOST_TEST(expected - rms < maxUnder);
    }        
}

BOOST_AUTO_TEST_CASE(sinusoid_slow_samepitch_realtime_finer)
{
    sinusoid_realtime(RubberBandStretcher::OptionEngineFiner |
                      RubberBandStretcher::OptionProcessRealTime,
                      8.0, 1.0);
}

BOOST_AUTO_TEST_CASE(sinusoid_fast_samepitch_realtime_finer)
{
    sinusoid_realtime(RubberBandStretcher::OptionEngineFiner |
                      RubberBandStretcher::OptionProcessRealTime,
                      0.5, 1.0);
}

BOOST_AUTO_TEST_CASE(sinusoid_slow_higher_realtime_finer)
{
    sinusoid_realtime(RubberBandStretcher::OptionEngineFiner |
                      RubberBandStretcher::OptionProcessRealTime,
                      4.0, 1.5);
}

BOOST_AUTO_TEST_CASE(sinusoid_slow_higher_realtime_finer_hqpitch)
{
    sinusoid_realtime(RubberBandStretcher::OptionEngineFiner |
                      RubberBandStretcher::OptionProcessRealTime |
                      RubberBandStretcher::OptionPitchHighQuality,
                      4.0, 1.5);
}

BOOST_AUTO_TEST_CASE(sinusoid_slow_higher_realtime_finer_hcpitch)
{
    sinusoid_realtime(RubberBandStretcher::OptionEngineFiner |
                      RubberBandStretcher::OptionProcessRealTime |
                      RubberBandStretcher::OptionPitchHighConsistency,
                      4.0, 1.5);
}

BOOST_AUTO_TEST_CASE(sinusoid_fast_higher_realtime_finer)
{
    sinusoid_realtime(RubberBandStretcher::OptionEngineFiner |
                      RubberBandStretcher::OptionProcessRealTime,
                      0.5, 1.5);
}

BOOST_AUTO_TEST_CASE(sinusoid_fast_higher_realtime_finer_hqpitch)
{
    sinusoid_realtime(RubberBandStretcher::OptionEngineFiner |
                      RubberBandStretcher::OptionProcessRealTime |
                      RubberBandStretcher::OptionPitchHighQuality,
                      0.5, 1.5);
}

BOOST_AUTO_TEST_CASE(sinusoid_fast_higher_realtime_finer_hcpitch)
{
    sinusoid_realtime(RubberBandStretcher::OptionEngineFiner |
                      RubberBandStretcher::OptionProcessRealTime |
                      RubberBandStretcher::OptionPitchHighConsistency,
                      0.5, 1.5);
}

BOOST_AUTO_TEST_CASE(sinusoid_slow_lower_realtime_finer)
{
    sinusoid_realtime(RubberBandStretcher::OptionEngineFiner |
                      RubberBandStretcher::OptionProcessRealTime,
                      8.0, 0.5);
}

BOOST_AUTO_TEST_CASE(sinusoid_slow_lower_realtime_finer_hqpitch)
{
    sinusoid_realtime(RubberBandStretcher::OptionEngineFiner |
                      RubberBandStretcher::OptionProcessRealTime |
                      RubberBandStretcher::OptionPitchHighQuality,
                      8.0, 0.5);
}

BOOST_AUTO_TEST_CASE(sinusoid_slow_lower_realtime_finer_hcpitch)
{
    sinusoid_realtime(RubberBandStretcher::OptionEngineFiner |
                      RubberBandStretcher::OptionProcessRealTime |
                      RubberBandStretcher::OptionPitchHighConsistency,
                      8.0, 0.5);
}

BOOST_AUTO_TEST_CASE(sinusoid_slow_samepitch_realtime_finer_short)
{
    sinusoid_realtime(RubberBandStretcher::OptionEngineFiner |
                      RubberBandStretcher::OptionProcessRealTime |
                      RubberBandStretcher::OptionWindowShort,
                      8.0, 1.0);
}

BOOST_AUTO_TEST_CASE(sinusoid_fast_samepitch_realtime_finer_short)
{
    sinusoid_realtime(RubberBandStretcher::OptionEngineFiner |
                      RubberBandStretcher::OptionProcessRealTime |
                      RubberBandStretcher::OptionWindowShort,
                      0.5, 1.0);
}

BOOST_AUTO_TEST_CASE(sinusoid_slow_higher_realtime_finer_short)
{
    sinusoid_realtime(RubberBandStretcher::OptionEngineFiner |
                      RubberBandStretcher::OptionProcessRealTime |
                      RubberBandStretcher::OptionWindowShort,
                      4.0, 1.5);
}

BOOST_AUTO_TEST_CASE(sinusoid_slow_higher_realtime_finer_short_hcpitch)
{
    sinusoid_realtime(RubberBandStretcher::OptionEngineFiner |
                      RubberBandStretcher::OptionProcessRealTime |
                      RubberBandStretcher::OptionWindowShort |
                      RubberBandStretcher::OptionPitchHighConsistency,
                      4.0, 1.5);
}

BOOST_AUTO_TEST_CASE(sinusoid_fast_higher_realtime_finer_short)
{
    sinusoid_realtime(RubberBandStretcher::OptionEngineFiner |
                      RubberBandStretcher::OptionProcessRealTime |
                      RubberBandStretcher::OptionWindowShort,
                      0.5, 1.5);
}

BOOST_AUTO_TEST_CASE(sinusoid_fast_higher_realtime_finer_hcpitch_short)
{
    sinusoid_realtime(RubberBandStretcher::OptionEngineFiner |
                      RubberBandStretcher::OptionProcessRealTime |
                      RubberBandStretcher::OptionWindowShort |
                      RubberBandStretcher::OptionPitchHighConsistency,
                      0.5, 1.5);
}

BOOST_AUTO_TEST_CASE(sinusoid_slow_lower_realtime_finer_short)
{
    sinusoid_realtime(RubberBandStretcher::OptionEngineFiner |
                      RubberBandStretcher::OptionProcessRealTime |
                      RubberBandStretcher::OptionWindowShort,
                      8.0, 0.5);
}

BOOST_AUTO_TEST_CASE(sinusoid_slow_lower_realtime_finer_short_hcpitch)
{
    sinusoid_realtime(RubberBandStretcher::OptionEngineFiner |
                      RubberBandStretcher::OptionProcessRealTime |
                      RubberBandStretcher::OptionWindowShort |
                      RubberBandStretcher::OptionPitchHighConsistency,
                      8.0, 0.5);
}

BOOST_AUTO_TEST_CASE(sinusoid_slow_samepitch_realtime_faster)
{
    sinusoid_realtime(RubberBandStretcher::OptionEngineFaster |
                      RubberBandStretcher::OptionProcessRealTime,
                      8.0, 1.0);
}

BOOST_AUTO_TEST_CASE(sinusoid_fast_samepitch_realtime_faster)
{
    sinusoid_realtime(RubberBandStretcher::OptionEngineFaster |
                      RubberBandStretcher::OptionProcessRealTime,
                      0.5, 1.0);
}

BOOST_AUTO_TEST_CASE(sinusoid_slow_higher_realtime_faster)
{
    sinusoid_realtime(RubberBandStretcher::OptionEngineFaster |
                      RubberBandStretcher::OptionProcessRealTime,
                      4.0, 1.5);
}

BOOST_AUTO_TEST_CASE(sinusoid_slow_higher_realtime_faster_hqpitch)
{
    sinusoid_realtime(RubberBandStretcher::OptionEngineFaster |
                      RubberBandStretcher::OptionProcessRealTime |
                      RubberBandStretcher::OptionPitchHighQuality,
                      4.0, 1.5);
}

BOOST_AUTO_TEST_CASE(sinusoid_slow_higher_realtime_faster_hcpitch)
{
    sinusoid_realtime(RubberBandStretcher::OptionEngineFaster |
                      RubberBandStretcher::OptionProcessRealTime |
                      RubberBandStretcher::OptionPitchHighConsistency,
                      4.0, 1.5);
}

BOOST_AUTO_TEST_CASE(sinusoid_fast_higher_realtime_faster)
{
    sinusoid_realtime(RubberBandStretcher::OptionEngineFaster |
                      RubberBandStretcher::OptionProcessRealTime,
                      0.5, 1.5);
}

BOOST_AUTO_TEST_CASE(sinusoid_fast_higher_realtime_faster_hqpitch)
{
    sinusoid_realtime(RubberBandStretcher::OptionEngineFaster |
                      RubberBandStretcher::OptionProcessRealTime |
                      RubberBandStretcher::OptionPitchHighQuality,
                      0.5, 1.5);
}

BOOST_AUTO_TEST_CASE(sinusoid_fast_higher_realtime_faster_hcpitch)
{
    sinusoid_realtime(RubberBandStretcher::OptionEngineFaster |
                      RubberBandStretcher::OptionProcessRealTime |
                      RubberBandStretcher::OptionPitchHighConsistency,
                      0.5, 1.5);
}

BOOST_AUTO_TEST_CASE(sinusoid_slow_lower_realtime_faster)
{
    sinusoid_realtime(RubberBandStretcher::OptionEngineFaster |
                      RubberBandStretcher::OptionProcessRealTime,
                      8.0, 0.5);
}

BOOST_AUTO_TEST_CASE(sinusoid_slow_lower_realtime_faster_hqpitch)
{
    sinusoid_realtime(RubberBandStretcher::OptionEngineFaster |
                      RubberBandStretcher::OptionProcessRealTime |
                      RubberBandStretcher::OptionPitchHighQuality,
                      8.0, 0.5);
}

BOOST_AUTO_TEST_CASE(sinusoid_slow_lower_realtime_faster_hcpitch)
{
    sinusoid_realtime(RubberBandStretcher::OptionEngineFaster |
                      RubberBandStretcher::OptionProcessRealTime |
                      RubberBandStretcher::OptionPitchHighConsistency,
                      8.0, 0.5);
}

BOOST_AUTO_TEST_CASE(sinusoid_realtime_long_blocksize_faster)
{
    sinusoid_realtime(RubberBandStretcher::OptionEngineFaster |
                      RubberBandStretcher::OptionProcessRealTime,
                      4.0, 0.5,
                      80000, true);
}

BOOST_AUTO_TEST_CASE(sinusoid_realtime_long_blocksize_faster_stretch)
{
    sinusoid_realtime(RubberBandStretcher::OptionEngineFaster |
                      RubberBandStretcher::OptionProcessRealTime,
                      2.0, 1.0,
                      80000, true);
}

BOOST_AUTO_TEST_CASE(sinusoid_realtime_long_blocksize_faster_shrink)
{
    sinusoid_realtime(RubberBandStretcher::OptionEngineFaster |
                      RubberBandStretcher::OptionProcessRealTime,
                      0.8, 1.0,
                      80000, true);
}

BOOST_AUTO_TEST_CASE(sinusoid_realtime_long_blocksize_faster_higher)
{
    sinusoid_realtime(RubberBandStretcher::OptionEngineFaster |
                      RubberBandStretcher::OptionProcessRealTime,
                      1.0, 2.0,
                      80000, true);
}

BOOST_AUTO_TEST_CASE(sinusoid_realtime_long_blocksize_faster_lower)
{
    sinusoid_realtime(RubberBandStretcher::OptionEngineFaster |
                      RubberBandStretcher::OptionProcessRealTime,
                      1.0, 0.5,
                      80000, true);
}

BOOST_AUTO_TEST_CASE(sinusoid_realtime_long_blocksize_finer)
{
    sinusoid_realtime(RubberBandStretcher::OptionEngineFiner |
                      RubberBandStretcher::OptionProcessRealTime,
                      4.0, 0.5,
                      80000, true);
}

BOOST_AUTO_TEST_CASE(sinusoid_realtime_long_blocksize_finer_stretch)
{
    sinusoid_realtime(RubberBandStretcher::OptionEngineFiner |
                      RubberBandStretcher::OptionProcessRealTime,
                      2.0, 1.0,
                      80000, true);
}

BOOST_AUTO_TEST_CASE(sinusoid_realtime_long_blocksize_finer_shift)
{
    sinusoid_realtime(RubberBandStretcher::OptionEngineFiner |
                      RubberBandStretcher::OptionProcessRealTime,
                      1.0, 0.5,
                      80000, true);
}

BOOST_AUTO_TEST_CASE(impulses_2x_offline_faster)
{
    int n = 10000;
    int rate = 44100;
    RubberBandStretcher stretcher
        (rate, 1, RubberBandStretcher::OptionEngineFaster);

    stretcher.setTimeRatio(2.0);

    vector<float> in(n, 0.f), out(n * 2, 0.f);

    in[100] = 1.f;
    in[101] = -1.f;

    in[5000] = 1.f;
    in[5001] = -1.f;

    in[9900] = 1.f;
    in[9901] = -1.f;
    
    float *inp = in.data(), *outp = out.data();

    stretcher.setMaxProcessSize(n);
    stretcher.setExpectedInputDuration(n);
    BOOST_TEST(stretcher.available() == 0);

    stretcher.study(&inp, n, true);
    BOOST_TEST(stretcher.available() == 0);

    stretcher.process(&inp, n, true);
    BOOST_TEST(stretcher.available() == n * 2);

    BOOST_TEST(stretcher.getStartDelay() == 0u); // offline mode
    
    size_t got = stretcher.retrieve(&outp, n * 2);
    BOOST_TEST(got == size_t(n) * 2);
    BOOST_TEST(stretcher.available() == -1);

    int peak0 = -1, peak1 = -1, peak2 = -1;
    float max;
    
    max = -2.f;
    for (int i = 0; i < n/2; ++i) {
        if (out[i] > max) { max = out[i]; peak0 = i; }
    }

    max = -2.f;
    for (int i = n/2; i < (n*3)/2; ++i) {
        if (out[i] > max) { max = out[i]; peak1 = i; }
    }

    max = -2.f;
    for (int i = (n*3)/2; i < n*2; ++i) {
        if (out[i] > max) { max = out[i]; peak2 = i; }
    }

    BOOST_TEST(peak0 == 100);
    BOOST_TEST(peak1 > n - 600);
    BOOST_TEST(peak1 < n + 500);
    BOOST_TEST(peak2 > n*2 - 600);
    BOOST_TEST(peak2 < n*2);
/*
    std::cout << "ms\tV" << std::endl;
    for (int i = 0; i < n*2; ++i) {
        std::cout << i << "\t" << out[i] << std::endl;
    }
*/
}

BOOST_AUTO_TEST_CASE(impulses_2x_offline_finer)
{
    int n = 10000;
    int rate = 44100;
    RubberBandStretcher stretcher
        (rate, 1, RubberBandStretcher::OptionEngineFiner);

    stretcher.setTimeRatio(2.0);

    vector<float> in(n, 0.f), out(n * 2, 0.f);

    in[100] = 1.f;
    in[101] = -1.f;

    in[5000] = 1.f;
    in[5001] = -1.f;

    in[9900] = 1.f;
    in[9901] = -1.f;
    
    float *inp = in.data(), *outp = out.data();

    stretcher.setMaxProcessSize(n);
    stretcher.setExpectedInputDuration(n);
    BOOST_TEST(stretcher.available() == 0);

    stretcher.study(&inp, n, true);
    BOOST_TEST(stretcher.available() == 0);

    stretcher.process(&inp, n, true);
    BOOST_TEST(stretcher.available() == n * 2);

    BOOST_TEST(stretcher.getStartDelay() == 0u); // offline mode
    
    size_t got = stretcher.retrieve(&outp, n * 2);
    BOOST_TEST(got == size_t(n) * 2);
    BOOST_TEST(stretcher.available() == -1);

    int peak0 = -1, peak1 = -1, peak2 = -1;
    float max;

    max = -2.f;
    for (int i = 0; i < n/2; ++i) {
        if (out[i] > max) { max = out[i]; peak0 = i; }
    }

    max = -2.f;
    for (int i = n/2; i < (n*3)/2; ++i) {
        if (out[i] > max) { max = out[i]; peak1 = i; }
    }

    max = -2.f;
    for (int i = (n*3)/2; i < n*2; ++i) {
        if (out[i] > max) { max = out[i]; peak2 = i; }
    }

    BOOST_TEST(peak0 == 100);
    BOOST_TEST(peak1 > n - 400);
    BOOST_TEST(peak1 < n + 50);
    BOOST_TEST(peak2 > n*2 - 600);
    BOOST_TEST(peak2 < n*2);
/*
    std::cout << "ms\tV" << std::endl;
    for (int i = 0; i < n*2; ++i) {
        std::cout << i << "\t" << out[i] << std::endl;
    }
*/
}

BOOST_AUTO_TEST_CASE(impulses_2x_5up_offline_finer)
{
    int n = 10000;
    int rate = 44100;
    RubberBandStretcher stretcher
        (rate, 1, RubberBandStretcher::OptionEngineFiner);

    stretcher.setTimeRatio(2.0);
    stretcher.setPitchScale(1.5);

    vector<float> in(n, 0.f), out(n * 2, 0.f);

    in[100] = 1.f;
    in[101] = -1.f;

    in[5000] = 1.f;
    in[5001] = -1.f;

    in[9900] = 1.f;
    in[9901] = -1.f;
    
    float *inp = in.data(), *outp = out.data();

    stretcher.setMaxProcessSize(n);
    stretcher.setExpectedInputDuration(n);
    BOOST_TEST(stretcher.available() == 0);

    stretcher.study(&inp, n, true);
    BOOST_TEST(stretcher.available() == 0);

    stretcher.process(&inp, n, true);
    BOOST_TEST(stretcher.available() == n * 2);

    BOOST_TEST(stretcher.getStartDelay() == 0u); // offline mode
    
    size_t got = stretcher.retrieve(&outp, n * 2);
    BOOST_TEST(got == size_t(n) * 2);
    BOOST_TEST(stretcher.available() == -1);

    int peak0 = -1, peak1 = -1, peak2 = -1;
    float max;

    max = -2.f;
    for (int i = 0; i < n/2; ++i) {
        if (out[i] > max) { max = out[i]; peak0 = i; }
    }

    max = -2.f;
    for (int i = n/2; i < (n*3)/2; ++i) {
        if (out[i] > max) { max = out[i]; peak1 = i; }
    }

    max = -2.f;
    for (int i = (n*3)/2; i < n*2; ++i) {
        if (out[i] > max) { max = out[i]; peak2 = i; }
    }

    BOOST_TEST(peak0 < 250);
    BOOST_TEST(peak1 > n - 400);
    BOOST_TEST(peak1 < n + 50);
    BOOST_TEST(peak2 > n*2 - 600);
    BOOST_TEST(peak2 < n*2);
/*
    std::cout << "ms\tV" << std::endl;
    for (int i = 0; i < n*2; ++i) {
        std::cout << i << "\t" << out[i] << std::endl;
    }
*/
}

static void impulses_realtime(RubberBandStretcher::Options options,
                              double timeRatio,
                              double pitchScale,
                              bool printDebug)
{
    int n = 10000;
    int nOut = int(ceil(n * timeRatio));
    int rate = 48000;
    int bs = 1024;

    RubberBandStretcher stretcher(rate, 1, options, timeRatio, pitchScale);

    if (printDebug) {
        stretcher.setDebugLevel(2);
    }
    
    vector<float> in(n, 0.f);

    in[100] = 1.f;
    in[101] = -1.f;

    in[5000] = 1.f;
    in[5001] = -1.f;

    in[9900] = 1.f;
    in[9901] = -1.f;

    vector<float> out = process_realtime(stretcher, in, nOut, bs,
                                         false, printDebug);

    int peak0 = -1, peak1 = -1, peak2 = -1;
    float max;

    max = -2.f;
    for (int i = 0; i < nOut/4; ++i) {
        if (out[i] > max) { max = out[i]; peak0 = i; }
    }

    max = -2.f;
    for (int i = nOut/4; i < (nOut*3)/4; ++i) {
        if (out[i] > max) { max = out[i]; peak1 = i; }
    }

    max = -2.f;
    for (int i = (nOut*3)/4; i < nOut; ++i) {
        if (out[i] > max) { max = out[i]; peak2 = i; }
    }

    // These limits aren't alarming, but it may be worth tightening
    // them and and taking a look at the waveforms
    
    BOOST_TEST(peak0 < int(ceil(210 * timeRatio)));
    BOOST_TEST(peak0 > int(ceil(50 * timeRatio)));

    BOOST_TEST(peak1 < int(ceil(5070 * timeRatio)));
    BOOST_TEST(peak1 > int(ceil(4640 * timeRatio)));

    BOOST_TEST(peak2 < int(ceil(9970 * timeRatio)));
    BOOST_TEST(peak2 > int(ceil(9670 * timeRatio)));

    if (printDebug) {
        std::cout << "#sample\tV" << std::endl;
        for (int i = 0; i < n*2; ++i) {
            std::cout << "#" << i << "\t" << out[i] << std::endl;
        }
    }
}

BOOST_AUTO_TEST_CASE(impulses_slow_samepitch_realtime_finer)
{
    impulses_realtime(RubberBandStretcher::OptionEngineFiner |
                      RubberBandStretcher::OptionProcessRealTime,
                      8.0, 1.0,
                      false);
}

BOOST_AUTO_TEST_CASE(impulses_fast_samepitch_realtime_finer)
{
    impulses_realtime(RubberBandStretcher::OptionEngineFiner |
                      RubberBandStretcher::OptionProcessRealTime,
                      0.5, 1.0,
                      false);
}

BOOST_AUTO_TEST_CASE(impulses_slow_higher_realtime_finer)
{
    impulses_realtime(RubberBandStretcher::OptionEngineFiner |
                      RubberBandStretcher::OptionProcessRealTime,
                      4.0, 1.5,
                      false);
}

BOOST_AUTO_TEST_CASE(impulses_slow_higher_realtime_finer_hqpitch)
{
    impulses_realtime(RubberBandStretcher::OptionEngineFiner |
                      RubberBandStretcher::OptionProcessRealTime |
                      RubberBandStretcher::OptionPitchHighQuality,
                      4.0, 1.5,
                      false);
}

BOOST_AUTO_TEST_CASE(impulses_slow_higher_realtime_finer_hcpitch)
{
    impulses_realtime(RubberBandStretcher::OptionEngineFiner |
                      RubberBandStretcher::OptionProcessRealTime |
                      RubberBandStretcher::OptionPitchHighConsistency,
                      4.0, 1.5,
                      false);
}

BOOST_AUTO_TEST_CASE(impulses_slow_lower_realtime_finer)
{
    impulses_realtime(RubberBandStretcher::OptionEngineFiner |
                      RubberBandStretcher::OptionProcessRealTime,
                      8.0, 0.5,
                      false);
}

BOOST_AUTO_TEST_CASE(impulses_slow_lower_realtime_finer_hqpitch)
{
    impulses_realtime(RubberBandStretcher::OptionEngineFiner |
                      RubberBandStretcher::OptionProcessRealTime |
                      RubberBandStretcher::OptionPitchHighQuality,
                      8.0, 0.5,
                      false);
}

BOOST_AUTO_TEST_CASE(impulses_slow_lower_realtime_finer_hcpitch)
{
    impulses_realtime(RubberBandStretcher::OptionEngineFiner |
                      RubberBandStretcher::OptionProcessRealTime |
                      RubberBandStretcher::OptionPitchHighConsistency,
                      8.0, 0.5,
                      false);
}

static void final_realtime(RubberBandStretcher::Options options,
                           double timeRatio,
                           double pitchScale,
                           bool finalAfterFinishing,
                           bool printDebug)
{
    int n = 10000;
    float freq = 440.f;
    int rate = 44100;
    int blocksize = 700;
    RubberBandStretcher stretcher(rate, 1, options);

    if (printDebug) {
        stretcher.setDebugLevel(2);
    }

    stretcher.setTimeRatio(timeRatio);
    stretcher.setPitchScale(pitchScale);

    int nOut = int(ceil(n * timeRatio));
    int excess = std::max(nOut, n);
    vector<float> in(n, 0.f), out(nOut + excess, 0.f);

    for (int i = 0; i < 100; ++i) {
        in[n - 101 + i] = sinf(float(i) * freq * M_PI * 2.f / float(rate));
    }
    
    // Prime the start
    {
        float *source = out.data(); // just reuse out because it's silent
        stretcher.process(&source, stretcher.getPreferredStartPad(), false);
    }

    float *inp = in.data(), *outp = out.data();

    stretcher.setMaxProcessSize(blocksize);
    BOOST_TEST(stretcher.available() == 0);

    int toSkip = stretcher.getStartDelay();
    
    int incount = 0, outcount = 0;
    while (true) {

        int inbs = std::min(blocksize, n - incount);

        bool final;
        if (finalAfterFinishing) {
            BOOST_TEST(inbs >= 0);
            final = (inbs == 0);
        } else {
            BOOST_TEST(inbs > 0);
            final = (incount + inbs >= n);
        }
        
        float *in = inp + incount;
        stretcher.process(&in, inbs, final);
        incount += inbs;

        int avail = stretcher.available();
        BOOST_TEST(avail >= 0);
        BOOST_TEST(outcount + avail < nOut + excess);

//        cerr << "in = " << inbs << ", incount now = " << incount << ", avail = " << avail << endl;

        float *out = outp + outcount;

        if (toSkip > 0) {
            int skipHere = std::min(toSkip, avail);
            size_t got = stretcher.retrieve(&out, skipHere);
            BOOST_TEST(got == size_t(skipHere));
            toSkip -= got;
//            cerr << "got = " << got << ", toSkip now = " << toSkip << ", n = " << n << endl;
        }

        avail = stretcher.available();
        if (toSkip == 0 && avail > 0) {
            size_t got = stretcher.retrieve(&out, avail);
            BOOST_TEST(got == size_t(avail));
            outcount += got;
//            cerr << "got = " << got << ", outcount = " << outcount << ", n = " << n << endl;
            if (final) {
                BOOST_TEST(stretcher.available() == -1);
            } else {
                BOOST_TEST(stretcher.available() == 0);
            }
        }

        if (final) break;
    }

    BOOST_TEST(outcount >= nOut);
    
    if (printDebug) {
        // The initial # is to allow grep on the test output
        std::cout << "#sample\tV" << std::endl;
        for (int i = 0; i < outcount; ++i) {
            std::cout << "#" << i << "\t" << out[i] << std::endl;
        }
    }

}

BOOST_AUTO_TEST_CASE(final_slow_samepitch_realtime_finer)
{
    final_realtime(RubberBandStretcher::OptionEngineFiner |
                   RubberBandStretcher::OptionProcessRealTime |
                   RubberBandStretcher::OptionPitchHighConsistency,
                   8.0, 1.0,
                   false,
                   false);
}

BOOST_AUTO_TEST_CASE(final_slow_samepitch_realtime_finer_after)
{
    final_realtime(RubberBandStretcher::OptionEngineFiner |
                   RubberBandStretcher::OptionProcessRealTime |
                   RubberBandStretcher::OptionPitchHighConsistency,
                   8.0, 1.0,
                   true,
                   false);
}

BOOST_AUTO_TEST_CASE(final_fast_samepitch_realtime_finer)
{
    final_realtime(RubberBandStretcher::OptionEngineFiner |
                   RubberBandStretcher::OptionProcessRealTime |
                   RubberBandStretcher::OptionPitchHighConsistency,
                   0.2, 1.0,
                   false,
                   false);
}

BOOST_AUTO_TEST_CASE(final_fast_samepitch_realtime_finer_after)
{
    final_realtime(RubberBandStretcher::OptionEngineFiner |
                   RubberBandStretcher::OptionProcessRealTime |
                   RubberBandStretcher::OptionPitchHighConsistency,
                   0.2, 1.0,
                   true,
                   false);
}

BOOST_AUTO_TEST_CASE(final_slow_higher_realtime_finer)
{
    final_realtime(RubberBandStretcher::OptionEngineFiner |
                   RubberBandStretcher::OptionProcessRealTime |
                   RubberBandStretcher::OptionPitchHighConsistency,
                   8.0, 1.5,
                   false,
                   false);
}

BOOST_AUTO_TEST_CASE(final_slow_higher_realtime_finer_after)
{
    final_realtime(RubberBandStretcher::OptionEngineFiner |
                   RubberBandStretcher::OptionProcessRealTime |
                   RubberBandStretcher::OptionPitchHighConsistency,
                   8.0, 1.5,
                   true,
                   false);
}

BOOST_AUTO_TEST_CASE(final_fast_higher_realtime_finer)
{
    final_realtime(RubberBandStretcher::OptionEngineFiner |
                   RubberBandStretcher::OptionProcessRealTime |
                   RubberBandStretcher::OptionPitchHighConsistency,
                   0.2, 1.5,
                   false,
                   false);
}

BOOST_AUTO_TEST_CASE(final_fast_higher_realtime_finer_after)
{
    final_realtime(RubberBandStretcher::OptionEngineFiner |
                   RubberBandStretcher::OptionProcessRealTime |
                   RubberBandStretcher::OptionPitchHighConsistency,
                   0.2, 1.5,
                   true,
                   false);
}

BOOST_AUTO_TEST_CASE(final_slow_lower_realtime_finer)
{
    final_realtime(RubberBandStretcher::OptionEngineFiner |
                   RubberBandStretcher::OptionProcessRealTime |
                   RubberBandStretcher::OptionPitchHighConsistency,
                   8.0, 0.5,
                   false,
                   false);
}

BOOST_AUTO_TEST_CASE(final_slow_lower_realtime_finer_after)
{
    final_realtime(RubberBandStretcher::OptionEngineFiner |
                   RubberBandStretcher::OptionProcessRealTime |
                   RubberBandStretcher::OptionPitchHighConsistency,
                   8.0, 0.5,
                   true,
                   false);
}

BOOST_AUTO_TEST_CASE(final_fast_lower_realtime_finer)
{
    final_realtime(RubberBandStretcher::OptionEngineFiner |
                   RubberBandStretcher::OptionProcessRealTime |
                   RubberBandStretcher::OptionPitchHighConsistency,
                   0.2, 0.5,
                   false,
                   false);
}

BOOST_AUTO_TEST_CASE(final_fast_lower_realtime_finer_after)
{
    final_realtime(RubberBandStretcher::OptionEngineFiner |
                   RubberBandStretcher::OptionProcessRealTime |
                   RubberBandStretcher::OptionPitchHighConsistency,
                   0.2, 0.5,
                   true,
                   false);
}

static void with_resets(RubberBandStretcher::Options options,
                        double timeRatio,
                        double pitchScale)
{
    const int n = 10000;
    const int nOut = int(round(n * timeRatio));
    const int rate = 44100;
    const bool realtime =
        (options & RubberBandStretcher::OptionProcessRealTime);

    vector<float> in(n, 0.f), outRef(nOut, 0.f), out(nOut, 0.f);

    in[100] = 1.f;
    in[101] = -1.f;

    in[5000] = 1.f;
    in[5001] = -1.f;

    in[9900] = 1.f;
    in[9901] = -1.f;
    
    int nExpected = 0;
    
    RubberBandStretcher *stretcher = nullptr;

    // Combinations we need to ensure produce identical outputs:
    // 1. With timeRatio and pitchScale passed to constructor
    // 2. Then reset called and run again
    // 3. New instance, no ratios passed to ctor, timeRatio and
    //    pitchScale set after construction
    // 4. Then reset called and run again
    // 5. Reset called again, timeRatio and pitchScale set again (to
    //    the same things, just in case the act of setting them
    //    changes anything) and run again
    // 6. More input supplied and left in stretcher (output not
    //    retrieved) - then reset called and run again

    for (int run = 1; run <= 6; ++run) { // run being index into list above

        float *inp = in.data(), *outp = out.data();

        if (run == 1 || run == 3) {
            delete stretcher;
            if (run == 1) {
                stretcher = new RubberBandStretcher(rate, 1, options,
                                                    timeRatio, pitchScale);
            } else {
                stretcher = new RubberBandStretcher(rate, 1, options);
                stretcher->setTimeRatio(timeRatio);
                stretcher->setPitchScale(pitchScale);
            }
        }

        if (run == 2 || run > 3) {
            stretcher->reset();
            if (run == 5) {
                stretcher->setTimeRatio(timeRatio);
                stretcher->setPitchScale(pitchScale);
            }
        }

        if (run == 6) {
            if (realtime) {
                stretcher->process(&inp, n / 2, false);
            } else {
                stretcher->study(&inp, n / 2, true);
                stretcher->process(&inp, n / 2, false);
            }
            stretcher->reset();
        }
        
        // In every case we request nOut samples into out, and nActual
        // records how many we actually get
        int nActual = 0;
        
        if (realtime) {

            int blocksize = 1024;
            int nIn = 0;

            while (nActual < nOut) {
                int bsHere = blocksize;
                bool final = false;
                if (nIn + blocksize > n) {
                    bsHere = n - nIn;
                    final = true;
                }
                stretcher->process(&inp, bsHere, final);
                nIn += bsHere;
                inp += bsHere;
                int avail = stretcher->available();
                if (avail < 0) break;
                int toGet = avail;
                if (nActual + toGet > nOut) {
                    toGet = nOut - nActual;
                }
                int got = (int)stretcher->retrieve(&outp, toGet);
                nActual += got;
                outp += got;
                if (final) break;
            }
                
            BOOST_TEST(nActual <= nOut);
            
        } else {
            
            stretcher->setMaxProcessSize(n);
            stretcher->setExpectedInputDuration(n);
            BOOST_TEST(stretcher->available() == 0);

            stretcher->study(&inp, n, true);
            BOOST_TEST(stretcher->available() == 0);

            stretcher->process(&inp, n, true);
            BOOST_TEST(stretcher->available() == nOut);

            BOOST_TEST(stretcher->getStartDelay() == 0u); // offline mode
    
            nActual = (int)stretcher->retrieve(&outp, nOut);
            BOOST_TEST(nActual == nOut);
            BOOST_TEST(stretcher->available() == -1);
        }

        if (run == 1) { // set up expectations for subsequent runs

            for (int i = 0; i < nActual; ++i) {
                outRef[i] = out[i];
            }
            nExpected = nActual;

        } else { // check expectations
            
            BOOST_TEST(nActual == nExpected);
            
            for (int i = 0; i < nActual; ++i) {
                BOOST_TEST(out[i] == outRef[i]);
                if (out[i] != outRef[i]) {
                    std::cerr << "Failure at index " << i << " of "
                              << nActual << " in run " << run << std::endl;
                    break;
                }
            }
        }
    }

    delete stretcher;
}

BOOST_AUTO_TEST_CASE(with_resets_1x_0up_offline_finer)
{
    with_resets(RubberBandStretcher::OptionEngineFiner, 1.0, 1.0);
}

BOOST_AUTO_TEST_CASE(with_resets_2x_0up_offline_finer)
{
    with_resets(RubberBandStretcher::OptionEngineFiner, 2.0, 1.0);
}

BOOST_AUTO_TEST_CASE(with_resets_2x_5up_offline_finer)
{
    with_resets(RubberBandStretcher::OptionEngineFiner, 2.0, 1.5);
}

BOOST_AUTO_TEST_CASE(with_resets_1x_0up_offline_faster)
{
    with_resets(RubberBandStretcher::OptionEngineFaster, 1.0, 1.0);
}

BOOST_AUTO_TEST_CASE(with_resets_2x_0up_offline_faster)
{
    with_resets(RubberBandStretcher::OptionEngineFaster, 2.0, 1.0);
}

BOOST_AUTO_TEST_CASE(with_resets_2x_5up_offline_faster)
{
    with_resets(RubberBandStretcher::OptionEngineFaster, 2.0, 1.5);
}

BOOST_AUTO_TEST_CASE(with_resets_1x_0up_realtime_finer)
{
    with_resets(RubberBandStretcher::OptionProcessRealTime | RubberBandStretcher::OptionEngineFiner, 1.0, 1.0);
}

BOOST_AUTO_TEST_CASE(with_resets_2x_0up_realtime_finer)
{
    with_resets(RubberBandStretcher::OptionProcessRealTime | RubberBandStretcher::OptionEngineFiner, 2.0, 1.0);
}

BOOST_AUTO_TEST_CASE(with_resets_2x_5up_realtime_finer)
{
    with_resets(RubberBandStretcher::OptionProcessRealTime | RubberBandStretcher::OptionEngineFiner, 2.0, 1.5);
}

BOOST_AUTO_TEST_CASE(with_resets_1x_0up_realtime_faster)
{
    with_resets(RubberBandStretcher::OptionProcessRealTime | RubberBandStretcher::OptionEngineFaster, 1.0, 1.0);
}

BOOST_AUTO_TEST_CASE(with_resets_2x_0up_realtime_faster)
{
    with_resets(RubberBandStretcher::OptionProcessRealTime | RubberBandStretcher::OptionEngineFaster, 2.0, 1.0);
}

BOOST_AUTO_TEST_CASE(with_resets_2x_5up_realtime_faster)
{
    with_resets(RubberBandStretcher::OptionProcessRealTime | RubberBandStretcher::OptionEngineFaster, 2.0, 1.5);
}

BOOST_AUTO_TEST_CASE(long_blocksize_with_smoothing)
{
    // Test added because the smoothing option was calling alloca in a
    // loop, which could run us out of stack. This test is best used
    // with a small stack artificially enforced via e.g. ulimit -s 32
    
    int n = 10000;
    float freq = 440.f;
    int rate = 44100;
    RubberBandStretcher stretcher
        (rate, 1,
         RubberBandStretcher::OptionEngineFaster |
         RubberBandStretcher::OptionProcessOffline |
         RubberBandStretcher::OptionSmoothingOn);
    
    vector<float> in(n);
    for (int i = 0; i < n; ++i) {
        in[i] = sinf(float(i) * freq * M_PI * 2.f / float(rate));
    }
    float *inp = in.data();

    stretcher.setMaxProcessSize(n);
    stretcher.setExpectedInputDuration(n);

    stretcher.study(&inp, n, true);
    stretcher.process(&inp, n, true);
}

BOOST_AUTO_TEST_SUITE_END()
