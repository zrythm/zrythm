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

#include "R3LiveShifter.h"

#include "../common/VectorOpsComplex.h"
#include "../common/Profiler.h"

#include <array>

namespace RubberBand {

R3LiveShifter::R3LiveShifter(Parameters parameters, Log log) :
    m_log(log),
    m_parameters(validateSampleRate(parameters)),
    m_limits(parameters.options, m_parameters.sampleRate),
    m_pitchScale(1.0),
    m_formantScale(0.0),
    m_guide(Guide::Parameters
            (m_parameters.sampleRate, true),
            m_log),
    m_guideConfiguration(m_guide.getConfiguration()),
    m_channelAssembly(m_parameters.channels),
    m_initialResamplerDelays(32, 32),
    m_useReadahead(false),
    m_prevInhop(m_limits.maxInhopWithReadahead / 2),
    m_prevOuthop(m_prevInhop),
    m_firstProcess(true),
    m_unityCount(0)
{
    Profiler profiler("R3LiveShifter::R3LiveShifter");
    initialise();
}

void
R3LiveShifter::initialise()
{
    m_log.log(1, "R3LiveShifter::R3LiveShifter: rate, options",
              m_parameters.sampleRate, m_parameters.options);

    if (!isSingleWindowed()) {
        m_log.log(1, "R3LiveShifter::R3LiveShifter: multi window enabled");
    }

    if (m_parameters.options & RubberBandLiveShifter::OptionWindowMedium) {
        m_log.log(1, "R3LiveShifter::R3LiveShifter: readahead enabled");
        m_useReadahead = true;
    }

    double maxClassifierFrequency = 16000.0;
    if (maxClassifierFrequency > m_parameters.sampleRate/2) {
        maxClassifierFrequency = m_parameters.sampleRate/2;
    }
    int classificationBins =
        int(floor(m_guideConfiguration.classificationFftSize *
                  maxClassifierFrequency / m_parameters.sampleRate));
    
    BinSegmenter::Parameters segmenterParameters
        (m_guideConfiguration.classificationFftSize,
         classificationBins, m_parameters.sampleRate, 18);
    
    BinClassifier::Parameters classifierParameters
        (classificationBins, 9, 1, 10, 2.0, 2.0);

    if (isSingleWindowed()) {
        classifierParameters.horizontalFilterLength = 7;
    }

    int inRingBufferSize = getWindowSourceSize() * 4;
    int outRingBufferSize = getWindowSourceSize() * 4;

    m_channelData.clear();
    
    for (int c = 0; c < m_parameters.channels; ++c) {
        m_channelData.push_back(std::make_shared<ChannelData>
                                (segmenterParameters,
                                 classifierParameters,
                                 m_guideConfiguration.longestFftSize,
                                 getWindowSourceSize(),
                                 inRingBufferSize,
                                 outRingBufferSize));
        for (int b = 0; b < m_guideConfiguration.fftBandLimitCount; ++b) {
            const auto &band = m_guideConfiguration.fftBandLimits[b];
            int fftSize = band.fftSize;
            m_channelData[c]->scales[fftSize] =
                std::make_shared<ChannelScaleData>
                (fftSize, m_guideConfiguration.longestFftSize);
        }
        m_channelData[c]->guidance.phaseReset.present = true;
        m_channelData[c]->guidance.phaseReset.f0 = 0.0;
        m_channelData[c]->guidance.phaseReset.f1 = m_parameters.sampleRate / 2.0;
    }

    m_scaleData.clear();
    
    for (int b = 0; b < m_guideConfiguration.fftBandLimitCount; ++b) {
        const auto &band = m_guideConfiguration.fftBandLimits[b];
        int fftSize = band.fftSize;
        GuidedPhaseAdvance::Parameters guidedParameters
            (fftSize, m_parameters.sampleRate, m_parameters.channels,
             isSingleWindowed());
        m_scaleData[fftSize] = std::make_shared<ScaleData>
            (guidedParameters, m_log);
    }

    createResamplers();

    if (!m_pitchScale.is_lock_free()) {
        m_log.log(0, "R3LiveShifter: WARNING: std::atomic<double> is not lock-free");
    }
}

WindowType
R3LiveShifter::ScaleData::analysisWindowShape()
{
    if (singleWindowMode) {
        return HannWindow;
    } else {
        if (fftSize < 1024 || fftSize > 2048) return HannWindow;
        else return NiemitaloForwardWindow;
    }
}

int
R3LiveShifter::ScaleData::analysisWindowLength()
{
    return fftSize;
}

WindowType
R3LiveShifter::ScaleData::synthesisWindowShape()
{
    if (singleWindowMode) {
        return HannWindow;
    } else {
        if (fftSize < 1024 || fftSize > 2048) return HannWindow;
        else return NiemitaloReverseWindow;
    }
}

int
R3LiveShifter::ScaleData::synthesisWindowLength()
{
    if (singleWindowMode) {
        return fftSize;
    } else {
        if (fftSize > 2048) return fftSize/2;
        else return fftSize;
    }
}

void
R3LiveShifter::setPitchScale(double scale)
{
    m_log.log(2, "R3LiveShifter::setPitchScale", scale);

    if (scale == m_pitchScale) return;
    m_pitchScale = scale;

    if (m_firstProcess) {
        measureResamplerDelay();
    }
}

void
R3LiveShifter::setFormantScale(double scale)
{
    m_log.log(2, "R3LiveShifter::setFormantScale", scale);
    m_formantScale = scale;
}

void
R3LiveShifter::setFormantOption(RubberBandLiveShifter::Options options)
{
    int mask = (RubberBandLiveShifter::OptionFormantShifted |
                RubberBandLiveShifter::OptionFormantPreserved);
    m_parameters.options &= ~mask;
    options &= mask;
    m_parameters.options |= options;
}

void
R3LiveShifter::createResamplers()
{
    Profiler profiler("R3LiveShifter::createResamplers");
    
    Resampler::Parameters resamplerParameters;
    resamplerParameters.quality = Resampler::FastestTolerable;
    resamplerParameters.initialSampleRate = m_parameters.sampleRate;
    resamplerParameters.maxBufferSize = m_guideConfiguration.longestFftSize;
    resamplerParameters.dynamism = Resampler::RatioOftenChanging;
    resamplerParameters.ratioChange = Resampler::SuddenRatioChange;

    int debug = m_log.getDebugLevel();
    if (debug > 0) --debug;
    resamplerParameters.debugLevel = debug;
    
    m_inResampler = std::unique_ptr<Resampler>
        (new Resampler(resamplerParameters, m_parameters.channels));

    m_outResampler = std::unique_ptr<Resampler>
        (new Resampler(resamplerParameters, m_parameters.channels));

    measureResamplerDelay();
}

void
R3LiveShifter::measureResamplerDelay()
{
    // Delay in the sense that the resampler at first returns fewer
    // samples than requested, not that the samples are delayed within
    // the returned buffer relative to their positions at input. We
    // actually add the delay ourselves in the first block because we
    // need a fixed block size and the resampler doesn't return one.

    int bs = getBlockSize();
    std::vector<float> inbuf(bs * m_parameters.channels, 0.f);
    auto outbuf = inbuf;
    
    int incount = m_inResampler->resampleInterleaved
        (outbuf.data(), bs, inbuf.data(), bs, getInRatio(), false);
    m_inResampler->reset();

    int outcount = m_outResampler->resampleInterleaved
        (outbuf.data(), bs, inbuf.data(), bs, getOutRatio(), false);
    m_outResampler->reset();
    
    m_initialResamplerDelays = { bs - incount, bs - outcount };

    m_log.log(1, "R3LiveShifter::measureResamplerDelay: inRatio, outRatio ", getInRatio(), getOutRatio());
    m_log.log(1, "R3LiveShifter::measureResamplerDelay: measured delays ", m_initialResamplerDelays.first, m_initialResamplerDelays.second);
}

double
R3LiveShifter::getPitchScale() const
{
    return m_pitchScale;
}

double
R3LiveShifter::getFormantScale() const
{
    return m_formantScale;
}

size_t
R3LiveShifter::getPreferredStartPad() const
{
    return 0;
}

size_t
R3LiveShifter::getStartDelay() const
{
    int inDelay = getWindowSourceSize() + m_initialResamplerDelays.first;
    int outDelay = int(floor(inDelay * getOutRatio())) + m_initialResamplerDelays.second;

    int total = outDelay;
    int bs = getBlockSize();
    if (m_pitchScale > 1.0) {
        total += bs * (m_pitchScale - 1.0);
    } else if (m_pitchScale < 1.0) {
        total -= bs * (1.0 / m_pitchScale - 1.0);
    }

    m_log.log(2, "R3LiveShifter::getStartDelay: inDelay, outDelay", inDelay, outDelay);
    m_log.log(1, "R3LiveShifter::getStartDelay", total);
    return total;
}

size_t
R3LiveShifter::getChannelCount() const
{
    return m_parameters.channels;
}

void
R3LiveShifter::reset()
{
    m_inResampler->reset();
    m_outResampler->reset();

    m_unityCount = 0;

    m_prevInhop = m_limits.maxInhopWithReadahead / 2;
    m_prevOuthop = int(round(m_prevInhop * m_pitchScale));
    m_firstProcess = true;

    for (auto &it : m_scaleData) {
        it.second->guided.reset();
    }

    for (auto &cd : m_channelData) {
        cd->reset();
    }

    measureResamplerDelay();
}

size_t
R3LiveShifter::getBlockSize() const
{
    return 512;
}

void
R3LiveShifter::shift(const float *const *input, float *const *output)
{
    Profiler profiler("R3LiveShifter::shift");

    int incount = int(getBlockSize());
    
    m_log.log(2, "R3LiveShifter::shift: start of shift with incount", incount);
    m_log.log(2, "R3LiveShifter::shift: initially in inbuf", m_channelData[0]->inbuf->getReadSpace());
    m_log.log(2, "R3LiveShifter::shift: initially in outbuf", m_channelData[0]->outbuf->getReadSpace());

    int pad = 0;
    if (m_firstProcess) {
        pad = getWindowSourceSize();
        if (m_pitchScale > 1.0) {
            pad = int(ceil(pad * m_pitchScale));
        }
        m_log.log(2, "R3LiveShifter::shift: extending input with pre-pad", incount, pad);
        for (int c = 0; c < m_parameters.channels; ++c) {
            m_channelData[c]->inbuf->zero(pad);
        }
    }        
    
    readIn(input);

    double outRatio = getOutRatio();
    int requiredInOutbuf = int(ceil(incount / outRatio));
    generate(requiredInOutbuf);
    
    int got = readOut(output, incount);

    if (got < incount) {
        m_log.log(0, "R3LiveShifter::shift: ERROR: internal error: insufficient data at output (wanted, got)", incount, got);
        for (int c = 0; c < m_parameters.channels; ++c) {
            for (int i = got; i < incount; ++i) {
                if (i > 0) output[c][i] = output[c][i-1] * 0.9f;
                else output[c][i] = 0.f;
            }
        }
    }

    m_log.log(2, "R3LiveShifter::shift: end of process with incount", incount);
    m_log.log(2, "R3LiveShifter::shift: remaining in inbuf", m_channelData[0]->inbuf->getReadSpace());
    m_log.log(2, "R3LiveShifter::shift: remaining in outbuf", m_channelData[0]->outbuf->getReadSpace());
    m_log.log(2, "R3LiveShifter::shift: returning", got);

    m_firstProcess = false;
}

void
R3LiveShifter::readIn(const float *const *input)
{
    int incount = int(getBlockSize());
    
    int ws = m_channelData[0]->inbuf->getWriteSpace();
    if (ws < incount) {
        m_log.log(0, "R3LiveShifter::process: ERROR: internal error: insufficient space in inbuf (wanted, got)", incount, ws);
        return;
    }
        
    for (int c = 0; c < m_parameters.channels; ++c) {
        auto &cd = m_channelData.at(c);
        m_channelAssembly.resampled[c] = cd->resampled.data();
    }

    if (useMidSide()) {
        auto &c0 = m_channelData.at(0)->mixdown;
        auto &c1 = m_channelData.at(1)->mixdown;
        for (int i = 0; i < incount; ++i) {
            float l = input[0][i];
            float r = input[1][i];
            float m = (l + r) / 2.f;
            float s = (l - r) / 2.f;
            c0[i] = m;
            c1[i] = s;
        }
        m_channelAssembly.input[0] = m_channelData.at(0)->mixdown.data();
        m_channelAssembly.input[1] = m_channelData.at(1)->mixdown.data();
    } else {
        for (int c = 0; c < m_parameters.channels; ++c) {
            m_channelAssembly.input[c] = input[c];
        }
    }
    
    double inRatio = getInRatio();
    m_log.log(2, "R3LiveShifter::readIn: ratio", inRatio);
    
    int resampleBufSize = int(m_channelData.at(0)->resampled.size());

    int resampleOutput = m_inResampler->resample
        (m_channelAssembly.resampled.data(),
         resampleBufSize,
         m_channelAssembly.input.data(),
         incount,
         inRatio,
         false);

    m_log.log(2, "R3LiveShifter::readIn: writing to inbuf from resampled data, former read space and samples being added", m_channelData[0]->inbuf->getReadSpace(), resampleOutput);

    if (m_firstProcess) {
        int expected = floor(incount * inRatio);
        if (resampleOutput < expected) {
            m_log.log(2, "R3LiveShifter::readIn: resampler left us short on first process, pre-padding output: expected and obtained", expected, resampleOutput);
            for (int c = 0; c < m_parameters.channels; ++c) {
                m_channelData[c]->inbuf->zero(expected - resampleOutput);
            }
        }
    }
    
    for (int c = 0; c < m_parameters.channels; ++c) {
        m_channelData[c]->inbuf->write
            (m_channelData.at(c)->resampled.data(),
             resampleOutput);
    }
}

void
R3LiveShifter::generate(int requiredInOutbuf)
{
    Profiler profiler("R3LiveShifter::generate");

    auto &cd0 = m_channelData.at(0);
    int alreadyGenerated = cd0->outbuf->getReadSpace();
    if (alreadyGenerated >= requiredInOutbuf) {
        m_log.log(2, "R3LiveShifter::generate: have already generated required count", alreadyGenerated, requiredInOutbuf);
        return;
    }

    m_log.log(2, "R3LiveShifter::generate: alreadyGenerated, requiredInOutbuf", alreadyGenerated, requiredInOutbuf);

    int toGenerate = requiredInOutbuf - alreadyGenerated;

    int channels = m_parameters.channels;

    int ws = getWindowSourceSize();

    // We always leave at least ws in inbuf, and this function is
    // called after some input has just been written to inbuf, so we
    // must have more than ws samples in there.

    int atInput = cd0->inbuf->getReadSpace();
    if (atInput <= ws) {
        m_log.log(2, "R3LiveShifter::generate: insufficient samples at input: have and require more than", atInput, ws);
        return;
    }

    m_log.log(2, "R3LiveShifter::generate: atInput, toLeave", atInput, ws);

    int toConsume = atInput - ws;

    m_log.log(2, "R3LiveShifter::generate: toConsume, toGenerate", toConsume, toGenerate);

    int indicativeInhop = m_limits.maxInhopWithReadahead;
    int indicativeOuthop = m_limits.maxPreferredOuthop;
    
    int minHopsAtInput = 1 + (toConsume - 1) / indicativeInhop;
    int minHopsAtOutput = 1 + (toGenerate - 1) / indicativeOuthop;
    
    int hops = std::max(minHopsAtInput, minHopsAtOutput);

    m_log.log(2, "R3LiveShifter::generate: indicative inhop, outhop", indicativeInhop, indicativeOuthop);
    m_log.log(2, "R3LiveShifter::generate: minHopsAtInput, minHopsAtOutput", minHopsAtInput, minHopsAtOutput);
    m_log.log(2, "R3LiveShifter::generate: hops", hops);

    for (int hop = 0; hop < hops; ++hop) {

        Profiler profiler2("R3LiveShifter::generate/loop");

        if (toConsume <= 0) {
            m_log.log(2, "R3LiveShifter::generate: ERROR: toConsume is zero at top of loop, hop and hops", hop, hops);
            break;
        }
        
        int inhop = 1 + (toConsume - 1) / hops;
        int outhop = 1 + (toGenerate - 1) / hops;

        if (inhop > toConsume) {
            inhop = toConsume;
        }
        
        m_log.log(2, "R3LiveShifter::generate: inhop, outhop", inhop, outhop);
        
        // Now inhop is the distance by which the input stream will be
        // advanced after our current frame has been read, and outhop
        // is the distance by which the output will be advanced after
        // it has been emitted; m_prevInhop and m_prevOuthop are the
        // corresponding values the last time a frame was processed.
        //
        // Our phase adjustments need to be based on the distances we
        // have advanced the input and output since the previous
        // frame, not the distances we are about to advance them, so
        // they use the m_prev values.

        if (inhop != m_prevInhop) {
            m_log.log(2, "R3LiveShifter::generate: change in inhop", m_prevInhop, inhop);
        }
        if (outhop != m_prevOuthop) {
            m_log.log(2, "R3LiveShifter::generate: change in outhop", m_prevOuthop, outhop);
        }
        
        m_log.log(2, "R3LiveShifter::generate: write space and outhop", cd0->outbuf->getWriteSpace(), outhop);

        // NB our ChannelData, ScaleData, and ChannelScaleData maps
        // contain shared_ptrs; whenever we retain one of them in a
        // variable, we do so by reference to avoid copying the
        // shared_ptr (as that is not realtime safe). Same goes for
        // the map iterators

        // Analysis
        
        for (int c = 0; c < channels; ++c) {
            analyseChannel(c, inhop, m_prevInhop, m_prevOuthop);
        }

        // Phase update. This is synchronised across all channels
        
        for (auto &it : m_channelData[0]->scales) {
            int fftSize = it.first;
            for (int c = 0; c < channels; ++c) {
                auto &cd = m_channelData.at(c);
                auto &scale = cd->scales.at(fftSize);
                m_channelAssembly.mag[c] = scale->mag.data();
                m_channelAssembly.phase[c] = scale->phase.data();
                m_channelAssembly.prevMag[c] = scale->prevMag.data();
                m_channelAssembly.guidance[c] = &cd->guidance;
                m_channelAssembly.outPhase[c] = scale->advancedPhase.data();
            }
            m_scaleData.at(fftSize)->guided.advance
                (m_channelAssembly.outPhase.data(),
                 m_channelAssembly.mag.data(),
                 m_channelAssembly.phase.data(),
                 m_channelAssembly.prevMag.data(),
                 m_guideConfiguration,
                 m_channelAssembly.guidance.data(),
                 useMidSide(),
                 m_prevInhop,
                 m_prevOuthop);
        }

        for (int c = 0; c < channels; ++c) {
            adjustPreKick(c);
        }
        
        // Resynthesis
        
        for (int c = 0; c < channels; ++c) {
            synthesiseChannel(c, outhop, false);
        }

        // Emit

        int writeCount = outhop;
        int advanceCount = inhop;
        
        for (int c = 0; c < channels; ++c) {
            auto &cd = m_channelData.at(c);
            cd->outbuf->write(cd->mixdown.data(), writeCount);
            cd->inbuf->skip(advanceCount);
        }

        m_log.log(2, "R3LiveShifter::generate: writing and advancing", writeCount, advanceCount);

        m_prevInhop = inhop;
        m_prevOuthop = outhop;
    }

    m_log.log(2, "R3LiveShifter::generate: returning, now have generated", cd0->outbuf->getReadSpace());
}

int
R3LiveShifter::readOut(float *const *output, int outcount)
{
    double outRatio = getOutRatio();
    m_log.log(2, "R3LiveShifter::readOut: outcount and ratio", outcount, outRatio);

    int resampledCount = 0;
    bool fillingTail = false;

    while (resampledCount < outcount) {

        int fromOutbuf;

        if (fillingTail) {
            fromOutbuf = 1;
        } else {
            fromOutbuf = int(floor(outcount / outRatio));
            if (fromOutbuf == 0) {
                fromOutbuf = 1;
            }
        }
        
        m_log.log(2, "R3LiveShifter::readOut: fillingTail and fromOutbuf", fillingTail, fromOutbuf);

        int got = fromOutbuf;
    
        for (int c = 0; c < m_parameters.channels; ++c) {
            auto &cd = m_channelData.at(c);
            int available = cd->outbuf->getReadSpace();
            int gotHere = cd->outbuf->read
                (cd->resampled.data(), std::min(got, available));
            if (gotHere < got) {
                if (c > 0) {
                    m_log.log(0, "R3LiveShifter::readOut: WARNING: channel imbalance detected");
                }
            }
            got = std::min(got, std::max(gotHere, 0));
        }

        m_log.log(2, "R3LiveShifter::readOut: requested and got from outbufs", fromOutbuf, got);
        m_log.log(2, "R3LiveShifter::readOut: leaving behind", m_channelData.at(0)->outbuf->getReadSpace());

        for (int c = 0; c < m_parameters.channels; ++c) {
            auto &cd = m_channelData.at(c);
            m_channelAssembly.resampled[c] = cd->resampled.data();
            m_channelAssembly.mixdown[c] = output[c] + resampledCount;
        }

        auto resampledHere = m_outResampler->resample
            (m_channelAssembly.mixdown.data(),
             outcount - resampledCount,
             m_channelAssembly.resampled.data(),
             got,
             outRatio,
             false);

        m_log.log(2, "R3LiveShifter::readOut: resampledHere", resampledHere);

        if (got == 0 && resampledHere == 0) {
            m_log.log(2, "R3LiveShifter::readOut: made no progress, finishing");
            break;
        }            
        
        resampledCount += resampledHere;

        fillingTail = true;
    }
    
    if (useMidSide()) {
        for (int i = 0; i < resampledCount; ++i) {
            float m = output[0][i];
            float s = output[1][i];
            float l = m + s;
            float r = m - s;
            output[0][i] = l;
            output[1][i] = r;
        }
    }

    m_log.log(2, "R3LiveShifter::readOut: resampled to", resampledCount);

    if (resampledCount < outcount) {
        if (m_firstProcess) {
            m_log.log(2, "R3LiveShifter::readOut: resampler left us short on first process, pre-padding output: expected and obtained", outcount, resampledCount);
            int prepad = outcount - resampledCount;
            for (int c = 0; c < m_parameters.channels; ++c) {
                v_move(output[c] + prepad, output[c], resampledCount);
                v_zero(output[c], prepad);
            }
            resampledCount = outcount;
        } else {
            m_log.log(0, "R3LiveShifter::readOut: WARNING: Failed to obtain enough samples from resampler", resampledCount, outcount);
        }
    }

    m_log.log(2, "R3LiveShifter::readOut: returning", resampledCount);
    
    return resampledCount;
}

void
R3LiveShifter::analyseChannel(int c, int inhop, int prevInhop, int prevOuthop)
{
    Profiler profiler("R3LiveShifter::analyseChannel");
    
    auto &cd = m_channelData.at(c);

    int sourceSize = cd->windowSource.size();
    process_t *buf = cd->windowSource.data();

    int readSpace = cd->inbuf->getReadSpace();
    if (readSpace < sourceSize) {
        cd->inbuf->peek(buf, readSpace);
        v_zero(buf + readSpace, sourceSize - readSpace);
    } else {
        cd->inbuf->peek(buf, sourceSize);
    }
    
    // We have an unwindowed time-domain frame in buf that is as long
    // as required for the union of all FFT sizes and readahead
    // hops. Populate the various sizes from it with aligned centres,
    // windowing as we copy. The classification scale is handled
    // separately because it has readahead, so skip it here. (In
    // single-window mode that means we do nothing here, since the
    // classification scale is the only one.)

    int longest = m_guideConfiguration.longestFftSize;
    int classify = m_guideConfiguration.classificationFftSize;

    for (auto &it: cd->scales) {
        int fftSize = it.first;
        if (fftSize == classify) continue;
        int offset = (longest - fftSize) / 2;
        m_scaleData.at(fftSize)->analysisWindow.cut
            (buf + offset, it.second->timeDomain.data());
    }

    auto &classifyScale = cd->scales.at(classify);
    ClassificationReadaheadData &readahead = cd->readahead;
    bool copyFromReadahead = false;
    
    if (m_useReadahead) {
        
        // The classification scale has a one-hop readahead, so
        // populate the readahead from further down the long
        // unwindowed frame.

        m_scaleData.at(classify)->analysisWindow.cut
            (buf + (longest - classify) / 2 + inhop,
             readahead.timeDomain.data());

        // If inhop has changed since the previous frame, we must
        // populate the classification scale (but for
        // analysis/resynthesis rather than classification) anew
        // rather than reuse the previous frame's readahead.

        copyFromReadahead = cd->haveReadahead;
        if (inhop != prevInhop) copyFromReadahead = false;
    }
    
    if (!copyFromReadahead) {
        m_scaleData.at(classify)->analysisWindow.cut
            (buf + (longest - classify) / 2,
             classifyScale->timeDomain.data());
    }

    // FFT shift, forward FFT, and carry out cartesian-polar
    // conversion for each FFT size.

    // For the classification scale we need magnitudes for the full
    // range (polar only in a subset) and we operate in the readahead,
    // pulling current values from the existing readahead (except
    // where the inhop has changed as above, in which case we need to
    // do both readahead and current)

    if (m_useReadahead) {

        if (copyFromReadahead) {
            v_copy(classifyScale->mag.data(),
                   readahead.mag.data(),
                   classifyScale->bufSize);
            v_copy(classifyScale->phase.data(),
                   readahead.phase.data(),
                   classifyScale->bufSize);
        }

        v_fftshift(readahead.timeDomain.data(), classify);
        m_scaleData.at(classify)->fft.forward(readahead.timeDomain.data(),
                                              classifyScale->real.data(),
                                              classifyScale->imag.data());

        for (int b = 0; b < m_guideConfiguration.fftBandLimitCount; ++b) {
            const auto &band = m_guideConfiguration.fftBandLimits[b];
            if (band.fftSize == classify) {
                ToPolarSpec spec;
                spec.magFromBin = 0;
                spec.magBinCount = classify/2 + 1;
                spec.polarFromBin = band.b0min;
                spec.polarBinCount = band.b1max - band.b0min + 1;
                convertToPolar(readahead.mag.data(),
                               readahead.phase.data(),
                               classifyScale->real.data(),
                               classifyScale->imag.data(),
                               spec);
                    
                v_scale(classifyScale->mag.data(),
                        1.0 / double(classify),
                        classifyScale->mag.size());
                break;
            }
        }

        cd->haveReadahead = true;
    }

    // For the others (and the classify as well, if the inhop has
    // changed or we aren't using readahead or haven't filled the
    // readahead yet) we operate directly in the scale data and
    // restrict the range for cartesian-polar conversion
            
    for (auto &it: cd->scales) {
        int fftSize = it.first;
        if (fftSize == classify && copyFromReadahead) {
            continue;
        }
        
        auto &scale = it.second;
        
        v_fftshift(scale->timeDomain.data(), fftSize);

        m_scaleData.at(fftSize)->fft.forward(scale->timeDomain.data(),
                                             scale->real.data(),
                                             scale->imag.data());

        for (int b = 0; b < m_guideConfiguration.fftBandLimitCount; ++b) {
            const auto &band = m_guideConfiguration.fftBandLimits[b];
            if (band.fftSize == fftSize) {

                ToPolarSpec spec;

                // For the classify scale we always want the full
                // range, as all the magnitudes (though not
                // necessarily all phases) are potentially relevant to
                // classification and formant analysis. But this case
                // here only happens if we don't copyFromReadahead -
                // the normal case is above and, er, copies from the
                // previous readahead.
                if (fftSize == classify) {
                    spec.magFromBin = 0;
                    spec.magBinCount = classify/2 + 1;
                    spec.polarFromBin = band.b0min;
                    spec.polarBinCount = band.b1max - band.b0min + 1;
                } else {
                    spec.magFromBin = band.b0min;
                    spec.magBinCount = band.b1max - band.b0min + 1;
                    spec.polarFromBin = spec.magFromBin;
                    spec.polarBinCount = spec.magBinCount;
                }

                convertToPolar(scale->mag.data(),
                               scale->phase.data(),
                               scale->real.data(),
                               scale->imag.data(),
                               spec);

                v_scale(scale->mag.data() + spec.magFromBin,
                        1.0 / double(fftSize),
                        spec.magBinCount);
                
                break;
            }
        }
    }

    if (m_parameters.options & RubberBandLiveShifter::OptionFormantPreserved) {
        analyseFormant(c);
        adjustFormant(c);
    }
        
    // Use the classification scale to get a bin segmentation and
    // calculate the adaptive frequency guide for this channel

    v_copy(cd->classification.data(), cd->nextClassification.data(),
           cd->classification.size());

    if (m_useReadahead) {
        cd->classifier->classify(readahead.mag.data(),
                                 cd->nextClassification.data());
    } else {
        cd->classifier->classify(classifyScale->mag.data(),
                                 cd->nextClassification.data());
    }

    cd->prevSegmentation = cd->segmentation;
    cd->segmentation = cd->nextSegmentation;
    cd->nextSegmentation = cd->segmenter->segment(cd->nextClassification.data());
/*
    if (c == 0) {
        double pb = cd->nextSegmentation.percussiveBelow;
        double pa = cd->nextSegmentation.percussiveAbove;
        double ra = cd->nextSegmentation.residualAbove;
        int pbb = binForFrequency(pb, classify, m_parameters.sampleRate);
        int pab = binForFrequency(pa, classify, m_parameters.sampleRate);
        int rab = binForFrequency(ra, classify, m_parameters.sampleRate);
        std::cout << "pb = " << pb << ", pbb = " << pbb << std::endl;
        std::cout << "pa = " << pa << ", pab = " << pab << std::endl;
        std::cout << "ra = " << ra << ", rab = " << rab << std::endl;
        std::cout << "s:";
        for (int i = 0; i <= classify/2; ++i) {
            if (i > 0) std::cout << ",";
            if (i < pbb || (i >= pab && i <= rab)) {
                std::cout << "1";
            } else {
                std::cout << "0";
            }
        }
        std::cout << std::endl;
    }
*/

    double ratio = m_pitchScale;

    if (fabs(ratio - 1.0) < 1.0e-7) {
        ++m_unityCount;
    } else {
        m_unityCount = 0;
    }

    bool tighterChannelLock =
        m_parameters.options & RubberBandLiveShifter::OptionChannelsTogether;

    double magMean = v_mean(classifyScale->mag.data() + 1, classify/2);

    bool resetOnSilence = true;
    if (useMidSide() && c == 1) {
        // Do not phase reset on silence in the side channel - the
        // reset is propagated across to the mid channel, giving
        // constant resets for e.g. mono material in a stereo
        // configuration
        resetOnSilence = false;
    }
    
    if (m_useReadahead) {
        m_guide.updateGuidance(ratio,
                               prevOuthop,
                               classifyScale->mag.data(),
                               classifyScale->prevMag.data(),
                               cd->readahead.mag.data(),
                               cd->segmentation,
                               cd->prevSegmentation,
                               cd->nextSegmentation,
                               magMean,
                               m_unityCount,
                               true,
                               tighterChannelLock,
                               resetOnSilence,
                               cd->guidance);
    } else {
        m_guide.updateGuidance(ratio,
                               prevOuthop,
                               classifyScale->prevMag.data(),
                               classifyScale->prevMag.data(),
                               classifyScale->mag.data(),
                               cd->segmentation,
                               cd->prevSegmentation,
                               cd->nextSegmentation,
                               magMean,
                               m_unityCount,
                               true,
                               tighterChannelLock,
                               resetOnSilence,
                               cd->guidance);
    }
    
/*
    if (c == 0) {
        if (cd->guidance.kick.present) {
            std::cout << "k:2" << std::endl;
        } else if (cd->guidance.preKick.present) {
            std::cout << "k:1" << std::endl;
        } else {
            std::cout << "k:0" << std::endl;
        }
    }
*/
}

void
R3LiveShifter::analyseFormant(int c)
{
    Profiler profiler("R3LiveShifter::analyseFormant");

    auto &cd = m_channelData.at(c);
    auto &f = *cd->formant;

    int fftSize = f.fftSize;
    int binCount = fftSize/2 + 1;
    
    auto &scale = cd->scales.at(fftSize);
    auto &scaleData = m_scaleData.at(fftSize);

    scaleData->fft.inverseCepstral(scale->mag.data(), f.cepstra.data());
    
    int cutoff = int(floor(m_parameters.sampleRate / 650.0));
    if (cutoff < 1) cutoff = 1;

    f.cepstra[0] /= 2.0;
    f.cepstra[cutoff-1] /= 2.0;
    for (int i = cutoff; i < fftSize; ++i) {
        f.cepstra[i] = 0.0;
    }
    v_scale(f.cepstra.data(), 1.0 / double(fftSize), cutoff);

    scaleData->fft.forward(f.cepstra.data(), f.envelope.data(), f.spare.data());

    v_exp(f.envelope.data(), binCount);
    v_square(f.envelope.data(), binCount);

    for (int i = 0; i < binCount; ++i) {
        if (f.envelope[i] > 1.0e10) f.envelope[i] = 1.0e10;
    }
}

void
R3LiveShifter::adjustFormant(int c)
{
    Profiler profiler("R3LiveShifter::adjustFormant");

    auto &cd = m_channelData.at(c);
        
    for (auto &it : cd->scales) {
        
        int fftSize = it.first;
        auto &scale = it.second;

        int highBin = int(floor(fftSize * 10000.0 / m_parameters.sampleRate));
        process_t targetFactor = process_t(cd->formant->fftSize) / process_t(fftSize);
        process_t formantScale = m_formantScale;
        if (formantScale == 0.0) formantScale = 1.0 / m_pitchScale;
        process_t sourceFactor = targetFactor / formantScale;
        process_t maxRatio = 60.0;
        process_t minRatio = 1.0 / maxRatio;

        for (int b = 0; b < m_guideConfiguration.fftBandLimitCount; ++b) {
            const auto &band = m_guideConfiguration.fftBandLimits[b];
            if (band.fftSize != fftSize) continue;
            for (int i = band.b0min; i < band.b1max && i < highBin; ++i) {
                process_t source = cd->formant->envelopeAt(i * sourceFactor);
                process_t target = cd->formant->envelopeAt(i * targetFactor);
                if (target > 0.0) {
                    process_t ratio = source / target;
                    if (ratio < minRatio) ratio = minRatio;
                    if (ratio > maxRatio) ratio = maxRatio;
                    scale->mag[i] *= ratio;
                }
            }
        }
    }
}

void
R3LiveShifter::adjustPreKick(int c)
{
    if (isSingleWindowed()) return;
    
    Profiler profiler("R3LiveShifter::adjustPreKick");

    auto &cd = m_channelData.at(c);
    auto fftSize = cd->guidance.fftBands[0].fftSize;
    if (cd->guidance.preKick.present) {
        auto &scale = cd->scales.at(fftSize);
        int from = binForFrequency(cd->guidance.preKick.f0,
                                   fftSize, m_parameters.sampleRate);
        int to = binForFrequency(cd->guidance.preKick.f1,
                                 fftSize, m_parameters.sampleRate);
        for (int i = from; i <= to; ++i) {
            process_t diff = scale->mag[i] - scale->prevMag[i];
            if (diff > 0.0) {
                scale->pendingKick[i] = diff;
                scale->mag[i] -= diff;
            }
        }
    } else if (cd->guidance.kick.present) {
        auto &scale = cd->scales.at(fftSize);
        int from = binForFrequency(cd->guidance.preKick.f0,
                                   fftSize, m_parameters.sampleRate);
        int to = binForFrequency(cd->guidance.preKick.f1,
                                 fftSize, m_parameters.sampleRate);
        for (int i = from; i <= to; ++i) {
            scale->mag[i] += scale->pendingKick[i];
            scale->pendingKick[i] = 0.0;
        }
    }                
}

void
R3LiveShifter::synthesiseChannel(int c, int outhop, bool draining)
{
    Profiler profiler("R3LiveShifter::synthesiseChannel");
    
    int longest = m_guideConfiguration.longestFftSize;

    auto &cd = m_channelData.at(c);

    for (int b = 0; b < cd->guidance.fftBandCount; ++b) {

        const auto &band = cd->guidance.fftBands[b];
        int fftSize = band.fftSize;
        
        auto &scale = cd->scales.at(fftSize);
        auto &scaleData = m_scaleData.at(fftSize);

        // copy to prevMag before filtering
        v_copy(scale->prevMag.data(),
               scale->mag.data(),
               scale->bufSize);

        process_t winscale = process_t(outhop) / scaleData->windowScaleFactor;

        m_log.log(2, "R3LiveShifter::synthesiseChannel: outhop and winscale", outhop, winscale);
        
        // The frequency filter is applied naively in the frequency
        // domain. Aliasing is reduced by the shorter resynthesis
        // window. We resynthesise each scale individually, then sum -
        // it's easier to manage scaling for in situations with a
        // varying resynthesis hop
            
        int lowBin = binForFrequency(band.f0, fftSize, m_parameters.sampleRate);
        int highBin = binForFrequency(band.f1, fftSize, m_parameters.sampleRate);
        if (highBin % 2 == 0 && highBin > 0) --highBin;

        int n = scale->mag.size();
        if (lowBin >= n) lowBin = n - 1;
        if (highBin >= n) highBin = n - 1;
        if (highBin < lowBin) highBin = lowBin;
        
        if (lowBin > 0) {
            v_zero(scale->real.data(), lowBin);
            v_zero(scale->imag.data(), lowBin);
        }

        v_scale(scale->mag.data() + lowBin, winscale, highBin - lowBin);

        v_polar_to_cartesian(scale->real.data() + lowBin,
                             scale->imag.data() + lowBin,
                             scale->mag.data() + lowBin,
                             scale->advancedPhase.data() + lowBin,
                             highBin - lowBin);
        
        if (highBin < scale->bufSize) {
            v_zero(scale->real.data() + highBin, scale->bufSize - highBin);
            v_zero(scale->imag.data() + highBin, scale->bufSize - highBin);
        }

        scaleData->fft.inverse(scale->real.data(),
                               scale->imag.data(),
                               scale->timeDomain.data());
        
        v_fftshift(scale->timeDomain.data(), fftSize);

        // Synthesis window may be shorter than analysis window, so
        // copy and cut only from the middle of the time-domain frame;
        // and the accumulator length always matches the longest FFT
        // size, so as to make mixing straightforward, so there is an
        // additional offset needed for the target
                
        int synthesisWindowSize = scaleData->synthesisWindow.getSize();
        int fromOffset = (fftSize - synthesisWindowSize) / 2;
        int toOffset = (longest - synthesisWindowSize) / 2;

        scaleData->synthesisWindow.cutAndAdd
            (scale->timeDomain.data() + fromOffset,
             scale->accumulator.data() + toOffset);
    }

    // Mix this channel and move the accumulator along
            
    float *mixptr = cd->mixdown.data();
    v_zero(mixptr, outhop);

    for (auto &it : cd->scales) {
        auto &scale = it.second;

        process_t *accptr = scale->accumulator.data();
        for (int i = 0; i < outhop; ++i) {
            mixptr[i] += float(accptr[i]);
        }

        int n = scale->accumulator.size() - outhop;
        v_move(accptr, accptr + outhop, n);
        v_zero(accptr + n, outhop);

        if (draining) {
            if (scale->accumulatorFill > outhop) {
                auto newFill = scale->accumulatorFill - outhop;
                m_log.log(2, "draining: reducing accumulatorFill from, to", scale->accumulatorFill, newFill);
                scale->accumulatorFill = newFill;
            } else {
                scale->accumulatorFill = 0;
            }
        } else {
            scale->accumulatorFill = scale->accumulator.size();
        }
    }
}

}

