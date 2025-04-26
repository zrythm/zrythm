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

#ifndef RUBBERBAND_GUIDE_H
#define RUBBERBAND_GUIDE_H

#include "../common/Log.h"
#include "../common/Profiler.h"

#include <functional>
#include <sstream>

namespace RubberBand
{

class Guide
{
public:
    struct FftBand {
        int fftSize;
        double f0;
        double f1;
        FftBand(int _s, double _f0, double _f1) :
            fftSize(_s), f0(_f0), f1(_f1) { }
        FftBand() :
            fftSize(0), f0(0.f), f1(0.f) { }
    };

    struct PhaseLockBand {
        int p;
        double beta;
        double f0;
        double f1;
        PhaseLockBand(int _p, double _beta, double _f0, double _f1) :
            p(_p), beta(_beta), f0(_f0), f1(_f1) { }
        PhaseLockBand() :
            p(0), beta(1.0), f0(0.f), f1(0.f) { }
    };

    struct Range {
        bool present;
        double f0;
        double f1;
        Range(bool _present, double _f0, double _f1) :
            present(_present), f0(_f0), f1(_f1) { }
        Range() :
            present(false), f0(0.f), f1(0.f) { }
    };
        
    struct Guidance {
        FftBand fftBands[3];
        int fftBandCount;
        PhaseLockBand phaseLockBands[4];
        int phaseLockBandCount;
        Range kick;
        Range preKick;
        Range highUnlocked;
        Range phaseReset;
        Range channelLock;
    };

    struct BandLimits {
        int fftSize;
        double f0min;
        double f1max;
        int b0min;
        int b1max;
        BandLimits(int _fftSize, double _rate, double _f0min, double _f1max) :
            fftSize(_fftSize), f0min(_f0min), f1max(_f1max),
            b0min(int(floor(f0min * fftSize / _rate))),
            b1max(int(ceil(f1max * fftSize / _rate))) { }
        BandLimits() :
            fftSize(0), f0min(0.f), f1max(0.f), b0min(0), b1max(0) { }
    };

    struct Configuration {
        int longestFftSize;
        int shortestFftSize;
        int classificationFftSize;
        BandLimits fftBandLimits[3];
        int fftBandLimitCount;
        Configuration() :
            longestFftSize(0), shortestFftSize(0), classificationFftSize(0),
            fftBandLimitCount(0) { }
    };
    
    struct Parameters {
        double sampleRate;
        bool singleWindowMode;
        Parameters(double _sampleRate, bool _singleWindow) :
            sampleRate(_sampleRate),
            singleWindowMode(_singleWindow) { }
    };

    Guide(Parameters parameters, Log log) :
        m_parameters(parameters),
        m_log(log)
    {
        double rate = m_parameters.sampleRate;
        double nyquist = rate / 2.0;

        m_log.log(1, "Guide: rate and single-window mode",
                  rate, m_parameters.singleWindowMode);

        int classificationFftSize = roundUpDiv(parameters.sampleRate, 32);

        int minClassificationFftSize = 1024;
        if (classificationFftSize < minClassificationFftSize) {
            m_log.log(1, "Guide: sample rate is too low to work well");
            m_log.log(1, "Guide: rounding up classification FFT size from and to", classificationFftSize, minClassificationFftSize);
            classificationFftSize = minClassificationFftSize;
        }
        
        m_configuration.classificationFftSize = classificationFftSize;
        
        m_log.log(1, "Guide: classification FFT size",
                  m_configuration.classificationFftSize);

        if (m_parameters.singleWindowMode) {

            // Single-window mode
            
            m_configuration.longestFftSize = classificationFftSize;
            m_configuration.shortestFftSize = classificationFftSize;

            m_defaultLower = nyquist;
            m_minLower = m_defaultLower;
            m_maxLower = m_defaultLower;

            m_defaultHigher = nyquist;
            m_minHigher = m_defaultHigher;
            m_maxHigher = m_defaultHigher;

            m_configuration.fftBandLimitCount = 1;

            m_configuration.fftBandLimits[0] = 
                BandLimits(classificationFftSize, rate, 0.0, nyquist);

        } else {
            
            // The normal multi-window mode

            m_configuration.longestFftSize = classificationFftSize * 2;
            m_configuration.shortestFftSize = classificationFftSize / 2;
            
            m_defaultLower = 700.0;
            m_minLower = 500.0;
            m_maxLower = 1100.0;

            m_defaultHigher = 4800.0;
            m_minHigher = 4000.0;
            m_maxHigher = 7000.0;
            
            m_configuration.fftBandLimitCount = 3;

            m_configuration.fftBandLimits[0] =
                BandLimits(m_configuration.longestFftSize,
                           rate, 0.0, m_maxLower);
        
            // This is the classification and fallback FFT: we need it
            // to go up to Nyquist so we can seamlessly switch to it
            // for longer stretches, and down to 0.0 so we can use it
            // for unity in offline mode

            m_configuration.fftBandLimits[1] =
                BandLimits(classificationFftSize,
                           rate, 0.0, nyquist);
        
            m_configuration.fftBandLimits[2] =
                BandLimits(m_configuration.shortestFftSize,
                           rate, m_minHigher, nyquist);
        }
    }

    const Configuration &getConfiguration() const {
        return m_configuration;
    }
    
    void updateGuidance(double ratio,
                        int outhop,
                        const process_t *const magnitudes,
                        const process_t *const prevMagnitudes,
                        const process_t *const nextMagnitudes,
                        const BinSegmenter::Segmentation &segmentation,
                        const BinSegmenter::Segmentation &prevSegmentation,
                        const BinSegmenter::Segmentation &nextSegmentation,
                        process_t meanMagnitude,
                        int unityCount,
                        bool realtime,
                        bool tighterChannelLock,
                        bool resetOnSilence,
                        Guidance &guidance) const {

        Profiler profiler("Guide::updateGuidance");
        
        bool hadPhaseReset = guidance.phaseReset.present;

        guidance.phaseReset.present = false;
        guidance.kick.present = false;
        guidance.preKick.present = false;
        guidance.highUnlocked.present = false;
        guidance.channelLock.present = false;

        double nyquist = m_parameters.sampleRate / 2.0;

        if (m_parameters.singleWindowMode) {

            // All the fft and phase-lock bands are fixed in this
            // mode. We'll still need to continue to set up phase
            // reset ranges etc, including the unity case.

            guidance.fftBandCount = 1;
            guidance.fftBands[0].fftSize = m_configuration.classificationFftSize;
            guidance.fftBands[0].f0 = 0.0;
            guidance.fftBands[0].f1 = nyquist;

            guidance.phaseLockBandCount = 3;

            guidance.phaseLockBands[0].p = 1;
            guidance.phaseLockBands[0].beta = betaFor(1200.0, ratio);
            guidance.phaseLockBands[0].f0 = 0.0;
            guidance.phaseLockBands[0].f1 = 1600.0;
            
            guidance.phaseLockBands[1].p = 2;
            guidance.phaseLockBands[1].beta = betaFor(5000.0, ratio);
            guidance.phaseLockBands[1].f0 = 1600.0;
            guidance.phaseLockBands[1].f1 = 7000.0;
            
            guidance.phaseLockBands[2].p = 5;
            guidance.phaseLockBands[2].beta = betaFor(10000.0, ratio);
            guidance.phaseLockBands[2].f0 = 7000.0;
            guidance.phaseLockBands[2].f1 = nyquist;

            if (outhop > 256) {
                guidance.phaseLockBands[2].p = 4;
            }

            for (int i = 0; i < 3; ++i) {
                if (guidance.phaseLockBands[i].f0 > nyquist) {
                    guidance.phaseLockBands[i].f0 = nyquist;
                }
                if (guidance.phaseLockBands[i].f1 > nyquist) {
                    guidance.phaseLockBands[i].f1 = nyquist;
                }
            }
            
        } else {

            // The normal multi-window mode
            
            guidance.fftBandCount = 3;
            guidance.fftBands[0].fftSize = m_configuration.longestFftSize;
            guidance.fftBands[1].fftSize = m_configuration.classificationFftSize;
            guidance.fftBands[2].fftSize = m_configuration.shortestFftSize;
            
            guidance.phaseLockBandCount = 4;
        
            // This is a vital stop case for PhaseAdvance
            guidance.phaseLockBands[3].f1 = nyquist;
        }

        // We've set the counts, and for single-window mode we've set
        // the band ranges as well - in normal multi-window mode we
        // still have to do that, but we should do these first
        
        if (resetOnSilence && meanMagnitude < 1.0e-6) {
            updateForSilence(guidance);
            return;
        }

        if (unityCount > 0) {
            updateForUnity(guidance,
                           hadPhaseReset,
                           segmentation,
                           realtime);
            return;
        }

        guidance.channelLock.present = true;
        guidance.channelLock.f0 = 0.0;

        if (tighterChannelLock) {
            guidance.channelLock.f1 = nyquist;
        } else {
            guidance.channelLock.f1 = 600.0;
        }

        if (!m_parameters.singleWindowMode) {
            
            bool kick =
                (segmentation.percussiveBelow > 40.0) &&
                (prevSegmentation.percussiveBelow < 40.0) &&
                checkPotentialKick(magnitudes, prevMagnitudes);

            bool futureKick = !kick &&
                (nextSegmentation.percussiveBelow > 40.0) &&
                (segmentation.percussiveBelow < 40.0) &&
                checkPotentialKick(nextMagnitudes, magnitudes);

            if (kick) {
                guidance.kick.present = true;
                guidance.kick.f0 = 0.0;
                guidance.kick.f1 = segmentation.percussiveBelow;
            } else if (futureKick) {
                guidance.preKick.present = true;
                guidance.preKick.f0 = 0.0;
                guidance.preKick.f1 = nextSegmentation.percussiveBelow;
            }
        }
        
/*
        std::cout << "d:"
                  << prevSegmentation.percussiveBelow << ","
                  << segmentation.percussiveBelow << ","
                  << nextSegmentation.percussiveBelow << ","
                  << checkPotentialKick(magnitudes, prevMagnitudes) << ","
                  << checkPotentialKick(nextMagnitudes, magnitudes) << ","
                  << (kick ? "K" : "N") << ","
                  << (futureKick ? "F" : "N") << std::endl;
*/        
        
        if (segmentation.residualAbove > segmentation.percussiveAbove) {
            guidance.highUnlocked.present = true;
            guidance.highUnlocked.f0 = segmentation.percussiveAbove;
            guidance.highUnlocked.f1 = segmentation.residualAbove;
        }

        double bigGap = 4000.0;
        if (segmentation.residualAbove >
            segmentation.percussiveAbove + bigGap &&
            prevSegmentation.residualAbove <
            prevSegmentation.percussiveAbove + bigGap) {
            guidance.phaseReset.present = true;
            guidance.phaseReset.f0 = std::min(segmentation.percussiveAbove,
                                              nextSegmentation.percussiveAbove);
            guidance.phaseReset.f1 = std::max(segmentation.residualAbove,
                                              nextSegmentation.residualAbove);
            if (guidance.phaseReset.f0 < 200.0) {
                guidance.phaseReset.f0 = 0.0;
            }
        }

        if (!m_parameters.singleWindowMode) {

            // The normal multi-window mode. For single-window we did
            // this already.
            
            double prevLower = guidance.fftBands[0].f1;
            double lower = descendToValley(prevLower, magnitudes);
            if (lower > m_maxLower || lower < m_minLower) {
                lower = m_defaultLower;
            }
        
            double prevHigher = guidance.fftBands[1].f1;
            double higher = descendToValley(prevHigher, magnitudes);
            if (higher > m_maxHigher || higher < m_minHigher) {
                higher = m_defaultHigher;
            }

            guidance.fftBands[0].f0 = 0.0;
            guidance.fftBands[0].f1 = lower;

            guidance.fftBands[1].f0 = lower;
            guidance.fftBands[1].f1 = higher;
        
            guidance.fftBands[2].f0 = higher;
            guidance.fftBands[2].f1 = nyquist;

            if (outhop > 256) {
                guidance.fftBands[1].f1 = nyquist;
                guidance.fftBands[2].f0 = nyquist;
            }
        
            double mid = std::max(lower, 1600.0);

            guidance.phaseLockBands[0].p = 1;
            guidance.phaseLockBands[0].beta = betaFor(300.0, ratio);
            guidance.phaseLockBands[0].f0 = 0.0;
            guidance.phaseLockBands[0].f1 = lower;
        
            guidance.phaseLockBands[1].p = 2;
            guidance.phaseLockBands[1].beta = betaFor(1600.0, ratio);
            guidance.phaseLockBands[1].f0 = lower;
            guidance.phaseLockBands[1].f1 = mid;
        
            guidance.phaseLockBands[2].p = 3;
            guidance.phaseLockBands[2].beta = betaFor(5000.0, ratio);
            guidance.phaseLockBands[2].f0 = mid;
            guidance.phaseLockBands[2].f1 = higher;
            
            guidance.phaseLockBands[3].p = 4;
            guidance.phaseLockBands[3].beta = betaFor(10000.0, ratio);
            guidance.phaseLockBands[3].f0 = higher;
            guidance.phaseLockBands[3].f1 = nyquist;
        
            if (outhop > 256) {
                guidance.phaseLockBands[3].p = 3;
            }
        }
        
        if (ratio > 2.0) {
            
            // For very long stretches, diffuse is better than
            // metallic - gradually unlock the higher frequencies and
            // reduce the channel lock
            
            double channelLimit = guidance.channelLock.f1;
            channelLimit = channelLimit - (ratio - 2.0) * 150.0;
            if (channelLimit < 100.0) channelLimit = 100.0;
            guidance.channelLock.f1 = channelLimit;
            
            double unlockedAbove = 12000.0 - (ratio - 2.0) * 400.0;
            if (unlockedAbove < channelLimit) unlockedAbove = channelLimit;
            if (guidance.highUnlocked.present) {
                guidance.highUnlocked.f0 = std::min(guidance.highUnlocked.f0,
                                                    unlockedAbove);
            } else {
                guidance.highUnlocked.f0 = unlockedAbove;
            }
            guidance.highUnlocked.f1 = nyquist;
            guidance.highUnlocked.present = true;
        }

/*
        std::ostringstream str;
        str << "Guidance: FFT bands: ["
            << guidance.fftBands[0].fftSize << " from "
            << guidance.fftBands[0].f0 << " to " << guidance.fftBands[0].f1
            << ", "
            << guidance.fftBands[1].fftSize << " from "
            << guidance.fftBands[1].f0 << " to " << guidance.fftBands[1].f1
            << ", "
            << guidance.fftBands[2].fftSize << " from "
            << guidance.fftBands[2].f0 << " to " << guidance.fftBands[2].f1
            << "]; phase reset range: ["
            << guidance.phaseReset.present << " from "
            << guidance.phaseReset.f0 << " to " << guidance.phaseReset.f1
            << "]" << std::endl;

        m_log.log(2, str.str().c_str());
*/
    }

    void setDebugLevel(int level) {
        m_log.setDebugLevel(level);
    }
    
protected:
    Parameters m_parameters;
    Log m_log;
    Configuration m_configuration;

    double m_minLower;
    double m_minHigher;
    double m_defaultLower;
    double m_defaultHigher;
    double m_maxLower;
    double m_maxHigher;
    
    void updateForSilence(Guidance &guidance) const {
        
        m_log.log(2, "Guide::updateForSilence");

        double nyquist = m_parameters.sampleRate / 2.0;
        if (!m_parameters.singleWindowMode) {
            guidance.fftBands[0].f0 = 0.0;
            guidance.fftBands[0].f1 = 0.0;
            guidance.fftBands[1].f0 = 0.0;
            guidance.fftBands[1].f1 = nyquist;
            guidance.fftBands[2].f0 = nyquist;
            guidance.fftBands[2].f1 = nyquist;
        }
        guidance.phaseReset.present = true;
        guidance.phaseReset.f0 = 0.0;
        guidance.phaseReset.f1 = nyquist;
    }

    void updateForUnity(Guidance &guidance,
                        bool hadPhaseReset,
                        const BinSegmenter::Segmentation &segmentation,
                        bool realtime) const {
        
        m_log.log(2, "Guide::updateForUnity: realtime and single-window mode", (int)realtime, m_parameters.singleWindowMode);
        
        double nyquist = m_parameters.sampleRate / 2.0;

        if (!realtime) {
            // ratio can't change, so we are just running 1.0 ratio
            // throughout
            if (!m_parameters.singleWindowMode) {
                guidance.fftBands[0].f0 = 0.0;
                guidance.fftBands[0].f1 = 0.0;
                guidance.fftBands[1].f0 = 0.0;
                guidance.fftBands[1].f1 = nyquist;
                guidance.fftBands[2].f0 = nyquist;
                guidance.fftBands[2].f1 = nyquist;
            }
            guidance.phaseReset.present = true;
            guidance.phaseReset.f0 = 0.0;
            guidance.phaseReset.f1 = nyquist;
            return;
        }

        if (!m_parameters.singleWindowMode) {
            guidance.fftBands[0].f0 = 0.0;
            guidance.fftBands[0].f1 = m_minLower;
            guidance.fftBands[1].f0 = m_minLower;
            guidance.fftBands[1].f1 = m_minHigher;
            guidance.fftBands[2].f0 = m_minHigher;
            guidance.fftBands[2].f1 = nyquist;
        }
        
        guidance.phaseReset.present = true;

        if (!hadPhaseReset) {
            guidance.phaseReset.f0 = 16000.0;
            guidance.phaseReset.f1 = nyquist;
            return;
        } else {
            m_log.log(2, "Guide::updateForUnity: had phase reset");
            guidance.phaseReset.f0 *= 0.9;
            guidance.phaseReset.f1 *= 1.1;
        }

        if (guidance.phaseReset.f0 < segmentation.residualAbove) {
            guidance.phaseReset.f0 = std::min(guidance.phaseReset.f0,
                                              segmentation.percussiveAbove);
        }

        if (guidance.phaseReset.f1 > 16000.0) {
            guidance.phaseReset.f1 = nyquist;
        }
        
        if (guidance.phaseReset.f0 < 100.0) {
            guidance.phaseReset.f0 = 0.0;
        }

        m_log.log(2, "Guide::updateForUnity: f0 and f1", guidance.phaseReset.f0, guidance.phaseReset.f1);
    }

    bool checkPotentialKick(const process_t *const magnitudes,
                            const process_t *const prevMagnitudes) const {
        int b = binForFrequency(200.0, m_configuration.classificationFftSize,
                                m_parameters.sampleRate);
        process_t here = 0.0, there = 0.0;
        for (int i = 1; i <= b; ++i) {
            here += magnitudes[i];
        }
        for (int i = 1; i <= b; ++i) {
            there += prevMagnitudes[i];
        }
        return (here > 10.e-3 && here > there * 1.4);
    }

    double descendToValley(double f, const process_t *const magnitudes) const {
        if (f == 0.0 || f == m_parameters.sampleRate/2.0) {
            // These are special cases
            return f;
        }
        int b = binForFrequency(f, m_configuration.classificationFftSize,
                                m_parameters.sampleRate);
        int n = m_configuration.classificationFftSize/2;
        if (b > n) b = n;
        for (int i = 0; i < 3; ++i) {
            if (b < n && magnitudes[b+1] < magnitudes[b]) {
                ++b;
            } else if (b > 0 && magnitudes[b-1] < magnitudes[b]) {
                --b;
            } else {
                break;
            }
        }
        double sf = frequencyForBin(b, m_configuration.classificationFftSize,
                                    m_parameters.sampleRate);
        return sf;
    }

    double betaFor(double f, double ratio) const {
        double b = (2.0 + ratio) / 3.0;
        double limit = 10000.0;
        if (f > limit) {
            return b;
        } else {
            return 1.0 + f * (b - 1.0) / limit;
        }
    }
};

}

#endif
