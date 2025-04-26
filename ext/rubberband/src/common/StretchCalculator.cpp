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

#include "StretchCalculator.h"

#include <math.h>
#include <iostream>
#include <deque>
#include <set>
#include <cassert>
#include <algorithm>
#include <sstream>

#include "sysutils.h"

namespace RubberBand
{
	
StretchCalculator::StretchCalculator(size_t sampleRate,
                                     size_t inputIncrement,
                                     bool useHardPeaks,
                                     Log log) :
    m_sampleRate(sampleRate),
    m_increment(inputIncrement),
    m_prevDf(0),
    m_prevRatio(1.0),
    m_prevTimeRatio(1.0),
    m_justReset(true),
    m_transientAmnesty(0),
    m_debugLevel(0),
    m_useHardPeaks(useHardPeaks),
    m_inFrameCounter(0),
    m_frameCheckpoint(0, 0),
    m_outFrameCounter(0),
    m_log(log)
{
    m_log.log(2, "StretchCalculator: useHardPeaks", useHardPeaks);
}    

StretchCalculator::~StretchCalculator()
{
}

void
StretchCalculator::setKeyFrameMap(const std::map<size_t, size_t> &mapping)
{
    m_keyFrameMap = mapping;

    // Ensure we always have a 0 -> 0 mapping. If there's nothing in
    // the map at all, don't need to worry about this (empty map is
    // handled separately anyway)
    if (!m_keyFrameMap.empty()) {
        if (m_keyFrameMap.find(0) == m_keyFrameMap.end()) {
            m_keyFrameMap[0] = 0;
        }
    }
}

std::vector<int>
StretchCalculator::calculate(double ratio, size_t inputDuration,
                             const std::vector<float> &phaseResetDf)
{
    m_peaks = findPeaks(phaseResetDf);

    size_t totalCount = phaseResetDf.size();

    size_t outputDuration = lrint(inputDuration * ratio);

    m_log.log(1, "StretchCalculator::calculate: inputDuration and ratio", inputDuration, ratio);

    outputDuration = lrint((phaseResetDf.size() * m_increment) * ratio);

    m_log.log(1, "StretchCalculator::calculate: outputDuration rounds up from and to", inputDuration * ratio, outputDuration);
    m_log.log(1, "StretchCalculator::calculate: df size and increment", phaseResetDf.size(), m_increment);
    
    std::vector<Peak> peaks; // peak position (in chunks) and hardness
    std::vector<size_t> targets; // targets for mapping peaks (in samples)
    mapPeaks(peaks, targets, outputDuration, totalCount);

    m_log.log(2, "have fixed positions", peaks.size());

    size_t totalInput = 0, totalOutput = 0;

    std::vector<int> increments;

    for (size_t i = 0; i <= peaks.size(); ++i) {
        
        size_t regionStart, regionStartChunk, regionEnd, regionEndChunk;
        bool phaseReset = false;

        if (i == 0) {
            regionStartChunk = 0;
            regionStart = 0;
        } else {
            regionStartChunk = peaks[i-1].chunk;
            regionStart = targets[i-1];
            phaseReset = peaks[i-1].hard;
        }

        if (i == peaks.size()) {
            regionEndChunk = totalCount;
            regionEnd = outputDuration;
        } else {
            regionEndChunk = peaks[i].chunk;
            regionEnd = targets[i];
        }
        
        if (regionStartChunk > totalCount) regionStartChunk = totalCount;
        if (regionStart > outputDuration) regionStart = outputDuration;
        if (regionEndChunk > totalCount) regionEndChunk = totalCount;
        if (regionEnd > outputDuration) regionEnd = outputDuration;

        if (regionEndChunk < regionStartChunk) regionEndChunk = regionStartChunk;
        if (regionEnd < regionStart) regionEnd = regionStart;

        size_t regionDuration = regionEnd - regionStart;

        size_t nchunks = regionEndChunk - regionStartChunk;

        m_log.log(2, "region from and to (chunks)", regionStartChunk, regionEndChunk);
        m_log.log(2, "region from and to (samples)", regionStart, regionEnd);

        if (nchunks == 0) {
            m_log.log(2, "note: nchunks == 0");
            continue;
        }

        double per = double(regionDuration) / double(nchunks);
        double acc = 0.0;
        size_t nremaining = nchunks;
        size_t totalForRegion = 0;

        if (phaseReset) {
            size_t incr;
            if (nchunks > 1) {
                incr = m_increment;
                if (incr > regionDuration) {
                    incr = regionDuration;
                }
            } else {
                incr = regionDuration;
            }
            increments.push_back(- int64_t(incr));
            per = double(regionDuration - incr) / double(nchunks - 1);
            acc += incr;
            totalForRegion += incr;
            totalInput += m_increment;
            nremaining = nremaining - 1;
        }

        if (nremaining > 0) {
            for (size_t j = 0; j+1 < nremaining; ++j) {
                acc += per;
                size_t incr = size_t(round(acc - totalForRegion));
                increments.push_back(incr);
                totalForRegion += incr;
                totalInput += m_increment;
            }
            if (regionDuration > totalForRegion) {
                size_t final = regionDuration - totalForRegion;
                increments.push_back(final);
                totalForRegion += final;
                totalInput += m_increment;
            }
        }

        totalOutput += totalForRegion;
    }

    m_log.log(1, "total input (frames, chunks)", totalInput, totalInput / m_increment);
    m_log.log(1, "total output and achieved ratio", totalOutput, double(totalOutput)/double(totalInput));
    m_log.log(1, "ideal output", totalInput * ratio);
    
    return increments;
}

void
StretchCalculator::mapPeaks(std::vector<Peak> &peaks,
                            std::vector<size_t> &targets,
                            size_t outputDuration,
                            size_t totalCount)
{
    // outputDuration is in audio samples; totalCount is in chunks

    if (m_keyFrameMap.empty()) {
        // "normal" behaviour -- fixed points are strictly in
        // proportion
        peaks = m_peaks;
        for (size_t i = 0; i < peaks.size(); ++i) {
            targets.push_back
                (lrint((double(peaks[i].chunk) * outputDuration) / totalCount));
        }
        return;
    }

    // We have been given a set of source -> target sample frames in
    // m_keyFrameMap.  We want to ensure that (to the nearest chunk) these
    // are followed exactly, and any fixed points that we calculated
    // ourselves are interpolated in linear proportion in between.

    size_t peakidx = 0;
    std::map<size_t, size_t>::const_iterator mi = m_keyFrameMap.begin();

    // NB we know for certain we have a mapping from 0 -> 0 (or at
    // least, some mapping for source sample 0) because that is
    // enforced in setKeyFrameMap above.  However, we aren't guaranteed
    // to have a mapping for the total duration -- we will usually
    // need to assume it maps to the normal duration * ratio sample

    while (mi != m_keyFrameMap.end()) {

        // The map we've been given is from sample to sample, but
        // we can only map from chunk to sample.  We should perhaps
        // adjust the target sample to compensate for the discrepancy
        // between the chunk position and the exact requested source
        // sample.  But we aren't doing that yet.

        size_t sourceStartChunk = mi->first / m_increment;
        size_t sourceEndChunk = totalCount;

        size_t targetStartSample = mi->second;
        size_t targetEndSample = outputDuration;

        ++mi;
        if (mi != m_keyFrameMap.end()) {
            sourceEndChunk = mi->first / m_increment;
            targetEndSample = mi->second;
        }

        if (sourceStartChunk >= totalCount ||
            sourceStartChunk >= sourceEndChunk ||
            targetStartSample >= outputDuration ||
            targetStartSample >= targetEndSample) {
            m_log.log(0, "NOTE: ignoring key-frame mapping from chunk to sample", sourceStartChunk, targetStartSample);
            m_log.log(0, "(source or target chunk exceeds total count, or end is not later than start)");
            continue;
        }
        
        // one peak and target for the mapping, then one for each of
        // the computed peaks that appear before the following mapping

        Peak p;
        p.chunk = sourceStartChunk;
        p.hard = false; // mappings are in time only, not phase reset points
        peaks.push_back(p);
        targets.push_back(targetStartSample);

        m_log.log(2, "mapped key-frame chunk to frame", sourceStartChunk, targetStartSample);

        while (peakidx < m_peaks.size()) {

            size_t pchunk = m_peaks[peakidx].chunk;

            if (pchunk < sourceStartChunk) {
                // shouldn't happen, should have been dealt with
                // already -- but no harm in ignoring it explicitly
                ++peakidx;
                continue;
            }
            if (pchunk == sourceStartChunk) {
                // convert that last peak to a hard one, after all
                peaks[peaks.size()-1].hard = true;
                ++peakidx;
                continue;
            }
            if (pchunk >= sourceEndChunk) {
                // leave the rest for after the next mapping
                break;
            }
            p.chunk = pchunk;
            p.hard = m_peaks[peakidx].hard;

            double proportion =
                double(pchunk - sourceStartChunk) /
                double(sourceEndChunk - sourceStartChunk);
            
            size_t target =
                targetStartSample +
                lrint(proportion *
                      (targetEndSample - targetStartSample));

            if (target <= targets[targets.size()-1] + m_increment) {
                // peaks will become too close together afterwards, ignore
                ++peakidx;
                continue;
            }

            m_log.log(2, "mapped peak chunk to frame", pchunk, target);

            peaks.push_back(p);
            targets.push_back(target);
            ++peakidx;
        }
    }
}    

int64_t
StretchCalculator::expectedOutFrame(int64_t inFrame, double timeRatio)
{
    int64_t checkpointedAt = m_frameCheckpoint.first;
    int64_t checkpointed = m_frameCheckpoint.second;
    return int64_t(round(checkpointed + (inFrame - checkpointedAt) * timeRatio));
}

int
StretchCalculator::calculateSingle(double timeRatio,
                                   double effectivePitchRatio,
                                   float df,
                                   size_t inIncrement,
                                   size_t analysisWindowSize,
                                   size_t synthesisWindowSize,
                                   bool alignFrameStarts)
{
    double ratio = timeRatio / effectivePitchRatio;
    
    int increment = int(inIncrement);
    if (increment == 0) increment = m_increment;

    int outIncrement = lrint(increment * ratio); // the normal case
    bool isTransient = false;
    
    // We want to ensure, as close as possible, that the phase reset
    // points appear at the right audio frame numbers. To this end we
    // track the incoming frame number, its corresponding expected
    // output frame number, and the actual output frame number
    // projected based on the ratios provided.
    //
    // There are two subtleties:
    // 
    // (1) on a ratio change, we need to checkpoint the expected
    // output frame number reached so far and start counting again
    // with the new ratio. We could do this with a reset to zero, but
    // it's easier to reason about absolute input/output frame
    // matches, so for the moment at least we're doing this by
    // explicitly checkpointing the current numbers (hence the use of
    // the above expectedOutFrame() function which refers to the
    // last checkpointed values).
    //
    // (2) in the case of a pitch shift in a configuration where
    // resampling occurs after stretching, all of our output
    // increments will be effectively modified by resampling after we
    // return. This is why we separate out timeRatio and
    // effectivePitchRatio arguments - the former is the ratio that
    // has already been applied and the latter is the ratio that will
    // be applied by any subsequent resampling step (which will be 1.0
    // / pitchScale if resampling is happening after stretching). So
    // the overall ratio is timeRatio / effectivePitchRatio.

    bool ratioChanged = (!m_justReset) && (ratio != m_prevRatio);
    m_justReset = false;
    
    if (ratioChanged) {
        // Reset our frame counters from the ratio change.

        // m_outFrameCounter tracks the frames counted at output from
        // this function, which normally precedes resampling - hence
        // the use of timeRatio rather than ratio here

        m_log.log(2, "StretchCalculator: ratio changed from and to", m_prevRatio, ratio);

        int64_t toCheckpoint = expectedOutFrame
            (m_inFrameCounter, m_prevTimeRatio);
        m_frameCheckpoint =
            std::pair<int64_t, int64_t>(m_inFrameCounter, toCheckpoint);
    }
    
    m_prevRatio = ratio;
    m_prevTimeRatio = timeRatio;

    if (m_log.getDebugLevel() > 2) {
        std::ostringstream os;
        os << "StretchCalculator::calculateSingle: timeRatio = "
           << timeRatio << ", effectivePitchRatio = "
           << effectivePitchRatio << " (that's 1.0 / "
           << (1.0 / effectivePitchRatio)
           << "), ratio = " << ratio << ", df = " << df
           << ", inIncrement = " << inIncrement
           << ", default outIncrement = " << outIncrement
           << ", analysisWindowSize = " << analysisWindowSize
           << ", synthesisWindowSize = " << synthesisWindowSize
           << "\n";

        os << "inFrameCounter = " << m_inFrameCounter
           << ", outFrameCounter = " << m_outFrameCounter
           << "\n";

        os << "The next sample out is input sample " << m_inFrameCounter << "\n";
        m_log.log(3, os.str().c_str());
    }

    int64_t intended, projected;
    if (alignFrameStarts) { // R3
        intended = expectedOutFrame(m_inFrameCounter, timeRatio);
        projected =
            int64_t(round(m_outFrameCounter));
    } else { // R2
        intended = expectedOutFrame
            (m_inFrameCounter + analysisWindowSize/4, timeRatio);
        projected =
            int64_t(round(m_outFrameCounter +
                          (synthesisWindowSize/4 * effectivePitchRatio)));
    }

    int64_t divergence = projected - intended;

    m_log.log(3, "for current frame + quarter frame: intended vs projected", intended, projected);
    m_log.log(3, "divergence", divergence);
    
    // In principle, the threshold depends on chunk size: larger chunk
    // sizes need higher thresholds.  Since chunk size depends on
    // ratio, I suppose we could in theory calculate the threshold
    // from the ratio directly.  For the moment we're happy if it
    // works well in common situations.

    float transientThreshold = 0.35f;
//    if (ratio > 1) transientThreshold = 0.25f;

    if (m_useHardPeaks && df > m_prevDf * 1.1f && df > transientThreshold) {
        if (divergence > 1000 || divergence < -1000) {
            m_log.log(2, "StretchCalculator::calculateSingle: transient, but we're not permitting it because the divergence is too great", divergence);
        } else {
            isTransient = true;
        }
    }

    m_log.log(3, "df and prevDf", df, m_prevDf);

    m_prevDf = df;

    if (m_transientAmnesty > 0) {
        if (isTransient) {
            m_log.log(2, "StretchCalculator::calculateSingle: transient, but we have an amnesty: df and threshold", df, transientThreshold);
            isTransient = false;
        }
        --m_transientAmnesty;
    }
            
    if (isTransient) {
        m_log.log(2, "StretchCalculator::calculateSingle: transient: df and threshold", df, transientThreshold);

        // as in offline mode, 0.05 sec approx min between transients
        m_transientAmnesty =
            lrint(ceil(double(m_sampleRate) / (20 * double(increment))));

        outIncrement = increment;

    } else {

        double recovery = 0.0;
        if (divergence > 1000 || divergence < -1000) {
            recovery = divergence / ((m_sampleRate / 10.0) / increment);
        } else if (divergence > 100 || divergence < -100) {
            recovery = divergence / ((m_sampleRate / 20.0) / increment);
        } else {
            recovery = divergence / 4.0;
        }

        int incr = lrint(outIncrement - recovery);

        int level = (divergence != 0 ? 2 : 3);
        m_log.log(level, "divergence and recovery", divergence, recovery);
        m_log.log(level, "outIncrement and adjusted incr", outIncrement, incr);

        int minIncr = lrint(increment * ratio * 0.3);
        int maxIncr = lrint(increment * ratio * 2);
        
        if (incr < minIncr) {
            incr = minIncr;
        } else if (incr > maxIncr) {
            incr = maxIncr;
        }

        m_log.log(level, "clamped into", minIncr, maxIncr);
        m_log.log(level, "giving incr", incr);

        if (incr < 0) {
            m_log.log(0, "WARNING: internal error: incr < 0 in calculateSingle");
            outIncrement = 0;
        } else {
            outIncrement = incr;
        }
    }

    m_log.log(2, "StretchCalculator::calculateSingle: returning isTransient and outIncrement", isTransient, outIncrement);

    m_inFrameCounter += inIncrement;
    m_outFrameCounter += outIncrement * effectivePitchRatio;
    
    if (isTransient) {
        return -outIncrement;
    } else {
        return outIncrement;
    }
}

void
StretchCalculator::reset()
{
    m_prevDf = 0;
    m_prevRatio = 1.0;
    m_prevTimeRatio = 1.0;
    m_inFrameCounter = 0;
    m_frameCheckpoint = std::pair<int64_t, int64_t>(0, 0);
    m_outFrameCounter = 0.0;
    m_transientAmnesty = 0;
    m_keyFrameMap.clear();

    m_justReset = true;
}

std::vector<StretchCalculator::Peak>
StretchCalculator::findPeaks(const std::vector<float> &rawDf)
{
    std::vector<float> df = smoothDF(rawDf);

    // We distinguish between "soft" and "hard" peaks.  A soft peak is
    // simply the result of peak-picking on the smoothed onset
    // detection function, and it represents any (strong-ish) onset.
    // We aim to ensure always that soft peaks are placed at the
    // correct position in time.  A hard peak is where there is a very
    // rapid rise in detection function, and it presumably represents
    // a more broadband, noisy transient.  For these we perform a
    // phase reset (if in the appropriate mode), and we locate the
    // reset at the first point where we notice enough of a rapid
    // rise, rather than necessarily at the peak itself, in order to
    // preserve the shape of the transient.
            
    std::set<size_t> hardPeakCandidates;
    std::set<size_t> softPeakCandidates;

    if (m_useHardPeaks) {

        // 0.05 sec approx min between hard peaks
        size_t hardPeakAmnesty = lrint(ceil(double(m_sampleRate) /
                                            (20 * double(m_increment))));
        size_t prevHardPeak = 0;

        m_log.log(2, "hardPeakAmnesty", hardPeakAmnesty);

        for (size_t i = 1; i + 1 < df.size(); ++i) {

            if (df[i] < 0.1) continue;
            if (df[i] <= df[i-1] * 1.1) continue;
            if (df[i] < 0.22) continue;

            if (!hardPeakCandidates.empty() &&
                i < prevHardPeak + hardPeakAmnesty) {
                continue;
            }

            bool hard = (df[i] > 0.4);

            if (hard) {
                m_log.log(2, "hard peak, df > absolute 0.4: chunk and df", i, df[i]);
            }

            if (!hard) {
                hard = (df[i] > df[i-1] * 1.4);

                if (hard) {
                    m_log.log(2, "hard peak, single rise of 40%: chunk and df", i, df[i]);
                }
            }

            if (!hard && i > 1) {
                hard = (df[i]   > df[i-1] * 1.2 &&
                        df[i-1] > df[i-2] * 1.2);

                if (hard) {
                    m_log.log(2, "hard peak, two rises of 20%: chunk and df", i, df[i]);
                }
            }

            if (!hard && i > 2) {
                // have already established that df[i] > df[i-1] * 1.1
                hard = (df[i] > 0.3 &&
                        df[i-1] > df[i-2] * 1.1 &&
                        df[i-2] > df[i-3] * 1.1);

                if (hard) {
                    m_log.log(2, "hard peak, three rises of 10%: chunk and df", i, df[i]);
                }
            }

            if (!hard) continue;

            size_t peakLocation = i;

            if (i + 1 < rawDf.size() &&
                rawDf[i + 1] > rawDf[i] * 1.4) {

                ++peakLocation;

                m_log.log(2, "big rise next, pushing hard peak forward to", peakLocation);
            }

            hardPeakCandidates.insert(peakLocation);
            prevHardPeak = peakLocation;
        }
    }

    size_t medianmaxsize = lrint(ceil(double(m_sampleRate) /
                                 double(m_increment))); // 1 sec ish

    m_log.log(2, "mediansize", medianmaxsize);
    if (medianmaxsize < 7) {
        medianmaxsize = 7;
        m_log.log(2, "adjusted mediansize", medianmaxsize);
    }

    int minspacing = lrint(ceil(double(m_sampleRate) /
                                (20 * double(m_increment)))); // 0.05 sec ish
    
    std::deque<float> medianwin;
    std::vector<float> sorted;
    int softPeakAmnesty = 0;

    for (size_t i = 0; i < medianmaxsize/2; ++i) {
        medianwin.push_back(0);
    }
    for (size_t i = 0; i < medianmaxsize/2 && i < df.size(); ++i) {
        medianwin.push_back(df[i]);
    }

    size_t lastSoftPeak = 0;

    for (size_t i = 0; i < df.size(); ++i) {
        
        size_t mediansize = medianmaxsize;

        if (medianwin.size() < mediansize) {
            mediansize = medianwin.size();
        }

        size_t middle = medianmaxsize / 2;
        if (middle >= mediansize) middle = mediansize-1;

        size_t nextDf = i + mediansize - middle;

        if (mediansize < 2) {
            if (mediansize > medianmaxsize) { // absurd, but never mind that
                medianwin.pop_front();
            }
            if (nextDf < df.size()) {
                medianwin.push_back(df[nextDf]);
            } else {
                medianwin.push_back(0);
            }
            continue;
        }

        sorted.clear();
        for (size_t j = 0; j < mediansize; ++j) {
            sorted.push_back(medianwin[j]);
        }
        std::sort(sorted.begin(), sorted.end());

        size_t n = 90; // percentile above which we pick peaks
        size_t index = (sorted.size() * n) / 100;
        if (index >= sorted.size()) index = sorted.size()-1;
        if (index == sorted.size()-1 && index > 0) --index;
        float thresh = sorted[index];

        if (medianwin[middle] > thresh &&
            medianwin[middle] > medianwin[middle-1] &&
            medianwin[middle] > medianwin[middle+1] &&
            softPeakAmnesty == 0) {

            size_t maxindex = middle;
            float maxval = medianwin[middle];

            for (size_t j = middle+1; j < mediansize; ++j) {
                if (medianwin[j] > maxval) {
                    maxval = medianwin[j];
                    maxindex = j;
                } else if (medianwin[j] < medianwin[middle]) {
                    break;
                }
            }

            size_t peak = i + maxindex - middle;

            if (softPeakCandidates.empty() || lastSoftPeak != peak) {
                m_log.log(2, "soft peak: chunk and median df", peak, medianwin[middle]);
                if (peak >= df.size()) {
                    m_log.log(2, "peak is beyond end");
                } else {
                    softPeakCandidates.insert(peak);
                    lastSoftPeak = peak;
                }
            }

            softPeakAmnesty = minspacing + maxindex - middle;
            m_log.log(3, "amnesty", softPeakAmnesty);

        } else if (softPeakAmnesty > 0) --softPeakAmnesty;

        if (mediansize >= medianmaxsize) {
            medianwin.pop_front();
        }
        if (nextDf < df.size()) {
            medianwin.push_back(df[nextDf]);
        } else {
            medianwin.push_back(0);
        }
    }

    std::vector<Peak> peaks;

    while (!hardPeakCandidates.empty() || !softPeakCandidates.empty()) {

        bool haveHardPeak = !hardPeakCandidates.empty();
        bool haveSoftPeak = !softPeakCandidates.empty();

        size_t hardPeak = (haveHardPeak ? *hardPeakCandidates.begin() : 0);
        size_t softPeak = (haveSoftPeak ? *softPeakCandidates.begin() : 0);

        Peak peak;
        peak.hard = false;
        peak.chunk = softPeak;

        bool ignore = false;

        if (haveHardPeak &&
            (!haveSoftPeak || hardPeak <= softPeak)) {
            m_log.log(3, "hard peak", hardPeak);
            peak.hard = true;
            peak.chunk = hardPeak;
            hardPeakCandidates.erase(hardPeakCandidates.begin());
        } else {
            m_log.log(3, "soft peak", softPeak);
            if (!peaks.empty() &&
                peaks[peaks.size()-1].hard &&
                peaks[peaks.size()-1].chunk + 3 >= softPeak) {
                m_log.log(3, "ignoring, as we just had a hard peak");
                ignore = true;
            }
        }            

        if (haveSoftPeak && peak.chunk == softPeak) {
            softPeakCandidates.erase(softPeakCandidates.begin());
        }

        if (!ignore) {
            peaks.push_back(peak);
        }
    }                

    return peaks;
}

std::vector<float>
StretchCalculator::smoothDF(const std::vector<float> &df)
{
    std::vector<float> smoothedDF;
    
    for (size_t i = 0; i < df.size(); ++i) {
        // three-value moving mean window for simple smoothing
        float total = 0.f, count = 0;
        if (i > 0) { total += df[i-1]; ++count; }
        total += df[i]; ++count;
        if (i+1 < df.size()) { total += df[i+1]; ++count; }
        float mean = total / count;
        smoothedDF.push_back(mean);
    }

    return smoothedDF;
}

}

