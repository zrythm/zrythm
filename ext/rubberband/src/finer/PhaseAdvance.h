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

#ifndef RUBBERBAND_PHASE_ADVANCE_H
#define RUBBERBAND_PHASE_ADVANCE_H

#include "Guide.h"

#include "../common/Log.h"
#include "../common/mathmisc.h"
#include "../common/Profiler.h"

#include <sstream>
#include <functional>

namespace RubberBand
{

class GuidedPhaseAdvance
{
public:
    struct Parameters {
        int fftSize;
        double sampleRate;
        int channels;
        bool singleWindowMode;
        Parameters(int _fftSize, double _sampleRate, int _channels,
                   bool _singleWindow) :
            fftSize(_fftSize), sampleRate(_sampleRate), channels(_channels),
            singleWindowMode(_singleWindow) { }
    };
    
    GuidedPhaseAdvance(Parameters parameters, Log log) :
        m_parameters(parameters),
        m_log(log),
        m_binCount(parameters.fftSize / 2 + 1),
        m_peakPicker(m_binCount),
        m_reported(false) {
        int ch = m_parameters.channels;
        m_currentPeaks = allocate_and_zero_channels<int>(ch, m_binCount);
        m_prevPeaks = allocate_and_zero_channels<int>(ch, m_binCount);
        m_greatestChannel = allocate_and_zero<int>(m_binCount);
        m_prevInPhase = allocate_and_zero_channels<process_t>(ch, m_binCount);
        m_prevOutPhase = allocate_and_zero_channels<process_t>(ch, m_binCount);
        m_unlocked = allocate_and_zero_channels<process_t>(ch, m_binCount);

        for (int c = 0; c < ch; ++c) {
            for (int i = 0; i < m_binCount; ++i) {
                m_prevPeaks[c][i] = i;
            }
        }
    }

    ~GuidedPhaseAdvance() {
        int ch = m_parameters.channels;
        deallocate_channels(m_currentPeaks, ch);
        deallocate_channels(m_prevPeaks, ch);
        deallocate(m_greatestChannel);
        deallocate_channels(m_prevInPhase, ch);
        deallocate_channels(m_prevOutPhase, ch);
        deallocate_channels(m_unlocked, ch);
    }

    void reset() {
        int ch = m_parameters.channels;
        v_zero_channels(m_prevPeaks, ch, m_binCount);
        v_zero_channels(m_prevInPhase, ch, m_binCount);
        v_zero_channels(m_prevOutPhase, ch, m_binCount);
    }
    
    void advance(process_t *const *outPhase,
                 const process_t *const *mag,
                 const process_t *const *phase,
                 const process_t *const *prevMag,
                 const Guide::Configuration &configuration,
                 const Guide::Guidance *const *guidance,
                 bool usingMidSide,
                 int inhop,
                 int outhop) {

        Profiler profiler("GuidedPhaseAdvance::advance");
        
        int myFftBand = 0;
        int bandi = 0;
        for (const auto &fband : guidance[0]->fftBands) {
            if (fband.fftSize == m_parameters.fftSize) {
                myFftBand = bandi;
                break;
            }
            ++bandi;
        }

        int bs = m_parameters.fftSize / 2 + 1;
        int channels = m_parameters.channels;
        double ratio = double(outhop) / double(inhop);

        int lowest = configuration.fftBandLimits[myFftBand].b0min;
        int highest = configuration.fftBandLimits[myFftBand].b1max;
        
        if (m_log.getDebugLevel() > 0 && !m_reported) {
            m_log.log(1, "PhaseAdvance: for fftSize and bins",
                      m_parameters.fftSize, bs);
            m_log.log(1, "PhaseAdvance: channels", channels);
            m_log.log(1, "PhaseAdvance: widest bin range for this size",
                      lowest, highest);
            m_log.log(1, "PhaseAdvance: widest freq range for this size",
                      configuration.fftBandLimits[myFftBand].f0min,
                      configuration.fftBandLimits[myFftBand].f1max);
            m_log.log(1, "PhaseAdvance: initial inhop and outhop",
                      inhop, outhop);
            m_log.log(1, "PhaseAdvance: initial ratio", ratio);
            m_reported = true;
        }
        
        for (int c = 0; c < channels; ++c) {
            for (int i = lowest; i <= highest; ++i) {
                m_currentPeaks[c][i] = i;
            }
            for (int i = 0; i < guidance[c]->phaseLockBandCount; ++i) {
                const auto &band = guidance[c]->phaseLockBands[i];
                int startBin = binForFrequency
                    (band.f0, m_parameters.fftSize, m_parameters.sampleRate);
                int endBin = binForFrequency
                    (band.f1, m_parameters.fftSize, m_parameters.sampleRate);
                if (startBin > highest || endBin < lowest) continue;
                if (endBin > highest) endBin = highest;
                int count = endBin - startBin + 1;
                if (count < 1) continue;
                m_peakPicker.findNearestAndNextPeaks(mag[c],
                                                     startBin, count,
                                                     band.p, m_currentPeaks[c],
                                                     nullptr);
            }
            m_peakPicker.findNearestAndNextPeaks(prevMag[c],
                                                 lowest, highest - lowest + 1,
                                                 1, m_prevPeaks[c],
                                                 nullptr);
        }

        if (channels > 1) {
            for (int i = lowest; i <= highest; ++i) {
                int gc = 0;
                float gmag = mag[0][i];
                for (int c = 1; c < channels; ++c) {
                    if (mag[c][i] > gmag) {
                        gmag = mag[c][i];
                        gc = c;
                    }
                }
                m_greatestChannel[i] = gc;
            }
        } else {
            v_zero(m_greatestChannel, bs);
        }

        process_t omegaFactor = 2.0 * M_PI * process_t(inhop) /
            process_t(m_parameters.fftSize);
        for (int c = 0; c < channels; ++c) {
            for (int i = lowest; i <= highest; ++i) {
                process_t omega = omegaFactor * process_t(i);
                process_t expected = m_prevInPhase[c][i] + omega;
                process_t error = princarg(phase[c][i] - expected);
                process_t advance = ratio * (omega + error);
                m_unlocked[c][i] = m_prevOutPhase[c][i] + advance;
            }
        }

        for (int c = 0; c < channels; ++c) {
            const Guide::Guidance *g = guidance[c];
            int phaseLockBand = 0;
            for (int i = lowest; i <= highest; ++i) {
                process_t f = frequencyForBin
                    (i, m_parameters.fftSize, m_parameters.sampleRate);
                while (f > g->phaseLockBands[phaseLockBand].f1 &&
                       phaseLockBand + 1 < g->phaseLockBandCount) {
                    ++phaseLockBand;
                }
                process_t ph = 0.0;
                if (inRange(f, g->phaseReset) || inRange(f, g->kick)) {
                    ph = phase[c][i];
                } else if (usingMidSide && channels == 2 &&
                           c == 0 && inRange(f, guidance[1]->phaseReset)) {
                    ph = phase[c][i];
                } else if (inhop == outhop) {
                    ph = m_unlocked[c][i];
                } else if (inRange (f, g->highUnlocked)) {
                    ph = m_unlocked[c][i];
                } else {
                    int peak = m_currentPeaks[c][i];
                    int prevPeak = m_prevPeaks[c][peak];
                    int peakCh = c;
                    if (inRange(f, g->channelLock)) {
                        int other = m_greatestChannel[i];
                        if (other != c &&
                            inRange(f, guidance[other]->channelLock)) {
                            int otherPeak = m_currentPeaks[other][i];
                            int otherPrevPeak = m_prevPeaks[other][otherPeak];
                            if (otherPrevPeak == prevPeak) {
                                peakCh = other;
                            }
                        }
                    }
                    process_t peakAdvance =
                        m_unlocked[peakCh][peak] - m_prevOutPhase[peakCh][peak];
                    process_t peakNew =
                        m_prevOutPhase[peakCh][prevPeak] + peakAdvance;
                    process_t diff =
                        process_t(phase[c][i]) - process_t(phase[peakCh][peak]);
                    process_t beta =
                        process_t(g->phaseLockBands[phaseLockBand].beta);
                    ph = peakNew + beta * diff;
                }
                outPhase[c][i] = princarg(ph);
            }
        }
                
        for (int c = 0; c < channels; ++c) {
            for (int i = lowest; i <= highest; ++i) {
                m_prevInPhase[c][i] = phase[c][i];
            }
            for (int i = lowest; i <= highest; ++i) {
                m_prevOutPhase[c][i] = outPhase[c][i];
            }
        }
    }

    void setDebugLevel(int debugLevel) {
        m_log.setDebugLevel(debugLevel);
    }
    
protected:
    Parameters m_parameters;
    Log m_log;
    int m_binCount;
    Peak<process_t> m_peakPicker;
    int **m_currentPeaks;
    int **m_prevPeaks;
    int *m_greatestChannel;
    process_t **m_prevInPhase;
    process_t **m_prevOutPhase;
    process_t **m_unlocked;
    bool m_reported;

    bool inRange(process_t f, const Guide::Range &r) {
        return r.present && f >= r.f0 && f < r.f1;
    }

    GuidedPhaseAdvance(const GuidedPhaseAdvance &) =delete;
    GuidedPhaseAdvance &operator=(const GuidedPhaseAdvance &) =delete;
};

}

#endif
