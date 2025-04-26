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

#ifndef RUBBERBAND_PEAK_H
#define RUBBERBAND_PEAK_H

#include <vector>

namespace RubberBand
{

template <typename T, typename GreaterThan = std::greater<T>>
class Peak
{
public:
    /** Peak picker for array of length n. This allocates on
        construction an internal buffer for temporary values, to be
        used within the peak-picking functions, so that it does not
        have to allocate when used. It does not have persistent state.
    */
    Peak(int n) :
        m_n(n),
        m_locations(n, 0) { }

    /** Find the nearest peak to each bin, and optionally the next
        highest peak above each bin, within an array v, where a peak
        is a value greater than the p nearest neighbours on each
        side. The array must have length n where n is the size passed
        the the constructor.
    */
    void findNearestAndNextPeaks(const T *v,
                                 int p,
                                 int *nearest,
                                 int *next = nullptr)
    {
        findNearestAndNextPeaks(v, 0, m_n, p, nearest, next);
    }

    /** As above but consider only the range of size rangeCount from
        index rangeStart. Write rangeCount results into nearest and
        optionally next, starting to write at index rangeStart - so
        these arrays must have the full length even if rangeCount is
        shorter. Leave the rest of nearest and/or next unmodified.
    */
    void findNearestAndNextPeaks(const T *v,
                                 int rangeStart,
                                 int rangeCount,
                                 int p,
                                 int *nearest,
                                 int *next = nullptr)
    {
        int nPeaks = 0;
        int n = rangeStart + rangeCount;
        GreaterThan greater;

        for (int i = rangeStart; i < n; ++i) {
            T x = v[i];
            bool good = true;
            for (int k = i - p; k <= i + p; ++k) {
                if (k < rangeStart || k == i) continue;
                if (k >= n) break;
                if (k < i && !greater(x, v[k])) {
                    good = false;
                    break;
                }
                if (k > i && greater(v[k], x)) {
                    good = false;
                    break;
                }
            }
            if (good) {
                m_locations[nPeaks++] = i;
            }
        }

        int pp = rangeStart - 1;
        for (int i = rangeStart, j = 0; i < n; ++i) {
            int np = i;
            if (j < nPeaks) {
                np = m_locations[j];
            } else if (nPeaks > 0) {
                np = m_locations[nPeaks-1];
            }
            if (next) {
                if (pp == i || j >= nPeaks) {
                    next[i] = i;
                } else {
                    next[i] = np;
                }
            }
            if (nearest) {
                if (j == 0) {
                    nearest[i] = np;
                } else {
                    if (np - i <= i - pp) {
                        nearest[i] = np;
                    } else {
                        nearest[i] = pp;
                    }
                }
            }
            while (j < nPeaks && m_locations[j] <= i) {
                pp = np;
                ++j;
            }
        }
    }

protected:
    int m_n;
    std::vector<int> m_locations;
};


}

#endif
