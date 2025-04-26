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

#ifndef RUBBERBAND_SINGLE_THREAD_RINGBUFFER_H
#define RUBBERBAND_SINGLE_THREAD_RINGBUFFER_H

#include <sys/types.h>

namespace RubberBand {

/**
 * SingleThreadRingBuffer implements a ring buffer to be used to store
 * a sample type T, for reading and writing within a single
 * thread. SingleThreadRingBuffer is a simple container, not a
 * thread-safe lock-free structure: use RingBuffer for the situation
 * with reader and writer in separate threads. Currently this
 * implementation only supports reading and writing a single sample at
 * a time.
 */
template <typename T>
class SingleThreadRingBuffer
{
public:
    /**
     * Create a ring buffer with room to write n samples.
     *
     * Note that the internal storage size will actually be n+1
     * samples, as one element is unavailable for administrative
     * reasons.  Since the ring buffer performs best if its size is a
     * power of two, this means n should ideally be some power of two
     * minus one.
     */
    SingleThreadRingBuffer(int n) :
        m_buffer(n + 1, T()),
        m_writer(0),
        m_reader(0),
        m_size(n + 1) { }

    SingleThreadRingBuffer() :
        m_buffer(1, T()),
        m_writer(0),
        m_reader(0),
        m_size(1) { }

    SingleThreadRingBuffer (const SingleThreadRingBuffer &other) =default;
    SingleThreadRingBuffer &operator=(const SingleThreadRingBuffer &other) =default;
        
    virtual ~SingleThreadRingBuffer() { }

    /**
     * Return the total capacity of the ring buffer in samples.
     * (This is the argument n passed to the constructor.)
     */
    int getSize() const {
        return m_size - 1;
    }

    /**
     * Reset read and write pointers, thus emptying the buffer.
     */
    void reset() {
        m_writer = m_reader;
    }

    /**
     * Return the amount of data available for reading, in samples.
     */
    int getReadSpace() const {
        if (m_writer > m_reader) return m_writer - m_reader;
        else if (m_writer < m_reader) return (m_writer + m_size) - m_reader;
        else return 0;
    }

    /**
     * Return the amount of space available for writing, in samples.
     */
    int getWriteSpace() const {
        int space = (m_reader + m_size - m_writer - 1);
        if (space >= m_size) space -= m_size;
        return space;
    }

    /**
     * Read one sample from the buffer.  If no sample is available,
     * silently return zero.
     */
    T readOne() {
        if (m_writer == m_reader) {
            return {};
        }
        auto value = m_buffer[m_reader];
        if (++m_reader == m_size) m_reader = 0;
        return value;
    }

    /**
     * Pretend to read one sample from the buffer, without actually
     * returning it (i.e. discard the next sample).
     */
    void skipOne() {
        if (m_writer == m_reader) {
            return;
        }
        if (++m_reader == m_size) m_reader = 0;
    }

    /**
     * Write one sample to the buffer.  If insufficient space is
     * available, the sample will not be written.  Returns the number
     * of samples actually written, i.e. 0 or 1.
     */
    int writeOne(const T &value) {
        if (getWriteSpace() == 0) return 0;
        m_buffer[m_writer] = value;
        if (++m_writer == m_size) m_writer = 0;
        return 1;
    }

protected:
    std::vector<T> m_buffer;
    int m_writer;
    int m_reader;
    int m_size;
};

}

#endif

