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

#ifndef RUBBERBAND_BIN_CLASSIFIER_H
#define RUBBERBAND_BIN_CLASSIFIER_H

#include "../common/Allocators.h"
#include "../common/MovingMedian.h"
#include "../common/RingBuffer.h"
#include "../common/Profiler.h"

#include <vector>
#include <memory>

namespace RubberBand {

class BinClassifier
{
public:
    enum class Classification {
        Harmonic = 0,
        Percussive = 1,
        Residual = 2
    };

    struct Parameters {
        int binCount;
        int horizontalFilterLength;
        int horizontalFilterLag;
        int verticalFilterLength;
        double harmonicThreshold;
        double percussiveThreshold;
        Parameters(int _binCount, int _horizontalFilterLength,
                   int _horizontalFilterLag, int _verticalFilterLength,
                   double _harmonicThreshold, double _percussiveThreshold) :
            binCount(_binCount),
            horizontalFilterLength(_horizontalFilterLength),
            horizontalFilterLag(_horizontalFilterLag),
            verticalFilterLength(_verticalFilterLength),
            harmonicThreshold(_harmonicThreshold),
            percussiveThreshold(_percussiveThreshold) { }
    };
    
    BinClassifier(Parameters parameters) :
        m_parameters(parameters),
        m_hFilters(new MovingMedianStack<process_t>(m_parameters.binCount,
                                                    m_parameters.horizontalFilterLength)),
        m_vFilter(new MovingMedian<process_t>(m_parameters.verticalFilterLength)),
        m_vfQueue(parameters.horizontalFilterLag)
    {
        int n = m_parameters.binCount;

        m_hf = allocate_and_zero<process_t>(n);
        m_vf = allocate_and_zero<process_t>(n);
        
        for (int i = 0; i < m_parameters.horizontalFilterLag; ++i) {
            process_t *entry = allocate_and_zero<process_t>(n);
            m_vfQueue.write(&entry, 1);
        }
    }

    ~BinClassifier()
    {
        while (m_vfQueue.getReadSpace() > 0) {
            process_t *entry = m_vfQueue.readOne();
            deallocate(entry);
        }

        deallocate(m_hf);
        deallocate(m_vf);
    }

    void reset()
    {
        while (m_vfQueue.getReadSpace() > 0) {
            process_t *entry = m_vfQueue.readOne();
            deallocate(entry);
        }
        
        for (int i = 0; i < m_parameters.horizontalFilterLag; ++i) {
            process_t *entry =
                allocate_and_zero<process_t>(m_parameters.binCount);
            m_vfQueue.write(&entry, 1);
        }

        m_hFilters->reset();
    }
    
    void classify(const process_t *const mag, // input, of at least binCount bins
                  Classification *classification) // output, of binCount bins
    {
        Profiler profiler("BinClassifier::classify");
        
        const int n = m_parameters.binCount;

        for (int i = 0; i < n; ++i) {
            m_hFilters->push(i, mag[i]);
            m_hf[i] = m_hFilters->get(i);
        }

        v_copy(m_vf, mag, n);
        MovingMedian<process_t>::filter(*m_vFilter, m_vf, n);

        if (m_parameters.horizontalFilterLag > 0) {
            process_t *lagged = m_vfQueue.readOne();
            m_vfQueue.write(&m_vf, 1);
            m_vf = lagged;
        }

        process_t eps = 1.0e-7;
            
        for (int i = 0; i < n; ++i) {
            Classification c;
            if (process_t(m_hf[i]) / (process_t(m_vf[i]) + eps) >
                m_parameters.harmonicThreshold) {
                c = Classification::Harmonic;
            } else if (process_t(m_vf[i]) / (process_t(m_hf[i]) + eps) >
                       m_parameters.percussiveThreshold) {
                c = Classification::Percussive;
            } else {
                c = Classification::Residual;
            }
            classification[i] = c;
        }
    }

protected:
    Parameters m_parameters;
    std::unique_ptr<MovingMedianStack<process_t>> m_hFilters;
    std::unique_ptr<MovingMedian<process_t>> m_vFilter;
    // We manage the queued frames through pointer swapping, hence
    // bare pointers here
    process_t *m_hf;
    process_t *m_vf;
    RingBuffer<process_t *> m_vfQueue;

    BinClassifier(const BinClassifier &) =delete;
    BinClassifier &operator=(const BinClassifier &) =delete;
};

}

#endif
