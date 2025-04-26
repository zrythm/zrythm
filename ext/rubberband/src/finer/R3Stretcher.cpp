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

#include "R3Stretcher.h"

#include "../common/VectorOpsComplex.h"
#include "../common/Profiler.h"

#include <array>

namespace RubberBand {

R3Stretcher::R3Stretcher(Parameters parameters,
                         double initialTimeRatio,
                         double initialPitchScale,
                         Log log) :
    m_log(log),
    m_parameters(validateSampleRate(parameters)),
    m_limits(parameters.options, m_parameters.sampleRate),
    m_timeRatio(initialTimeRatio),
    m_pitchScale(initialPitchScale),
    m_formantScale(0.0),
    m_guide(Guide::Parameters
            (m_parameters.sampleRate,
             m_parameters.options & RubberBandStretcher::OptionWindowShort),
            m_log),
    m_guideConfiguration(m_guide.getConfiguration()),
    m_channelAssembly(m_parameters.channels),
    m_useReadahead(true),
    m_inhop(1),
    m_prevInhop(1),
    m_prevOuthop(1),
    m_unityCount(0),
    m_startSkip(0),
    m_studyInputDuration(0),
    m_suppliedInputDuration(0),
    m_totalTargetDuration(0),
    m_consumedInputDuration(0),
    m_lastKeyFrameSurpassed(0),
    m_totalOutputDuration(0),
    m_mode(ProcessMode::JustCreated)
{
    Profiler profiler("R3Stretcher::R3Stretcher");

    initialise();
}

void
R3Stretcher::initialise()
{
    m_log.log(1, "R3Stretcher::R3Stretcher: rate, options",
              m_parameters.sampleRate, m_parameters.options);
    m_log.log(1, "R3Stretcher::R3Stretcher: initial time ratio and pitch scale",
              m_timeRatio, m_pitchScale);

    if (isRealTime()) {
        m_log.log(1, "R3Stretcher::R3Stretcher: real-time mode");
    } else {
        m_log.log(1, "R3Stretcher::R3Stretcher: offline mode");
    }

    if (isSingleWindowed()) {
        m_log.log(1, "R3Stretcher::R3Stretcher: intermediate shorter-window mode requested");
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

    int inRingBufferSize = getWindowSourceSize() * 16;
    int outRingBufferSize = getWindowSourceSize() * 16;

    int hopBufferSize =
        2 * std::max(m_limits.maxInhop, m_limits.maxPreferredOuthop);
    
    m_channelData.clear();
    
    for (int c = 0; c < m_parameters.channels; ++c) {
        m_channelData.push_back(std::make_shared<ChannelData>
                                (segmenterParameters,
                                 classifierParameters,
                                 getWindowSourceSize(),
                                 inRingBufferSize,
                                 outRingBufferSize,
                                 hopBufferSize));
        for (int b = 0; b < m_guideConfiguration.fftBandLimitCount; ++b) {
            const auto &band = m_guideConfiguration.fftBandLimits[b];
            int fftSize = band.fftSize;
            m_channelData[c]->scales[fftSize] =
                std::make_shared<ChannelScaleData>
                (fftSize, m_guideConfiguration.longestFftSize);
        }
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

    m_calculator = std::unique_ptr<StretchCalculator>
        (new StretchCalculator(int(round(m_parameters.sampleRate)), //!!! which is a double...
                               1, false, // no fixed inputIncrement
                               m_log));

    if (isRealTime()) {
        createResampler();
        // In offline mode we don't create the resampler yet - we
        // don't want to have one at all if the pitch ratio is 1.0,
        // but that could change before the first process call, so we
        // create the resampler if needed then
    }

    calculateHop();

    if (!m_inhop.is_lock_free()) {
        m_log.log(0, "R3Stretcher: WARNING: std::atomic<int> is not lock-free");
    }
    if (!m_timeRatio.is_lock_free()) {
        m_log.log(0, "R3Stretcher: WARNING: std::atomic<double> is not lock-free");
    }
}

WindowType
R3Stretcher::ScaleData::analysisWindowShape()
{
    if (singleWindowMode) {
        return HannWindow;
    } else {
        if (fftSize < 1024 || fftSize > 2048) return HannWindow;
        else return NiemitaloForwardWindow;
    }
}

int
R3Stretcher::ScaleData::analysisWindowLength()
{
    return fftSize;
}

WindowType
R3Stretcher::ScaleData::synthesisWindowShape()
{
    if (singleWindowMode) {
        return HannWindow;
    } else {
        if (fftSize < 1024 || fftSize > 2048) return HannWindow;
        else return NiemitaloReverseWindow;
    }
}

int
R3Stretcher::ScaleData::synthesisWindowLength()
{
    if (singleWindowMode) {
        return fftSize;
    } else {
        if (fftSize > 2048) return fftSize/2;
        else return fftSize;
    }
}

void
R3Stretcher::setTimeRatio(double ratio)
{
    if (!isRealTime()) {
        if (m_mode == ProcessMode::Studying ||
            m_mode == ProcessMode::Processing) {
            m_log.log(0, "R3Stretcher::setTimeRatio: Cannot set time ratio while studying or processing in non-RT mode");
            return;
        }
    }

    if (ratio == m_timeRatio) return;
    m_timeRatio = ratio;
    calculateHop();
}

void
R3Stretcher::setPitchScale(double scale)
{
    if (!isRealTime()) {
        if (m_mode == ProcessMode::Studying ||
            m_mode == ProcessMode::Processing) {
            m_log.log(0, "R3Stretcher::setPitchScale: Cannot set pitch scale while studying or processing in non-RT mode");
            return;
        }
    }

    if (scale == m_pitchScale) return;
    m_pitchScale = scale;
    calculateHop();
}

void
R3Stretcher::setFormantScale(double scale)
{
    if (!isRealTime()) {
        if (m_mode == ProcessMode::Studying ||
            m_mode == ProcessMode::Processing) {
            m_log.log(0, "R3Stretcher::setFormantScale: Cannot set formant scale while studying or processing in non-RT mode");
            return;
        }
    }

    m_formantScale = scale;
}

void
R3Stretcher::setFormantOption(RubberBandStretcher::Options options)
{
    int mask = (RubberBandStretcher::OptionFormantShifted |
                RubberBandStretcher::OptionFormantPreserved);
    m_parameters.options &= ~mask;
    options &= mask;
    m_parameters.options |= options;
}

void
R3Stretcher::setPitchOption(RubberBandStretcher::Options)
{
    m_log.log(0, "R3Stretcher::setPitchOption: Option change after construction is not supported in R3 engine");
}

void
R3Stretcher::setKeyFrameMap(const std::map<size_t, size_t> &mapping)
{
    if (isRealTime()) {
        m_log.log(0, "R3Stretcher::setKeyFrameMap: Cannot specify key frame map in RT mode");
        return;
    }
    if (m_mode == ProcessMode::Processing || m_mode == ProcessMode::Finished) {
        m_log.log(0, "R3Stretcher::setKeyFrameMap: Cannot specify key frame map after process() has begun");
        return;
    }

    m_keyFrameMap = mapping;
}

void
R3Stretcher::createResampler()
{
    Profiler profiler("R3Stretcher::createResampler");
    
    Resampler::Parameters resamplerParameters;
    resamplerParameters.quality = Resampler::FastestTolerable;
    resamplerParameters.initialSampleRate = m_parameters.sampleRate;
    resamplerParameters.maxBufferSize = m_guideConfiguration.longestFftSize;

    if (isRealTime()) {
        // If we knew the caller would never change ratio, we could
        // supply RatioMostlyFixed - but it can have such overhead
        // when the ratio *does* change (and it's not RT-safe overhead
        // either) that a single call would kill RT use
        resamplerParameters.dynamism = Resampler::RatioOftenChanging;
        resamplerParameters.ratioChange = Resampler::SmoothRatioChange;
    } else {
        resamplerParameters.dynamism = Resampler::RatioMostlyFixed;
        resamplerParameters.ratioChange = Resampler::SuddenRatioChange;
    }

    int debug = m_log.getDebugLevel();
    if (debug > 0) --debug;
    resamplerParameters.debugLevel = debug;
    
    m_resampler = std::unique_ptr<Resampler>
        (new Resampler(resamplerParameters, m_parameters.channels));

    bool before, after;
    areWeResampling(&before, &after);
    if (before) {
        if (after) {
            m_log.log(0, "R3Stretcher: WARNING: we think we are resampling both before and after!");
        } else {
            m_log.log(1, "createResampler: resampling before");
        }
    } else {
        if (after) {
            m_log.log(1, "createResampler: resampling after");
        }
    }
}

void
R3Stretcher::calculateHop()
{
    if (m_pitchScale <= 0.0) {
        // This special case is likelier than one might hope, because
        // of naive initialisations in programs that set it from a
        // variable
        m_log.log(0, "WARNING: Pitch scale must be greater than zero! Resetting it to default, no pitch shift will happen", m_pitchScale);
        m_pitchScale = 1.0;
    }
    if (m_timeRatio <= 0.0) {
        // Likewise
        m_log.log(0, "WARNING: Time ratio must be greater than zero! Resetting it to default, no time stretch will happen", m_timeRatio);
        m_timeRatio = 1.0;
    }
    if (m_pitchScale != m_pitchScale || m_timeRatio != m_timeRatio ||
        m_pitchScale == m_pitchScale/2.0 || m_timeRatio == m_timeRatio/2.0) {
        m_log.log(0, "WARNING: NaN or Inf presented for time ratio or pitch scale! Resetting it to default, no time stretch will happen", m_timeRatio, m_pitchScale);
        m_timeRatio = 1.0;
        m_pitchScale = 1.0;
    }

    double ratio = getEffectiveRatio();

    // In R2 we generally targeted a certain inhop, and calculated
    // outhop from that. Here we are the other way around. We aim for
    // outhop = 256 at ratios around 1, reducing down to 128 for
    // ratios far below 1 and up to 512 for ratios far above. As soon
    // as outhop exceeds 256 we have to drop the 1024-bin FFT, as the
    // overlap will be inadequate for it (that's among the jobs of the
    // Guide class) so we don't want to go above 256 until at least
    // factor 1.5. Also we can't go above 512 without changing the
    // window shape or dropping the 2048-bin FFT, and we can't do
    // either of those dynamically.

    double proposedOuthop = 256.0;
    if (ratio > 1.5) {
        proposedOuthop = pow(2.0, 8.0 + 2.0 * log10(ratio - 0.5));
    } else if (ratio < 1.0) {
        proposedOuthop = pow(2.0, 8.0 + 2.0 * log10(ratio));
    }

    if (isSingleWindowed()) {
        // the single (shorter) window mode actually uses a longer
        // synthesis window for the 2048-bin FFT and drops the
        // 1024-bin one, so it can survive longer hops, which is good
        // because reduced CPU consumption is the whole motivation
        proposedOuthop *= 2.0;
    }
    
    if (proposedOuthop > m_limits.maxPreferredOuthop) {
        proposedOuthop = m_limits.maxPreferredOuthop;
    }
    if (proposedOuthop < m_limits.minPreferredOuthop) {
        proposedOuthop = m_limits.minPreferredOuthop;
    }
    
    m_log.log(1, "calculateHop: ratio and proposed outhop", ratio, proposedOuthop);
    
    double inhop = proposedOuthop / ratio;
    if (inhop < m_limits.minInhop) {
        m_log.log(0, "R3Stretcher: WARNING: Ratio yields ideal inhop < minimum, results may be suspect", inhop, m_limits.minInhop);
        inhop = m_limits.minInhop;
    }
    if (inhop > m_limits.maxInhop) {
        // Log level 1, this is not as big a deal as < minInhop above
        m_log.log(1, "R3Stretcher: WARNING: Ratio yields ideal inhop > maximum, results may be suspect", inhop, m_limits.maxInhop);
        inhop = m_limits.maxInhop;
    }

    m_inhop = int(floor(inhop));
    m_log.log(1, "calculateHop: inhop and mean outhop", m_inhop, m_inhop * ratio);

    if (m_inhop < m_limits.maxInhopWithReadahead) {
        m_log.log(1, "calculateHop: using readahead; maxInhopWithReadahead", m_limits.maxInhopWithReadahead);
        m_useReadahead = true;
    } else {
        m_log.log(1, "calculateHop: not using readahead; maxInhopWithReadahead", m_limits.maxInhopWithReadahead);
        m_useReadahead = false;
    }

    if (m_mode == ProcessMode::JustCreated) {
        m_prevInhop = m_inhop;
        m_prevOuthop = int(round(m_inhop * getEffectiveRatio()));
    }
}

void
R3Stretcher::updateRatioFromMap()
{
    if (m_keyFrameMap.empty()) return;

    if (m_consumedInputDuration == 0) {
        m_timeRatio = double(m_keyFrameMap.begin()->second) /
            double(m_keyFrameMap.begin()->first);

        m_log.log(1, "initial key-frame map entry ",
                   double(m_keyFrameMap.begin()->first),
                   double(m_keyFrameMap.begin()->second));
        m_log.log(1, "giving initial ratio ", m_timeRatio);
        
        calculateHop();
        m_lastKeyFrameSurpassed = 0;
        return;
    }
    
    auto i0 = m_keyFrameMap.upper_bound(m_lastKeyFrameSurpassed);

    if (i0 == m_keyFrameMap.end()) {
        return;
    }

    if (m_consumedInputDuration >= i0->first) {

        m_log.log(1, "input duration surpasses pending key frame",
                   double(m_consumedInputDuration), double(i0->first));
        
        auto i1 = m_keyFrameMap.upper_bound(m_consumedInputDuration);

        size_t keyFrameAtInput, keyFrameAtOutput;
    
        if (i1 != m_keyFrameMap.end()) {
            keyFrameAtInput = i1->first;
            keyFrameAtOutput = i1->second;
        } else {
            keyFrameAtInput = m_studyInputDuration;
            keyFrameAtOutput = m_totalTargetDuration;
        }

        m_log.log(1, "current input and output",
                   double(m_consumedInputDuration), double(m_totalOutputDuration));
        m_log.log(1, "next key frame input and output",
                   double(keyFrameAtInput), double(keyFrameAtOutput));
        
        double ratio;

        if (keyFrameAtInput > i0->first) {
        
            size_t toKeyFrameAtInput, toKeyFrameAtOutput;
            
            toKeyFrameAtInput = keyFrameAtInput - i0->first;
            
            if (keyFrameAtOutput > i0->second) {
                toKeyFrameAtOutput = keyFrameAtOutput - i0->second;
            } else {
                m_log.log(1, "previous target key frame overruns next key frame (or total output duration)", i0->second, keyFrameAtOutput);
                toKeyFrameAtOutput = 1;
            }

            m_log.log(1, "diff to next key frame input and output",
                      double(toKeyFrameAtInput), double(toKeyFrameAtOutput));

            ratio = double(toKeyFrameAtOutput) / double(toKeyFrameAtInput);

        } else {
            m_log.log(1, "source key frame overruns following key frame or total input duration", i0->first, keyFrameAtInput);
            ratio = 1.0;
        }
        
        m_log.log(1, "new ratio", ratio);
    
        m_timeRatio = ratio;
        calculateHop();

        m_lastKeyFrameSurpassed = i0->first;
    }
}

double
R3Stretcher::getTimeRatio() const
{
    return m_timeRatio;
}

double
R3Stretcher::getPitchScale() const
{
    return m_pitchScale;
}

double
R3Stretcher::getFormantScale() const
{
    return m_formantScale;
}

size_t
R3Stretcher::getPreferredStartPad() const
{
    if (!isRealTime()) {
        return 0;
    } else {
        bool resamplingBefore = false;
        areWeResampling(&resamplingBefore, nullptr);
        size_t pad = getWindowSourceSize() / 2;
        if (resamplingBefore) {
            return size_t(ceil(pad * m_pitchScale));
        } else {
            return pad;
        }
    }
}

size_t
R3Stretcher::getStartDelay() const
{
    if (!isRealTime()) {
        return 0;
    } else {
        bool resamplingBefore = false;
        areWeResampling(&resamplingBefore, nullptr);
        size_t delay = getWindowSourceSize() / 2;
        if (resamplingBefore) {
            return delay;
        } else {
            return size_t(ceil(delay / m_pitchScale));
        }
    }
}

size_t
R3Stretcher::getChannelCount() const
{
    return m_parameters.channels;
}

void
R3Stretcher::reset()
{
    m_inhop = 1;
    m_prevInhop = 1;
    m_prevOuthop = 1;
    m_unityCount = 0;
    m_startSkip = 0;
    m_studyInputDuration = 0;
    m_suppliedInputDuration = 0;
    m_totalTargetDuration = 0;
    m_consumedInputDuration = 0;
    m_lastKeyFrameSurpassed = 0;
    m_totalOutputDuration = 0;
    m_keyFrameMap.clear();

    m_mode = ProcessMode::JustCreated;

    m_calculator->reset();
    
    if (m_resampler) {
        m_resampler->reset();
    }

    for (auto &it : m_scaleData) {
        it.second->guided.reset();
    }

    for (auto &cd : m_channelData) {
        cd->reset();
    }

    calculateHop();
}

void
R3Stretcher::study(const float *const *, size_t samples, bool)
{
    Profiler profiler("R3Stretcher::study");
    
    if (isRealTime()) {
        m_log.log(0, "R3Stretcher::study: Not meaningful in realtime mode");
        return;
    }

    if (m_mode == ProcessMode::Processing || m_mode == ProcessMode::Finished) {
        m_log.log(0, "R3Stretcher::study: Cannot study after processing");
        return;
    }
    
    if (m_mode == ProcessMode::JustCreated) {
        m_studyInputDuration = 0;
    }

    m_mode = ProcessMode::Studying;
    m_studyInputDuration += samples;
}

void
R3Stretcher::setExpectedInputDuration(size_t samples)
{
    m_suppliedInputDuration = samples;
}

size_t
R3Stretcher::getSamplesRequired() const
{
    if (available() != 0) return 0;
    int rs = m_channelData[0]->inbuf->getReadSpace();

    m_log.log(2, "getSamplesRequired: read space and window source size", rs, getWindowSourceSize());

    if (rs < getWindowSourceSize()) {

        int req = getWindowSourceSize() - rs;
        
        bool resamplingBefore = false;
        areWeResampling(&resamplingBefore, nullptr);

        if (!resamplingBefore) {
            return req;
        } else {
            int adjusted = int(ceil(double(req) * m_pitchScale));
            m_log.log(2, "getSamplesRequired: resamplingBefore is true, req and adjusted", req, adjusted);
            return adjusted;
        }
            
    } else {
        return 0;
    }
}

void
R3Stretcher::setMaxProcessSize(size_t requested)
{
    m_log.log(2, "R3Stretcher::setMaxProcessSize", requested);
    
    int n = m_limits.overallMaxProcessSize;
    if (requested > size_t(n)) {
        m_log.log(0, "R3Stretcher::setMaxProcessSize: request exceeds overall limit", requested, n);
    } else {
        n = int(requested);
    }

    ensureInbuf(n * 2, false);
    ensureOutbuf(n * 8, false);
}

size_t
R3Stretcher::getProcessSizeLimit() const
{
    return m_limits.overallMaxProcessSize;
}

void
R3Stretcher::ensureInbuf(int required, bool warn)
{
    int ws = m_channelData[0]->inbuf->getWriteSpace();
    if (required < ws) {
        return;
    }
    if (warn) {
        m_log.log(0, "R3Stretcher::ensureInbuf: WARNING: Forced to increase input buffer size. Either setMaxProcessSize was not properly called, process is being called repeatedly without retrieve, or an internal error has led to an incorrect resampler output calculation. Samples to write and space available", required, ws);
    }
    size_t oldSize = m_channelData[0]->inbuf->getSize();
    size_t newSize = oldSize - ws + required;
    if (newSize < oldSize * 2) newSize = oldSize * 2;
    m_log.log(warn ? 0 : 2, "R3Stretcher::ensureInbuf: old and new sizes", oldSize, newSize);
    for (int c = 0; c < m_parameters.channels; ++c) {
        auto newBuf = m_channelData[c]->inbuf->resized(newSize);
        m_channelData[c]->inbuf = std::unique_ptr<RingBuffer<float>>(newBuf);
        // mixdown is used for mid-side mixing as well as the single
        // hop output mix, so it needs to be enough to match the inbuf
        m_channelData[c]->mixdown.resize(newSize, 0.f);
    }
}

void
R3Stretcher::ensureOutbuf(int required, bool warn)
{
    int ws = m_channelData[0]->outbuf->getWriteSpace();
    if (required < ws) {
        return;
    }
    if (warn) {
        m_log.log(0, "R3Stretcher::ensureOutbuf: WARNING: Forced to increase output buffer size. Using smaller process blocks or an artificially larger value for setMaxProcessSize may avoid this. Samples to write and space available", required, ws);
    }
    size_t oldSize = m_channelData[0]->outbuf->getSize();
    size_t newSize = oldSize - ws + required;
    if (newSize < oldSize * 2) newSize = oldSize * 2;
    m_log.log(warn ? 0 : 2, "R3Stretcher::ensureOutbuf: old and new sizes", oldSize, newSize);
    for (int c = 0; c < m_parameters.channels; ++c) {
        auto newBuf = m_channelData[c]->outbuf->resized(newSize);
        m_channelData[c]->outbuf = std::unique_ptr<RingBuffer<float>>(newBuf);
    }
}

void
R3Stretcher::process(const float *const *input, size_t samples, bool final)
{
    Profiler profiler("R3Stretcher::process");
    
    if (m_mode == ProcessMode::Finished) {
        m_log.log(0, "R3Stretcher::process: Cannot process again after final chunk");
        return;
    }

    int n = m_limits.overallMaxProcessSize;
    if (samples > size_t(n)) {
        m_log.log(0, "R3Stretcher::process: request exceeds overall limit", samples, n);
    } else {
        n = int(samples);
    }

    if (!isRealTime()) {

        if (m_mode == ProcessMode::Studying) {
            m_totalTargetDuration =
                size_t(round(m_studyInputDuration * m_timeRatio));
            m_log.log(1, "study duration and target duration",
                      m_studyInputDuration, m_totalTargetDuration);
        } else if (m_mode == ProcessMode::JustCreated) {
            if (m_suppliedInputDuration != 0) {
                m_totalTargetDuration =
                    size_t(round(m_suppliedInputDuration * m_timeRatio));
                m_log.log(1, "supplied duration and target duration",
                          m_suppliedInputDuration, m_totalTargetDuration);
            }
        }

        // Update this on every process round, checking whether we've
        // surpassed the next key frame yet. This must follow the
        // overall target calculation above, which uses the "global"
        // time ratio, but precede any other use of the time ratio.
        
        if (!m_keyFrameMap.empty()) {
            updateRatioFromMap();
        }

        if (m_mode == ProcessMode::JustCreated ||
            m_mode == ProcessMode::Studying) {

            if (m_pitchScale != 1.0 && !m_resampler) {
                createResampler();
            }

            // Pad to half the frame. As with R2, in real-time mode we
            // don't do this -- it's better to start with a swoosh
            // than introduce more latency, and we don't want gaps
            // when the ratio changes.
            int pad = getWindowSourceSize() / 2;
            m_log.log(1, "offline mode: prefilling with", pad);
            ensureInbuf(pad);
            for (int c = 0; c < m_parameters.channels; ++c) {
                int zeroed = m_channelData[c]->inbuf->zero(pad);
                if (zeroed != pad) {
                    m_log.log(0, "R3Stretcher: WARNING: too few padding samples written", zeroed, pad);
                }
            }

            // NB by the time we skip this later we may have resampled
            // as well as stretched
            m_startSkip = int(round(pad / m_pitchScale));
            m_log.log(1, "start skip is", m_startSkip);
        }
    }

    bool resamplingBefore = false;
    areWeResampling(&resamplingBefore, nullptr);

    int channels = m_parameters.channels;
    int inputIx = 0;

    if (n == 0 && final) {

        m_log.log(2, "process: no samples but final specified, consuming");
        
        consume(true);

    } else while (inputIx < n) {

        int remaining = n - inputIx;

        int ws = m_channelData[0]->inbuf->getWriteSpace();
        if (ws == 0) {
            consume(false);
        }
        ensureInbuf(remaining);
        ws = m_channelData[0]->inbuf->getWriteSpace();
        
        if (resamplingBefore) {

            for (int c = 0; c < channels; ++c) {
                auto &cd = m_channelData.at(c);
                m_channelAssembly.resampled[c] = cd->resampled.data();
            }

            int resampleBufSize = int(m_channelData.at(0)->resampled.size());
            int maxResampleOutput = std::min(ws, resampleBufSize);
            
            int maxResampleInput = int(floor(maxResampleOutput * m_pitchScale));
            int resampleInput = std::min(remaining, maxResampleInput);
            if (resampleInput == 0) resampleInput = 1;

            m_log.log(2, "R3Stretcher::process: resamplingBefore is true, resampleInput and maxResampleOutput", resampleInput, maxResampleOutput);
            
            prepareInput(input, inputIx, resampleInput);

            bool finalHop = (final && inputIx + resampleInput >= n);
            
            int resampleOutput = m_resampler->resample
                (m_channelAssembly.resampled.data(),
                 maxResampleOutput,
                 m_channelAssembly.input.data(),
                 resampleInput,
                 1.0 / m_pitchScale,
                 finalHop);

            inputIx += resampleInput;
            
            for (int c = 0; c < m_parameters.channels; ++c) {

                int written = m_channelData[c]->inbuf->write
                    (m_channelData.at(c)->resampled.data(),
                     resampleOutput);

                if (written != resampleOutput) {
                    m_log.log(0, "R3Stretcher: WARNING: too few samples written to input buffer from resampler", written, resampleOutput);
                }
            }

        } else {
            int toWrite = std::min(ws, remaining);

            m_log.log(2, "process: resamplingBefore is false, writing to inbuf from supplied data, former read space and samples being added", m_channelData[0]->inbuf->getReadSpace(), toWrite);

            prepareInput(input, inputIx, toWrite);
            
            for (int c = 0; c < m_parameters.channels; ++c) {

                int written = m_channelData[c]->inbuf->write
                    (m_channelAssembly.input[c], toWrite);

                if (written != toWrite) {
                    m_log.log(0, "R3Stretcher: WARNING: too few samples written to input buffer", written, toWrite);
                }
            }
            
            inputIx += toWrite;
        }
        
        consume(final && inputIx >= n);
    }

    if (final) {
        // We don't distinguish between Finished and "draining, but
        // haven't yet delivered all the samples" because the
        // distinction is meaningless internally - it only affects
        // whether available() finds any samples in the buffer
        m_log.log(1, "final is set, entering Finished mode");
        m_mode = ProcessMode::Finished;
    } else {
        m_mode = ProcessMode::Processing;
    }
}

int
R3Stretcher::available() const
{
    int av = int(m_channelData[0]->outbuf->getReadSpace());
    if (av == 0 && m_mode == ProcessMode::Finished) {
        return -1;
    } else {
        return av;
    }
}

size_t
R3Stretcher::retrieve(float *const *output, size_t samples) const
{
    Profiler profiler("R3Stretcher::retrieve");
    
    int got = samples;
    
    m_log.log(2, "retrieve: requested, outbuf has", samples, m_channelData[0]->outbuf->getReadSpace());
    
    for (int c = 0; c < m_parameters.channels; ++c) {
        int gotHere = m_channelData[c]->outbuf->read(output[c], got);
        if (gotHere < got) {
            if (c > 0) {
                m_log.log(0, "R3Stretcher::retrieve: WARNING: channel imbalance detected");
            }
            got = std::min(got, std::max(gotHere, 0));
        }
    }

    if (useMidSide()) {
        for (int i = 0; i < got; ++i) {
            float m = output[0][i];
            float s = output[1][i];
            float l = m + s;
            float r = m - s;
            output[0][i] = l;
            output[1][i] = r;
        }
    }
    
    m_log.log(2, "retrieve: returning, outbuf now has", got, m_channelData[0]->outbuf->getReadSpace());

    return got;
}

void
R3Stretcher::prepareInput(const float *const *input, int ix, int n)
{
    if (useMidSide()) {
        auto &c0 = m_channelData.at(0)->mixdown;
        auto &c1 = m_channelData.at(1)->mixdown;
        int bufsize = c0.size();
        if (n > bufsize) {
            m_log.log(0, "R3Stretcher::prepareInput: WARNING: called with size greater than mixdown buffer length", n, bufsize);
            n = bufsize;
        }
        for (int i = 0; i < n; ++i) {
            float l = input[0][i + ix];
            float r = input[1][i + ix];
            float m = (l + r) / 2.f;
            float s = (l - r) / 2.f;
            c0[i] = m;
            c1[i] = s;
        }
        m_channelAssembly.input[0] = m_channelData.at(0)->mixdown.data();
        m_channelAssembly.input[1] = m_channelData.at(1)->mixdown.data();
    } else {
        for (int c = 0; c < m_parameters.channels; ++c) {
            m_channelAssembly.input[c] = input[c] + ix;
        }
    }
}

void
R3Stretcher::consume(bool final)
{
    Profiler profiler("R3Stretcher::consume");
    
    int longest = m_guideConfiguration.longestFftSize;
    int channels = m_parameters.channels;
    int inhop = m_inhop;

    bool resamplingAfter = false;
    areWeResampling(nullptr, &resamplingAfter);

    double effectivePitchRatio = 1.0 / m_pitchScale;
    if (m_resampler) {
        effectivePitchRatio =
            m_resampler->getEffectiveRatio(effectivePitchRatio);
    }
    
    int outhop = m_calculator->calculateSingle(m_timeRatio,
                                               effectivePitchRatio,
                                               1.f,
                                               inhop,
                                               longest,
                                               longest,
                                               true);

    if (outhop < 1) {
        m_log.log(0, "R3Stretcher::consume: WARNING: outhop calculated as", outhop);
        outhop = 1;
    }
    
    // Now inhop is the distance by which the input stream will be
    // advanced after our current frame has been read, and outhop is
    // the distance by which the output will be advanced after it has
    // been emitted; m_prevInhop and m_prevOuthop are the
    // corresponding values the last time a frame was processed (*not*
    // just the last time this function was called, since we can
    // return without doing anything if the output buffer is full).
    //
    // Our phase adjustments need to be based on the distances we have
    // advanced the input and output since the previous frame, not the
    // distances we are about to advance them, so they use the m_prev
    // values.

    if (inhop != m_prevInhop) {
        m_log.log(2, "change in inhop", double(m_prevInhop), double(inhop));
    }
    if (outhop != m_prevOuthop) {
        m_log.log(2, "change in outhop", double(m_prevOuthop), double(outhop));
    }

    auto &cd0 = m_channelData.at(0);

    m_log.log(2, "consume: write space and outhop", cd0->outbuf->getWriteSpace(), outhop);
        
    // NB our ChannelData, ScaleData, and ChannelScaleData maps
    // contain shared_ptrs; whenever we retain one of them in a
    // variable, we do so by reference to avoid copying the shared_ptr
    // (as that is not realtime safe). Same goes for the map iterators

    while (true) {

        Profiler profiler2("R3Stretcher::consume/loop");

        int readSpace = cd0->inbuf->getReadSpace();
        m_log.log(2, "consume: read space", readSpace);

        if (readSpace < getWindowSourceSize()) {
            if (final) {
                if (readSpace == 0) {
                    int fill = cd0->scales.at(longest)->accumulatorFill;
                    if (fill == 0) {
                        break;
                    } else {
                        m_log.log(1, "finished reading input, but samples remaining in output accumulator", fill);
                    }
                }
            } else {
                // await more input
                break;
            }
        }

        ensureOutbuf(outhop);
        
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
            synthesiseChannel(c, outhop, readSpace == 0);
        }
        
        // Resample

        int resampledCount = 0;
        if (resamplingAfter) {
            for (int c = 0; c < channels; ++c) {
                auto &cd = m_channelData.at(c);
                m_channelAssembly.mixdown[c] = cd->mixdown.data();
                m_channelAssembly.resampled[c] = cd->resampled.data();
            }

            bool finalHop = (final &&
                             readSpace < inhop &&
                             cd0->scales.at(longest)->accumulatorFill <= outhop);
            
            resampledCount = m_resampler->resample
                (m_channelAssembly.resampled.data(),
                 m_channelData[0]->resampled.size(),
                 m_channelAssembly.mixdown.data(),
                 outhop,
                 1.0 / m_pitchScale,
                 finalHop);
        }

        // Emit

        int writeCount = outhop;
        if (resamplingAfter) {
            writeCount = resampledCount;
        }
        if (!isRealTime()) {
            if (m_totalTargetDuration > 0 &&
                m_totalOutputDuration + writeCount > m_totalTargetDuration) {
                m_log.log(1, "writeCount would take output beyond target",
                          m_totalOutputDuration, m_totalTargetDuration);
                auto reduced = m_totalTargetDuration - m_totalOutputDuration;
                m_log.log(1, "reducing writeCount from and to", writeCount, reduced);
                writeCount = reduced;
            }
        }

        int advanceCount = inhop;
        if (advanceCount > readSpace) {
            // This should happen only when draining
            if (!final) {
                m_log.log(0, "R3Stretcher: WARNING: readSpace < inhop when processing is not yet finished", readSpace, inhop);
            }
            advanceCount = readSpace;
        }
        
        for (int c = 0; c < channels; ++c) {
            auto &cd = m_channelData.at(c);
            int written = 0;
            if (resamplingAfter) {
                written = cd->outbuf->write(cd->resampled.data(), writeCount);
            } else {
                written = cd->outbuf->write(cd->mixdown.data(), writeCount);
            }
            if (written != writeCount) {
                m_log.log(0, "R3Stretcher: WARNING: too few samples written to output buffer", written, writeCount);
            }
            int skipped = cd->inbuf->skip(advanceCount);
            if (skipped != advanceCount) {
                m_log.log(0, "R3Stretcher: WARNING: too few samples advanced", skipped, advanceCount);
            }
        }

        m_consumedInputDuration += advanceCount;
        m_totalOutputDuration += writeCount;
        
        if (m_startSkip > 0) {
            int rs = cd0->outbuf->getReadSpace();
            int toSkip = std::min(m_startSkip, rs);
            for (int c = 0; c < channels; ++c) {
                int skipped = m_channelData.at(c)->outbuf->skip(toSkip);
                if (skipped != toSkip) {
                    m_log.log(0, "R3Stretcher: WARNING: too few samples skipped at output", skipped, toSkip);
                }
            }
            m_startSkip -= toSkip;
            m_totalOutputDuration = rs - toSkip;
        }
        
        m_prevInhop = inhop;
        m_prevOuthop = outhop;
    }

    m_log.log(2, "consume: write space reduced to", cd0->outbuf->getWriteSpace());
}

void
R3Stretcher::analyseChannel(int c, int inhop, int prevInhop, int prevOuthop)
{
    Profiler profiler("R3Stretcher::analyseChannel");
    
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

    if (m_parameters.options & RubberBandStretcher::OptionFormantPreserved) {
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

    double ratio = getEffectiveRatio();

    if (fabs(ratio - 1.0) < 1.0e-7) {
        ++m_unityCount;
    } else {
        m_unityCount = 0;
    }

    bool tighterChannelLock =
        m_parameters.options & RubberBandStretcher::OptionChannelsTogether;

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
                               isRealTime(),
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
                               isRealTime(),
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
R3Stretcher::analyseFormant(int c)
{
    Profiler profiler("R3Stretcher::analyseFormant");

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
R3Stretcher::adjustFormant(int c)
{
    Profiler profiler("R3Stretcher::adjustFormant");

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
R3Stretcher::adjustPreKick(int c)
{
    if (isSingleWindowed()) return;
    
    Profiler profiler("R3Stretcher::adjustPreKick");

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
R3Stretcher::synthesiseChannel(int c, int outhop, bool draining)
{
    Profiler profiler("R3Stretcher::synthesiseChannel");
    
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

