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

#ifndef RUBBERBAND_R3_LIVE_SHIFTERIMPL_H
#define RUBBERBAND_R3_LIVE_SHIFTERIMPL_H

#include "BinSegmenter.h"
#include "Guide.h"
#include "Peak.h"
#include "PhaseAdvance.h"

#include "../common/Resampler.h"
#include "../common/FFT.h"
#include "../common/FixedVector.h"
#include "../common/Allocators.h"
#include "../common/Window.h"
#include "../common/VectorOpsComplex.h"
#include "../common/Log.h"

#include "../../rubberband/RubberBandLiveShifter.h"

#include <map>
#include <memory>
#include <atomic>

namespace RubberBand
{

class R3LiveShifter
{
public:
    struct Parameters {
        double sampleRate;
        int channels;
        RubberBandLiveShifter::Options options;
        Parameters(double _sampleRate, int _channels,
                   RubberBandLiveShifter::Options _options) :
            sampleRate(_sampleRate), channels(_channels), options(_options) { }
    };
    
    R3LiveShifter(Parameters parameters, Log log);
    ~R3LiveShifter() { }

    void reset();
    
    void setPitchScale(double scale);
    void setFormantScale(double scale);

    double getPitchScale() const;
    double getFormantScale() const;

    void setFormantOption(RubberBandLiveShifter::Options);

    size_t getBlockSize() const;
    void shift(const float *const *input, float *const *output);

    size_t getPreferredStartPad() const;
    size_t getStartDelay() const;
    
    size_t getChannelCount() const;
    
    void setDebugLevel(int level) {
        m_log.setDebugLevel(level);
        for (auto &sd : m_scaleData) {
            sd.second->guided.setDebugLevel(level);
        }
        m_guide.setDebugLevel(level);
    }

protected:
    struct Limits {
        int minPreferredOuthop;
        int maxPreferredOuthop;
        int minInhop;
        int maxInhopWithReadahead;
        int maxInhop;
        Limits(RubberBandLiveShifter::Options, double rate) :
            // commented values are results when rate = 44100 or 48000
            minInhop(1)
        {
            minPreferredOuthop = roundUpDiv(rate, 256); // 256
            maxPreferredOuthop = (roundUpDiv(rate, 128) * 5) / 4; // 640
            maxInhopWithReadahead = roundUpDiv(rate, 128); // 512
            maxInhop = (roundUpDiv(rate, 64) * 3) / 2; // 1536
        }
    };
    
    struct ClassificationReadaheadData {
        FixedVector<process_t> timeDomain;
        FixedVector<process_t> mag;
        FixedVector<process_t> phase;
        ClassificationReadaheadData(int _fftSize) :
            timeDomain(_fftSize, 0.f),
            mag(_fftSize/2 + 1, 0.f),
            phase(_fftSize/2 + 1, 0.f)
        { }

    private:
        ClassificationReadaheadData(const ClassificationReadaheadData &) =delete;
        ClassificationReadaheadData &operator=(const ClassificationReadaheadData &) =delete;
    };
    
    struct ChannelScaleData {
        int fftSize;
        int bufSize; // size of every freq-domain array here: fftSize/2 + 1
        FixedVector<process_t> timeDomain;
        FixedVector<process_t> real;
        FixedVector<process_t> imag;
        FixedVector<process_t> mag;
        FixedVector<process_t> phase;
        FixedVector<process_t> advancedPhase;
        FixedVector<process_t> prevMag;
        FixedVector<process_t> pendingKick;
        FixedVector<process_t> accumulator;
        int accumulatorFill;

        ChannelScaleData(int _fftSize, int _longestFftSize) :
            fftSize(_fftSize),
            bufSize(fftSize/2 + 1),
            timeDomain(fftSize, 0.f),
            real(bufSize, 0.f),
            imag(bufSize, 0.f),
            mag(bufSize, 0.f),
            phase(bufSize, 0.f),
            advancedPhase(bufSize, 0.f),
            prevMag(bufSize, 0.f),
            pendingKick(bufSize, 0.f),
            accumulator(_longestFftSize, 0.f),
            accumulatorFill(0)
        { }

        void reset() {
            v_zero(prevMag.data(), prevMag.size());
            v_zero(pendingKick.data(), pendingKick.size());
            v_zero(accumulator.data(), accumulator.size());
            accumulatorFill = 0;
        }

    private:
        ChannelScaleData(const ChannelScaleData &) =delete;
        ChannelScaleData &operator=(const ChannelScaleData &) =delete;
    };

    struct FormantData {
        int fftSize;
        FixedVector<process_t> cepstra;
        FixedVector<process_t> envelope;
        FixedVector<process_t> spare;

        FormantData(int _fftSize) :
            fftSize(_fftSize),
            cepstra(_fftSize, 0.0),
            envelope(_fftSize/2 + 1, 0.0),
            spare(_fftSize/2 + 1, 0.0) { }

        process_t envelopeAt(process_t bin) const {
            int b0 = int(floor(bin)), b1 = int(ceil(bin));
            if (b0 < 0 || b0 > fftSize/2) {
                return 0.0;
            } else if (b1 == b0 || b1 > fftSize/2) {
                return envelope.at(b0);
            } else {
                process_t diff = bin - process_t(b0);
                return envelope.at(b0) * (1.0 - diff) + envelope.at(b1) * diff;
            }
        }
    };

    struct ChannelData {
        std::map<int, std::shared_ptr<ChannelScaleData>> scales;
        FixedVector<process_t> windowSource;
        ClassificationReadaheadData readahead;
        bool haveReadahead;
        std::unique_ptr<BinClassifier> classifier;
        FixedVector<BinClassifier::Classification> classification;
        FixedVector<BinClassifier::Classification> nextClassification;
        std::unique_ptr<BinSegmenter> segmenter;
        BinSegmenter::Segmentation segmentation;
        BinSegmenter::Segmentation prevSegmentation;
        BinSegmenter::Segmentation nextSegmentation;
        Guide::Guidance guidance;
        FixedVector<float> mixdown;
        FixedVector<float> resampled;
        std::unique_ptr<RingBuffer<float>> inbuf;
        std::unique_ptr<RingBuffer<float>> outbuf;
        std::unique_ptr<FormantData> formant;
        ChannelData(BinSegmenter::Parameters segmenterParameters,
                    BinClassifier::Parameters classifierParameters,
                    int longestFftSize,
                    int windowSourceSize,
                    int inRingBufferSize,
                    int outRingBufferSize) :
            scales(),
            windowSource(windowSourceSize, 0.0),
            readahead(segmenterParameters.fftSize),
            haveReadahead(false),
            classifier(new BinClassifier(classifierParameters)),
            classification(classifierParameters.binCount,
                           BinClassifier::Classification::Residual),
            nextClassification(classifierParameters.binCount,
                               BinClassifier::Classification::Residual),
            segmenter(new BinSegmenter(segmenterParameters)),
            segmentation(), prevSegmentation(), nextSegmentation(),
            mixdown(longestFftSize, 0.f),
            resampled(outRingBufferSize, 0.f),
            inbuf(new RingBuffer<float>(inRingBufferSize)),
            outbuf(new RingBuffer<float>(outRingBufferSize)),
            formant(new FormantData(segmenterParameters.fftSize)) { }
        void reset() {
            haveReadahead = false;
            classifier->reset();
            segmentation = BinSegmenter::Segmentation();
            prevSegmentation = BinSegmenter::Segmentation();
            nextSegmentation = BinSegmenter::Segmentation();
            for (size_t i = 0; i < nextClassification.size(); ++i) {
                nextClassification[i] = BinClassifier::Classification::Residual;
            }
            inbuf->reset();
            outbuf->reset();
            for (auto &s : scales) {
                s.second->reset();
            }
        }
    };

    struct ChannelAssembly {
        // Vectors of bare pointers, used to package container data
        // from different channels into arguments for PhaseAdvance
        FixedVector<const float *> input;
        FixedVector<process_t *> mag;
        FixedVector<process_t *> phase;
        FixedVector<process_t *> prevMag;
        FixedVector<Guide::Guidance *> guidance;
        FixedVector<process_t *> outPhase;
        FixedVector<float *> mixdown;
        FixedVector<float *> resampled;
        ChannelAssembly(int channels) :
            input(channels, nullptr),
            mag(channels, nullptr), phase(channels, nullptr),
            prevMag(channels, nullptr), guidance(channels, nullptr),
            outPhase(channels, nullptr), mixdown(channels, nullptr),
            resampled(channels, nullptr) { }
    };

    struct ScaleData {
        int fftSize;
        bool singleWindowMode;
        FFT fft;
        Window<process_t> analysisWindow;
        Window<process_t> synthesisWindow;
        process_t windowScaleFactor;
        GuidedPhaseAdvance guided;

        ScaleData(GuidedPhaseAdvance::Parameters guidedParameters,
                  Log log) :
            fftSize(guidedParameters.fftSize),
            singleWindowMode(guidedParameters.singleWindowMode),
            fft(fftSize),
            analysisWindow(analysisWindowShape(),
                           analysisWindowLength()),
            synthesisWindow(synthesisWindowShape(),
                            synthesisWindowLength()),
            windowScaleFactor(0.0),
            guided(guidedParameters, log)
        {
            int asz = analysisWindow.getSize(), ssz = synthesisWindow.getSize();
            int off = (asz - ssz) / 2;
            for (int i = 0; i < ssz; ++i) {
                windowScaleFactor += analysisWindow.getValue(i + off) *
                    synthesisWindow.getValue(i);
            }
        }

        WindowType analysisWindowShape();
        int analysisWindowLength();
        WindowType synthesisWindowShape();
        int synthesisWindowLength();
    };
    
    Log m_log;
    Parameters m_parameters;
    const Limits m_limits;

    std::atomic<double> m_pitchScale;
    std::atomic<double> m_formantScale;
    
    std::vector<std::shared_ptr<ChannelData>> m_channelData;
    std::map<int, std::shared_ptr<ScaleData>> m_scaleData;
    Guide m_guide;
    Guide::Configuration m_guideConfiguration;
    ChannelAssembly m_channelAssembly;
    std::unique_ptr<Resampler> m_inResampler;
    std::unique_ptr<Resampler> m_outResampler;
    std::pair<int, int> m_initialResamplerDelays;
    bool m_useReadahead;
    int m_prevInhop;
    int m_prevOuthop;
    bool m_firstProcess;
    uint32_t m_unityCount;

    void initialise();

    void readIn(const float *const *input);
    void generate(int required);
    int readOut(float *const *output, int outcount);
    
    void createResamplers();
    void measureResamplerDelay();
    void analyseChannel(int channel, int inhop, int prevInhop, int prevOuthop);
    void analyseFormant(int channel);
    void adjustFormant(int channel);
    void adjustPreKick(int channel);
    void synthesiseChannel(int channel, int outhop, bool draining);

    struct ToPolarSpec {
        int magFromBin;
        int magBinCount;
        int polarFromBin;
        int polarBinCount;
    };

    Parameters validateSampleRate(const Parameters &params) {
        Parameters validated { params };
        double minRate = 8000.0, maxRate = 192000.0;
        if (params.sampleRate < minRate) {
            m_log.log(0, "R3LiveShifter: WARNING: Unsupported sample rate", params.sampleRate);
            m_log.log(0, "R3LiveShifter: Minimum rate is", minRate);
            validated.sampleRate = minRate;
        } else if (params.sampleRate > maxRate) {
            m_log.log(0, "R3LiveShifter: WARNING: Unsupported sample rate", params.sampleRate);
            m_log.log(0, "R3LiveShifter: Maximum rate is", maxRate);
            validated.sampleRate = maxRate;
        }
        return validated;
    }
    
    void convertToPolar(process_t *mag, process_t *phase,
                        const process_t *real, const process_t *imag,
                        const ToPolarSpec &s) const {
        v_cartesian_to_polar(mag + s.polarFromBin,
                             phase + s.polarFromBin,
                             real + s.polarFromBin,
                             imag + s.polarFromBin,
                             s.polarBinCount);
        if (s.magFromBin < s.polarFromBin) {
            v_cartesian_to_magnitudes(mag + s.magFromBin,
                                      real + s.magFromBin,
                                      imag + s.magFromBin,
                                      s.polarFromBin - s.magFromBin);
        }
        if (s.magFromBin + s.magBinCount > s.polarFromBin + s.polarBinCount) {
            v_cartesian_to_magnitudes(mag + s.polarFromBin + s.polarBinCount,
                                      real + s.polarFromBin + s.polarBinCount,
                                      imag + s.polarFromBin + s.polarBinCount,
                                      s.magFromBin + s.magBinCount -
                                      s.polarFromBin - s.polarBinCount);
        }
    }

    bool useMidSide() const {
        return m_parameters.channels == 2 &&
            (m_parameters.options &
             RubberBandLiveShifter::OptionChannelsTogether);
    }
    
    bool isSingleWindowed() const {
        return true;
    }

    double getInRatio() const {
        if (m_pitchScale > 1.0) {
            return 1.0 / m_pitchScale;
        } else {
            return 1.0;
        }
    }

    double getOutRatio() const {
        if (m_pitchScale < 1.0) {
            return 1.0 / m_pitchScale;
        } else {
            return 1.0;
        }
    }
    
    int getWindowSourceSize() const {
        if (m_useReadahead) {
            int sz = m_guideConfiguration.classificationFftSize +
                m_limits.maxInhopWithReadahead;
            if (m_guideConfiguration.longestFftSize > sz) {
                return m_guideConfiguration.longestFftSize;
            } else {
                return sz;
            }
        } else {
            return m_guideConfiguration.longestFftSize;
        }
    }
};

}

#endif
