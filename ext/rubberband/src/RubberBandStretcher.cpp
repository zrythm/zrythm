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

#include "faster/R2Stretcher.h"
#include "finer/R3Stretcher.h"

#include <iostream>

namespace RubberBand {

class RubberBandStretcher::Impl
{
    R2Stretcher *m_r2;
    R3Stretcher *m_r3;

    class CerrLogger : public RubberBandStretcher::Logger {
    public:
        void log(const char *message) override {
            std::cerr << "RubberBand: " << message << "\n";
        }
        void log(const char *message, double arg0) override {
            auto prec = std::cerr.precision();
            std::cerr.precision(10);
            std::cerr << "RubberBand: " << message << ": " << arg0 << "\n";
            std::cerr.precision(prec);
        }
        void log(const char *message, double arg0, double arg1) override {
            auto prec = std::cerr.precision();
            std::cerr.precision(10);
            std::cerr << "RubberBand: " << message
                      << ": (" << arg0 << ", " << arg1 << ")" << "\n";
            std::cerr.precision(prec);
        }
    };

    Log makeRBLog(std::shared_ptr<RubberBandStretcher::Logger> logger) {
        if (logger) {
            return Log(
                [=](const char *message) {
                    logger->log(message);
                },
                [=](const char *message, double arg0) {
                    logger->log(message, arg0);
                },
                [=](const char *message, double arg0, double arg1) {
                    logger->log(message, arg0, arg1);
                }
                );
        } else {
            return makeRBLog(std::shared_ptr<RubberBandStretcher::Logger>
                             (new CerrLogger()));
        }
    }

public:
    Impl(size_t sampleRate, size_t channels, Options options,
         std::shared_ptr<RubberBandStretcher::Logger> logger,
         double initialTimeRatio, double initialPitchScale) :
        m_r2 (!(options & OptionEngineFiner) ?
              new R2Stretcher(sampleRate, channels, options,
                              initialTimeRatio, initialPitchScale,
                              makeRBLog(logger))
              : nullptr),
        m_r3 ((options & OptionEngineFiner) ?
              new R3Stretcher(R3Stretcher::Parameters
                              (double(sampleRate), channels, options),
                              initialTimeRatio, initialPitchScale,
                              makeRBLog(logger))
              : nullptr)
    {
    }

    ~Impl()
    {
        delete m_r2;
        delete m_r3;
    }

    int getEngineVersion() const
    {
        if (m_r3) return 3;
        else return 2;
    }
    
    void reset()
    {
        if (m_r2) m_r2->reset();
        else m_r3->reset();
    }

    RTENTRY__
    void
    setTimeRatio(double ratio)
    {
        if (m_r2) m_r2->setTimeRatio(ratio);
        else m_r3->setTimeRatio(ratio);
    }

    RTENTRY__
    void
    setPitchScale(double scale)
    {
        if (m_r2) m_r2->setPitchScale(scale);
        else m_r3->setPitchScale(scale);
    }

    RTENTRY__
    void
    setFormantScale(double scale)
    {
        //!!!
        if (m_r3) m_r3->setFormantScale(scale);
    }

    RTENTRY__
    double
    getTimeRatio() const
    {
        if (m_r2) return m_r2->getTimeRatio();
        else return m_r3->getTimeRatio();
    }

    RTENTRY__
    double
    getPitchScale() const
    {
        if (m_r2) return m_r2->getPitchScale();
        else return m_r3->getPitchScale();
    }

    RTENTRY__
    double
    getFormantScale() const
    {
        //!!!
        if (m_r2) return 0.0;
        else return m_r3->getFormantScale();
    }

    RTENTRY__
    size_t
    getPreferredStartPad() const
    {
        if (m_r2) return m_r2->getPreferredStartPad();
        else return m_r3->getPreferredStartPad();
    }

    RTENTRY__
    size_t
    getStartDelay() const
    {
        if (m_r2) return m_r2->getStartDelay();
        else return m_r3->getStartDelay();
    }

//!!! review all these

    RTENTRY__
    void
    setTransientsOption(Options options) 
    {
        if (m_r2) m_r2->setTransientsOption(options);
    }

    RTENTRY__
    void
    setDetectorOption(Options options) 
    {
        if (m_r2) m_r2->setDetectorOption(options);
    }

    RTENTRY__
    void
    setPhaseOption(Options options) 
    {
        if (m_r2) m_r2->setPhaseOption(options);
    }

    RTENTRY__
    void
    setFormantOption(Options options)
    {
        if (m_r2) m_r2->setFormantOption(options);
        else if (m_r3) m_r3->setFormantOption(options);
    }

    RTENTRY__
    void
    setPitchOption(Options options)
    {
        if (m_r2) m_r2->setPitchOption(options);
        else if (m_r3) m_r3->setPitchOption(options);
    }

    void
    setExpectedInputDuration(size_t samples) 
    {
        if (m_r2) m_r2->setExpectedInputDuration(samples);
        else m_r3->setExpectedInputDuration(samples);
    }

    void
    setMaxProcessSize(size_t samples)
    {
        if (m_r2) m_r2->setMaxProcessSize(samples);
        else m_r3->setMaxProcessSize(samples);
    }

    size_t
    getProcessSizeLimit() const
    {
        if (m_r2) return m_r2->getProcessSizeLimit();
        else return m_r3->getProcessSizeLimit();
    }

    void
    setKeyFrameMap(const std::map<size_t, size_t> &mapping)
    {
        if (m_r2) m_r2->setKeyFrameMap(mapping);
        else m_r3->setKeyFrameMap(mapping);
    }

    RTENTRY__
    size_t
    getSamplesRequired() const
    {
        if (m_r2) return m_r2->getSamplesRequired();
        else return m_r3->getSamplesRequired();
    }

    void
    study(const float *const *input, size_t samples,
          bool final)
    {
        if (m_r2) m_r2->study(input, samples, final);
        else m_r3->study(input, samples, final);
    }

    RTENTRY__
    void
    process(const float *const *input, size_t samples,
            bool final)
    {
        if (m_r2) m_r2->process(input, samples, final);
        else m_r3->process(input, samples, final);
    }

    RTENTRY__
    int
    available() const
    {
        if (m_r2) return m_r2->available();
        else return m_r3->available();
    }

    RTENTRY__
    size_t
    retrieve(float *const *output, size_t samples) const
    {
        if (m_r2) return m_r2->retrieve(output, samples);
        else return m_r3->retrieve(output, samples);
    }

    float
    getFrequencyCutoff(int n) const
    {
        if (m_r2) return m_r2->getFrequencyCutoff(n);
        else return {};
    }

    void
    setFrequencyCutoff(int n, float f) 
    {
        if (m_r2) m_r2->setFrequencyCutoff(n, f);
    }

    size_t
    getInputIncrement() const
    {
        if (m_r2) return m_r2->getInputIncrement();
        else return {};
    }

    std::vector<int>
    getOutputIncrements() const
    {
        if (m_r2) return m_r2->getOutputIncrements();
        else return {};
    }

    std::vector<float>
    getPhaseResetCurve() const
    {
        if (m_r2) return m_r2->getPhaseResetCurve();
        else return {};
    }

    std::vector<int>
    getExactTimePoints() const
    {
        if (m_r2) return m_r2->getExactTimePoints();
        else return {};
    }

    RTENTRY__
    size_t
    getChannelCount() const
    {
        if (m_r2) return m_r2->getChannelCount();
        else return m_r3->getChannelCount();
    }

    void
    calculateStretch()
    {
        if (m_r2) m_r2->calculateStretch();
    }

    void
    setDebugLevel(int level)
    {
        if (m_r2) m_r2->setDebugLevel(level);
        else m_r3->setDebugLevel(level);
    }

    static void
    setDefaultDebugLevel(int level)
    {
        Log::setDefaultDebugLevel(level);
    }
};

RubberBandStretcher::RubberBandStretcher(size_t sampleRate,
                                         size_t channels,
                                         Options options,
                                         double initialTimeRatio,
                                         double initialPitchScale) :
    m_d(new Impl(sampleRate, channels, options, nullptr,
                 initialTimeRatio, initialPitchScale))
{
}

RubberBandStretcher::RubberBandStretcher(size_t sampleRate,
                                         size_t channels,
                                         std::shared_ptr<Logger> logger,
                                         Options options,
                                         double initialTimeRatio,
                                         double initialPitchScale) :
    m_d(new Impl(sampleRate, channels, options, logger,
                 initialTimeRatio, initialPitchScale))
{
}

RubberBandStretcher::~RubberBandStretcher()
{
    delete m_d;
}

void
RubberBandStretcher::reset()
{
    m_d->reset();
}

int
RubberBandStretcher::getEngineVersion() const
{
    return m_d->getEngineVersion();
}

RTENTRY__
void
RubberBandStretcher::setTimeRatio(double ratio)
{
    m_d->setTimeRatio(ratio);
}

RTENTRY__
void
RubberBandStretcher::setPitchScale(double scale)
{
    m_d->setPitchScale(scale);
}

RTENTRY__
void
RubberBandStretcher::setFormantScale(double scale)
{
    m_d->setFormantScale(scale);
}

RTENTRY__
double
RubberBandStretcher::getTimeRatio() const
{
    return m_d->getTimeRatio();
}

RTENTRY__
double
RubberBandStretcher::getPitchScale() const
{
    return m_d->getPitchScale();
}

RTENTRY__
double
RubberBandStretcher::getFormantScale() const
{
    return m_d->getFormantScale();
}

RTENTRY__
size_t
RubberBandStretcher::getPreferredStartPad() const
{
    return m_d->getPreferredStartPad();
}

RTENTRY__
size_t
RubberBandStretcher::getStartDelay() const
{
    return m_d->getStartDelay();
}

RTENTRY__
size_t
RubberBandStretcher::getLatency() const
{
    // deprecated alias for getStartDelay
    return m_d->getStartDelay();
}

RTENTRY__
void
RubberBandStretcher::setTransientsOption(Options options) 
{
    m_d->setTransientsOption(options);
}

RTENTRY__
void
RubberBandStretcher::setDetectorOption(Options options) 
{
    m_d->setDetectorOption(options);
}

RTENTRY__
void
RubberBandStretcher::setPhaseOption(Options options) 
{
    m_d->setPhaseOption(options);
}

RTENTRY__
void
RubberBandStretcher::setFormantOption(Options options)
{
    m_d->setFormantOption(options);
}

RTENTRY__
void
RubberBandStretcher::setPitchOption(Options options)
{
    m_d->setPitchOption(options);
}

void
RubberBandStretcher::setExpectedInputDuration(size_t samples) 
{
    m_d->setExpectedInputDuration(samples);
}

void
RubberBandStretcher::setMaxProcessSize(size_t samples)
{
    m_d->setMaxProcessSize(samples);
}

size_t
RubberBandStretcher::getProcessSizeLimit() const
{
    return m_d->getProcessSizeLimit();
}

void
RubberBandStretcher::setKeyFrameMap(const std::map<size_t, size_t> &mapping)
{
    m_d->setKeyFrameMap(mapping);
}

RTENTRY__
size_t
RubberBandStretcher::getSamplesRequired() const
{
    return m_d->getSamplesRequired();
}

void
RubberBandStretcher::study(const float *const *input, size_t samples,
                           bool final)
{
    m_d->study(input, samples, final);
}

RTENTRY__
void
RubberBandStretcher::process(const float *const *input, size_t samples,
                             bool final)
{
    m_d->process(input, samples, final);
}

RTENTRY__
int
RubberBandStretcher::available() const
{
    return m_d->available();
}

RTENTRY__
size_t
RubberBandStretcher::retrieve(float *const *output, size_t samples) const
{
    return m_d->retrieve(output, samples);
}

float
RubberBandStretcher::getFrequencyCutoff(int n) const
{
    return m_d->getFrequencyCutoff(n);
}

void
RubberBandStretcher::setFrequencyCutoff(int n, float f) 
{
    m_d->setFrequencyCutoff(n, f);
}

size_t
RubberBandStretcher::getInputIncrement() const
{
    return m_d->getInputIncrement();
}

std::vector<int>
RubberBandStretcher::getOutputIncrements() const
{
    return m_d->getOutputIncrements();
}

std::vector<float>
RubberBandStretcher::getPhaseResetCurve() const
{
    return m_d->getPhaseResetCurve();
}

std::vector<int>
RubberBandStretcher::getExactTimePoints() const
{
    return m_d->getExactTimePoints();
}

RTENTRY__
size_t
RubberBandStretcher::getChannelCount() const
{
    return m_d->getChannelCount();
}

void
RubberBandStretcher::calculateStretch()
{
    m_d->calculateStretch();
}

void
RubberBandStretcher::setDebugLevel(int level)
{
    m_d->setDebugLevel(level);
}

void
RubberBandStretcher::setDefaultDebugLevel(int level)
{
    Impl::setDefaultDebugLevel(level);
}

}

