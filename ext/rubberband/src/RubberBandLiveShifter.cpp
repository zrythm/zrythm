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
    Rubber Band Live Pitch Shifter obtained by agreement with the copyright
    holders, you may redistribute and/or modify it under the terms
    described in that licence.

    If you wish to distribute code using the Rubber Band Live Pitch Shifter
    under terms other than those of the GNU General Public License,
    you must obtain a valid commercial licence before doing so.
*/

#include "../rubberband/RubberBandLiveShifter.h"
#include "finer/R3LiveShifter.h"

#include <iostream>

namespace RubberBand {

class RubberBandLiveShifter::Impl
{
    R3LiveShifter *m_s;

    class CerrLogger : public RubberBandLiveShifter::Logger {
    public:
        void log(const char *message) override {
            std::cerr << "RubberBandLive: " << message << "\n";
        }
        void log(const char *message, double arg0) override {
            auto prec = std::cerr.precision();
            std::cerr.precision(10);
            std::cerr << "RubberBandLive: " << message << ": " << arg0 << "\n";
            std::cerr.precision(prec);
        }
        void log(const char *message, double arg0, double arg1) override {
            auto prec = std::cerr.precision();
            std::cerr.precision(10);
            std::cerr << "RubberBandLive: " << message
                      << ": (" << arg0 << ", " << arg1 << ")" << "\n";
            std::cerr.precision(prec);
        }
    };
    
    Log makeRBLog(std::shared_ptr<RubberBandLiveShifter::Logger> logger) {
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
            return makeRBLog(std::shared_ptr<RubberBandLiveShifter::Logger>
                             (new CerrLogger()));
        }
    }

public:
    Impl(size_t sampleRate, size_t channels,
         std::shared_ptr<RubberBandLiveShifter::Logger> logger,
         RubberBandLiveShifter::Options options) :
        m_s (new R3LiveShifter
             (R3LiveShifter::Parameters(double(sampleRate), channels,
                                        options),
              makeRBLog(logger)))
    {
    }

    ~Impl()
    {
        delete m_s;
    }

    void reset()
    {
        m_s->reset();
    }

    RTENTRY__
    void
    setPitchScale(double scale)
    {
        m_s->setPitchScale(scale);
    }

    RTENTRY__
    void
    setFormantScale(double scale)
    {
        m_s->setFormantScale(scale);
    }

    RTENTRY__
    double
    getPitchScale() const
    {
        return m_s->getPitchScale();
    }

    RTENTRY__
    double
    getFormantScale() const
    {
        return m_s->getFormantScale();
    }

    RTENTRY__
    void
    setFormantOption(RubberBandLiveShifter::Options options)
    {
        m_s->setFormantOption(options);
    }
    
    RTENTRY__
    size_t
    getStartDelay() const
    {
        return m_s->getStartDelay();
    }

    RTENTRY__
    size_t
    getBlockSize() const
    {
        return m_s->getBlockSize();
    }

    RTENTRY__
    void
    shift(const float *const *input, float *const *output)
    {
        m_s->shift(input, output);
    }

    RTENTRY__
    size_t
    getChannelCount() const
    {
        return m_s->getChannelCount();
    }

    void
    setDebugLevel(int level)
    {
        m_s->setDebugLevel(level);
    }

    static void
    setDefaultDebugLevel(int level)
    {
        Log::setDefaultDebugLevel(level);
    }
};

RubberBandLiveShifter::RubberBandLiveShifter(size_t sampleRate,
                                             size_t channels,
                                             Options options) :
    m_d(new Impl(sampleRate, channels, nullptr, options))
{
}

RubberBandLiveShifter::RubberBandLiveShifter(size_t sampleRate,
                                             size_t channels,
                                             std::shared_ptr<Logger> logger,
                                             Options options) :
    m_d(new Impl(sampleRate, channels, logger, options))
{
}

RubberBandLiveShifter::~RubberBandLiveShifter()
{
    delete m_d;
}

void
RubberBandLiveShifter::reset()
{
    m_d->reset();
}

RTENTRY__
void
RubberBandLiveShifter::setPitchScale(double scale)
{
    m_d->setPitchScale(scale);
}

RTENTRY__
void
RubberBandLiveShifter::setFormantScale(double scale)
{
    m_d->setFormantScale(scale);
}

RTENTRY__
double
RubberBandLiveShifter::getPitchScale() const
{
    return m_d->getPitchScale();
}

RTENTRY__
double
RubberBandLiveShifter::getFormantScale() const
{
    return m_d->getFormantScale();
}

RTENTRY__
size_t
RubberBandLiveShifter::getStartDelay() const
{
    return m_d->getStartDelay();
}

RTENTRY__
void
RubberBandLiveShifter::setFormantOption(Options options)
{
    m_d->setFormantOption(options);
}

RTENTRY__
size_t
RubberBandLiveShifter::getBlockSize() const
{
    return m_d->getBlockSize();
}

RTENTRY__
void
RubberBandLiveShifter::shift(const float *const *input, float *const *output)
{
    m_d->shift(input, output);
}

RTENTRY__
size_t
RubberBandLiveShifter::getChannelCount() const
{
    return m_d->getChannelCount();
}

void
RubberBandLiveShifter::setDebugLevel(int level)
{
    m_d->setDebugLevel(level);
}

void
RubberBandLiveShifter::setDefaultDebugLevel(int level)
{
    Impl::setDefaultDebugLevel(level);
}

}

