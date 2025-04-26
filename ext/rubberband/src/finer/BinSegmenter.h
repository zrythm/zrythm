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

#ifndef RUBBERBAND_BIN_SEGMENTER_H
#define RUBBERBAND_BIN_SEGMENTER_H

#include "BinClassifier.h"

#include "../common/HistogramFilter.h"
#include "../common/mathmisc.h"
#include "../common/Profiler.h"

#include <vector>

namespace RubberBand {

class BinSegmenter
{
public:
    struct Segmentation {
        double percussiveBelow;
        double percussiveAbove;
        double residualAbove;
        explicit Segmentation() :
            percussiveBelow(0.0), percussiveAbove(0.0), residualAbove(0.0) { }
        Segmentation(double _pb, double _pa, double _ra) :
            percussiveBelow(_pb), percussiveAbove(_pa), residualAbove(_ra) { }
    };

    struct Parameters {
        int fftSize;
        int binCount;
        double sampleRate;
        int classFilterLength;
        Parameters(int _fftSize, int _binCount, double _sampleRate,
                   int _classFilterLength) :
            fftSize(_fftSize), binCount(_binCount), sampleRate(_sampleRate),
            classFilterLength(_classFilterLength) { }
    };
    
    BinSegmenter(Parameters parameters) :
        m_parameters(parameters),
        m_numeric(m_parameters.binCount, 0),
        m_classFilter(3, m_parameters.classFilterLength)
    {
    }

    Segmentation segment(const BinClassifier::Classification *classification) {

        Profiler profiler("BinSegmenter::segment");
        
        int n = m_parameters.binCount;
        for (int i = 0; i < n; ++i) {
            switch (classification[i]) {
            case BinClassifier::Classification::Harmonic:
                m_numeric[i] = 0; break;
            case BinClassifier::Classification::Percussive:
                m_numeric[i] = 1; break;
            default:
                m_numeric[i] = 2; break;
            }
        }
        HistogramFilter::modalFilter(m_classFilter, m_numeric);
/*
        std::cout << "c:";
        for (int i = 0; i < n; ++i) {
            if (i > 0) std::cout << ",";
            std::cout << m_numeric[i];
        }
        std::cout << std::endl;
*/
        double f0 = 0.0;
        for (int i = 1; i < n; ++i) {
            if (m_numeric[i] != 1) { // percussive
                if (i == 1 && m_numeric[0] != 1) { // percussive
                    f0 = 0.0;
                } else {
                    f0 = frequencyForBin
                        (i, m_parameters.fftSize, m_parameters.sampleRate);
                }
                break;
            }
        }
        double nyquist = m_parameters.sampleRate / 2.0;
        double f1 = nyquist;
        double f2 = nyquist;
        bool inPercussive = false;
        for (int i = n - 1; i > 0; --i) {
            int c = m_numeric[i];
            if (!inPercussive) {
                if (c == 2) { // residual
                    continue;
                } else if (c == 1) { // percussive
                    inPercussive = true;
                    f2 = frequencyForBin
                        (i, m_parameters.fftSize, m_parameters.sampleRate);
                } else { // harmonic
                    f1 = f2 = frequencyForBin
                        (i, m_parameters.fftSize, m_parameters.sampleRate);
                    break;
                }
            } else { // inPercussive
                if (c != 1) { // non-percussive
                    f1 = frequencyForBin
                        (i, m_parameters.fftSize, m_parameters.sampleRate);
                    break;
                }
            }
        }
        if (f1 == nyquist && f2 < nyquist) {
            f1 = 0.0;
        }

//        std::cout << "f0 = " << f0 << ", f1 = " << f1 << ", f2 = " << f2 << std::endl;
        
        return Segmentation(f0, f1, f2);
    }

protected:
    Parameters m_parameters;
    std::vector<int> m_numeric;
    HistogramFilter m_classFilter;

    BinSegmenter(const BinSegmenter &) =delete;
    BinSegmenter &operator=(const BinSegmenter &) =delete;
};

}

#endif
