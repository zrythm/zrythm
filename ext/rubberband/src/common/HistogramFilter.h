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

#ifndef RUBBERBAND_HISTOGRAM_FILTER_H
#define RUBBERBAND_HISTOGRAM_FILTER_H

#include "SingleThreadRingBuffer.h"

#include <vector>
#include <iostream>

namespace RubberBand {

/**
 * A median or modal filter implemented using a histogram. The values
 * must come from a compact set of integers within [0,n) where n is
 * specified in the constructor. Pushing a value updates the
 * histogram, after which you can get either the median or the mode.
 * You can call drop() to drop the oldest value without pushing a new
 * one, for example to drain the filter at the tail of the sequence.
 */
class HistogramFilter
{
public:
    HistogramFilter(int nValues, int filterLength) :
        m_buffer(filterLength),
        m_histogram(nValues, 0),
        m_mode(-1) { }
    ~HistogramFilter() { }

    int getFilterLength() const {
        return m_buffer.getSize();
    }

    int getNValues() const {
        return int(m_histogram.size());
    }
    
    void reset() {
        m_buffer.reset();
        for (int i = 0; i < getNValues(); ++i) {
            m_histogram[i] = 0;
        }
    }
    
    void push(int value) {
        if (m_buffer.getWriteSpace() == 0) {
            int toDrop = m_buffer.readOne();
            --m_histogram[toDrop];
        }
        m_buffer.writeOne(value);
        int height = ++m_histogram[value];
        if (m_mode >= 0 && height >= m_histogram[m_mode]) {
            if (height > m_histogram[m_mode] || value < m_mode) {
                m_mode = value;
            }
        }
    }

    void drop() {
        if (m_buffer.getReadSpace() > 0) {
            int toDrop = m_buffer.readOne();
            --m_histogram[toDrop];
            if (toDrop == m_mode) {
                m_mode = -1;
            }
        }
    }

    /** Return the median of the values in the filter currently. If
     *  the median lies between two values, return the first of them.
     */
    int getMedian() const {
        int half = (m_buffer.getReadSpace() + 1) / 2;
        int acc = 0;
        int nvalues = getNValues();
        for (int i = 0; i < nvalues; ++i) {
            acc += m_histogram[i];
            if (acc >= half) {
                return i;
            }
        }
        return 0;
    }

    /** Return the modal value, that is, the value that occurs the
     *  most often in the filter currently. If multiple values occur
     *  an equal number of times, return the smallest of them.
     */
    int getMode() const {
        if (m_mode >= 0) {
            return m_mode;
        }
        int max = 0;
        int mode = 0;
        int nvalues = getNValues();
        for (int i = 0; i < nvalues; ++i) {
            int h = m_histogram[i];
            if (i == 0 || h > max) {
                max = h;
                mode = i;
            }
        }
        m_mode = mode;
        return mode;
    }

    // Convenience function that filters an array in-place. Filter is
    // a median filter unless modal is true. Array has length n.
    // Modifies both the filter and the array.
    //
    static void filter(HistogramFilter &f, int *v, int n, bool modal = false) {
        f.reset();
        int flen = f.getFilterLength();
        int i = -(flen / 2);
        int j = 0;
        while (i != n) {
            if (j < n) {
                f.push(v[j]);
            } else if (j >= flen) {
                f.drop();
            }
            if (i >= 0) {
                int m = modal ? f.getMode() : f.getMedian();
                v[i] = m;
            }
            ++i;
            ++j;
        }
    }

    // As above but with a vector argument
    //
    static void filter(HistogramFilter &f, std::vector<int> &v, bool modal = false) {
        filter(f, v.data(), v.size(), modal);
    }

    // Convenience function that median-filters an array
    // in-place. Array has length n. Modifies both the filter and the
    // array.
    //
    static void medianFilter(HistogramFilter &f, int *v, int n) {
        filter(f, v, n, false);
    }

    // As above but with a vector argument
    //
    static void medianFilter(HistogramFilter &f, std::vector<int> &v) {
        medianFilter(f, v.data(), v.size());
    }

    // Convenience function that modal-filters an array
    // in-place. Array has length n. Modifies both the filter and the
    // array.
    //
    static void modalFilter(HistogramFilter &f, int *v, int n) {
        filter(f, v, n, true);
    }

    // As above but with a vector argument
    //
    static void modalFilter(HistogramFilter &f, std::vector<int> &v) {
        modalFilter(f, v.data(), v.size());
    }
    
private:
    SingleThreadRingBuffer<int> m_buffer;
    std::vector<int> m_histogram;
    mutable int m_mode;
};

}

#endif
