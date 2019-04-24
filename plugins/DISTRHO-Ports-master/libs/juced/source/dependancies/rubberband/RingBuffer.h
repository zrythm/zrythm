/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    Rubber Band
    An audio time-stretching and pitch-shifting library.
    Copyright 2007-2008 Chris Cannam.
    
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _RUBBERBAND_RINGBUFFER_H_
#define _RUBBERBAND_RINGBUFFER_H_

#include <sys/types.h>

#include <cstring>

#ifndef _WIN32
#include <sys/mman.h>
#endif

#include "Scavenger.h"
#include "Profiler.h"


//#define DEBUG_RINGBUFFER 1

#ifdef _WIN32
#define MLOCK(a,b) 1
#define MUNLOCK(a,b) 1
#else
#define MLOCK(a,b) ::mlock(a,b)
#define MUNLOCK(a,b) ::munlock(a,b)
#endif

#ifdef DEBUG_RINGBUFFER
#include <iostream>
#endif

namespace RubberBand {

/**
 * RingBuffer implements a lock-free ring buffer for one writer and N
 * readers, that is to be used to store a sample type Type.
 */

template <typename Type, int N = 1>
class RingBuffer
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
    RingBuffer(int n);

    virtual ~RingBuffer();

    /**
     * Return the total capacity of the ring buffer in samples.
     * (This is the argument n passed to the constructor.)
     */
    int getSize() const;

    /**
     * Resize the ring buffer.  This also empties it; use resized()
     * below if you do not want this to happen.  Actually swaps in a
     * new, larger buffer; the old buffer is scavenged after a seemly
     * delay.  Should be called from the write thread.
     */
    void resize(int newSize);

    /**
     * Return a new ring buffer (allocated with "new" -- called must
     * delete when no longer needed) of the given size, containing the
     * same data as this one.  If another thread reads from or writes
     * to this buffer during the call, the results may be incomplete
     * or inconsistent.  If this buffer's data will not fit in the new
     * size, the contents are undefined.
     */
    RingBuffer<Type, N> *resized(int newSize, int R = 0) const;

    /**
     * Lock the ring buffer into physical memory.  Returns true
     * for success.
     */
    bool mlock();

    /**
     * Reset read and write pointers, thus emptying the buffer.
     * Should be called from the write thread.
     */
    void reset();

    /**
     * Return the amount of data available for reading by reader R, in
     * samples.
     */
    int getReadSpace(int R = 0) const;

    /**
     * Return the amount of space available for writing, in samples.
     */
    int getWriteSpace() const;

    /**
     * Read n samples from the buffer, for reader R.  If fewer than n
     * are available, the remainder will be zeroed out.  Returns the
     * number of samples actually read.
     */
    int read(Type *R__ destination, int n, int R = 0);

    /**
     * Read n samples from the buffer, for reader R, adding them to
     * the destination.  If fewer than n are available, the remainder
     * will be left alone.  Returns the number of samples actually
     * read.
     */
    int readAdding(Type *R__ destination, int n, int R = 0);

    /**
     * Read one sample from the buffer, for reader R.  If no sample is
     * available, this will silently return zero.  Calling this
     * repeatedly is obviously slower than calling read once, but it
     * may be good enough if you don't want to allocate a buffer to
     * read into.
     */
    Type readOne(int R = 0);

    /**
     * Read n samples from the buffer, if available, for reader R,
     * without advancing the read pointer -- i.e. a subsequent read()
     * or skip() will be necessary to empty the buffer.  If fewer than
     * n are available, the remainder will be zeroed out.  Returns the
     * number of samples actually read.
     */
    int peek(Type *R__ destination, int n, int R = 0) const;

    /**
     * Read one sample from the buffer, if available, without
     * advancing the read pointer -- i.e. a subsequent read() or
     * skip() will be necessary to empty the buffer.  Returns zero if
     * no sample was available.
     */
    Type peekOne(int R = 0) const;

    /**
     * Pretend to read n samples from the buffer, for reader R,
     * without actually returning them (i.e. discard the next n
     * samples).  Returns the number of samples actually available for
     * discarding.
     */
    int skip(int n, int R = 0);

    /**
     * Write n samples to the buffer.  If insufficient space is
     * available, not all samples may actually be written.  Returns
     * the number of samples actually written.
     */
    int write(const Type *source, int n);

    /**
     * Write n zero-value samples to the buffer.  If insufficient
     * space is available, not all zeros may actually be written.
     * Returns the number of zeroes actually written.
     */
    int zero(int n);

protected:
    Type *R__      m_buffer;
    volatile int     m_writer;
    volatile int     m_readers[N];
    int              m_size;
    bool             m_mlocked;

    static Scavenger<ScavengerArrayWrapper<Type> > m_scavenger;

private:
    RingBuffer(const RingBuffer &); // not provided
    RingBuffer &operator=(const RingBuffer &); // not provided
};

template <typename Type, int N>
Scavenger<ScavengerArrayWrapper<Type> > RingBuffer<Type, N>::m_scavenger;

template <typename Type, int N>
RingBuffer<Type, N>::RingBuffer(int n) :
    m_buffer(new Type[n + 1]),
    m_writer(0),
    m_size(n + 1),
    m_mlocked(false)
{
#ifdef DEBUG_RINGBUFFER
    std::cerr << "RingBuffer<Type," << N << ">[" << this << "]::RingBuffer(" << n << ")" << std::endl;
#endif

    for (int i = 0; i < N; ++i) m_readers[i] = 0;

    m_scavenger.scavenge();
}

template <typename Type, int N>
RingBuffer<Type, N>::~RingBuffer()
{
#ifdef DEBUG_RINGBUFFER
    std::cerr << "RingBuffer<Type," << N << ">[" << this << "]::~RingBuffer" << std::endl;
#endif

    if (m_mlocked) {
	MUNLOCK((void *)m_buffer, m_size * sizeof(Type));
    }
    delete[] m_buffer;

    m_scavenger.scavenge();
}

template <typename Type, int N>
int
RingBuffer<Type, N>::getSize() const
{
#ifdef DEBUG_RINGBUFFER
    std::cerr << "RingBuffer<Type," << N << ">[" << this << "]::getSize(): " << m_size-1 << std::endl;
#endif

    return m_size - 1;
}

template <typename Type, int N>
void
RingBuffer<Type, N>::resize(int newSize)
{
#ifdef DEBUG_RINGBUFFER
    std::cerr << "RingBuffer<Type," << N << ">[" << this << "]::resize(" << newSize << ")" << std::endl;
#endif

    m_scavenger.scavenge();

    if (m_mlocked) {
	MUNLOCK((void *)m_buffer, m_size * sizeof(Type));
    }

    m_scavenger.claim(new ScavengerArrayWrapper<Type>(m_buffer));

    reset();
    m_buffer = new Type[newSize + 1];
    m_size = newSize + 1;

    if (m_mlocked) {
	if (MLOCK((void *)m_buffer, m_size * sizeof(Type))) {
	    m_mlocked = false;
	}
    }
}

template <typename Type, int N>
RingBuffer<Type, N> *
RingBuffer<Type, N>::resized(int newSize, int R) const
{
    RingBuffer<Type, N> *newBuffer = new RingBuffer<Type, N>(newSize);

    int w = m_writer;
    int r = m_readers[R];

    while (r != w) {
        Type value = m_buffer[r];
        newBuffer->write(&value, 1);
        if (++r == m_size) r = 0;
    }

    return newBuffer;
}

template <typename Type, int N>
bool
RingBuffer<Type, N>::mlock()
{
    if (MLOCK((void *)m_buffer, m_size * sizeof(Type))) return false;
    m_mlocked = true;
    return true;
}

template <typename Type, int N>
void
RingBuffer<Type, N>::reset()
{
#ifdef DEBUG_RINGBUFFER
    std::cerr << "RingBuffer<Type," << N << ">[" << this << "]::reset" << std::endl;
#endif

    m_writer = 0;
    for (int i = 0; i < N; ++i) m_readers[i] = 0;
}

template <typename Type, int N>
int
RingBuffer<Type, N>::getReadSpace(int R) const
{
    int writer = m_writer;
    int reader = m_readers[R];
    int space;

#ifdef DEBUG_RINGBUFFER
    std::cerr << "RingBuffer<Type," << N << ">[" << this << "]::getReadSpace(" << R << "): reader " << reader << ", writer " << writer << std::endl;
#endif

    if (writer > reader) space = writer - reader;
    else if (writer < reader) space = (writer + m_size) - reader;
    else space = 0;

#ifdef DEBUG_RINGBUFFER
    std::cerr << "RingBuffer<Type," << N << ">[" << this << "]::getReadSpace(" << R << "): " << space << std::endl;
#endif

    return space;
}

template <typename Type, int N>
int
RingBuffer<Type, N>::getWriteSpace() const
{
    int space = 0;
    for (int i = 0; i < N; ++i) {
        int writer = m_writer;
        int reader = m_readers[i];
	int here = (reader + m_size - writer - 1);
        if (here >= m_size) here -= m_size;
	if (i == 0 || here < space) space = here;
    }

#ifdef DEBUG_RINGBUFFER
    int rs(getReadSpace()), rp(m_readers[0]);

    std::cerr << "RingBuffer: write space " << space << ", read space "
	      << rs << ", total " << (space + rs) << ", m_size " << m_size << std::endl;
    std::cerr << "RingBuffer: reader " << rp << ", writer " << m_writer << std::endl;
#endif

#ifdef DEBUG_RINGBUFFER
    std::cerr << "RingBuffer<Type," << N << ">[" << this << "]::getWriteSpace(): " << space << std::endl;
#endif

    return space;
}

template <typename Type, int N>
int
RingBuffer<Type, N>::read(Type *R__ destination, int n, int R)
{
    Profiler profiler("RingBuffer::read");

#ifdef DEBUG_RINGBUFFER
    std::cerr << "RingBuffer<Type," << N << ">[" << this << "]::read(dest, " << n << ", " << R << ")" << std::endl;
#endif

    int available = getReadSpace(R);
    if (n > available) {
#ifdef DEBUG_RINGBUFFER
	std::cerr << "WARNING: Only " << available << " samples available"
		  << std::endl;
#endif
        for (int i = available; i < n; ++i) {
            destination[i] = 0;
        }
	n = available;
    }
    if (n == 0) return n;

    int reader = m_readers[R];
    int here = m_size - reader;
    Type *const R__ bufbase = m_buffer + reader;

    if (here >= n) {
        for (int i = 0; i < n; ++i) {
            destination[i] = bufbase[i];
        }
    } else {
        for (int i = 0; i < here; ++i) {
            destination[i] = bufbase[i];
        }
        Type *const R__ destbase = destination + here;
        const int nh = n - here;
        for (int i = 0; i < nh; ++i) {
            destbase[i] = m_buffer[i];
        }
    }

    reader += n;
    while (reader >= m_size) reader -= m_size;
    m_readers[R] = reader;

#ifdef DEBUG_RINGBUFFER
    std::cerr << "RingBuffer<Type," << N << ">[" << this << "]::read: read " << n << ", reader now " << m_readers[R] << std::endl;
#endif

    return n;
}

template <typename Type, int N>
int
RingBuffer<Type, N>::readAdding(Type *R__ destination, int n, int R)
{
    Profiler profiler("RingBuffer::readAdding");

#ifdef DEBUG_RINGBUFFER
    std::cerr << "RingBuffer<Type," << N << ">[" << this << "]::readAdding(dest, " << n << ", " << R << ")" << std::endl;
#endif

    int available = getReadSpace(R);
    if (n > available) {
#ifdef DEBUG_RINGBUFFER
	std::cerr << "WARNING: Only " << available << " samples available"
		  << std::endl;
#endif
	n = available;
    }
    if (n == 0) return n;

    int reader = m_readers[R];
    int here = m_size - reader;
    const Type *const R__ bufbase = m_buffer + reader;

    if (here >= n) {
	for (int i = 0; i < n; ++i) {
	    destination[i] += bufbase[i];
	}
    } else {
	for (int i = 0; i < here; ++i) {
	    destination[i] += bufbase[i];
	}
        Type *const R__ destbase = destination + here;
        const int nh = n - here;
	for (int i = 0; i < nh; ++i) {
	    destbase[i] += m_buffer[i];
	}
    }

    reader += n;
    while (reader >= m_size) reader -= m_size;
    m_readers[R] = reader;
    return n;
}

template <typename Type, int N>
Type
RingBuffer<Type, N>::readOne(int R)
{
#ifdef DEBUG_RINGBUFFER
    std::cerr << "RingBuffer<Type," << N << ">[" << this << "]::readOne(" << R << ")" << std::endl;
#endif

    if (m_writer == m_readers[R]) {
#ifdef DEBUG_RINGBUFFER
	std::cerr << "WARNING: No sample available"
		  << std::endl;
#endif
	return 0;
    }
    int reader = m_readers[R];
    Type value = m_buffer[reader];
    if (++reader == m_size) reader = 0;
    m_readers[R] = reader;
    return value;
}

template <typename Type, int N>
int
RingBuffer<Type, N>::peek(Type *R__ destination, int n, int R) const
{
    Profiler profiler("RingBuffer::peek");

#ifdef DEBUG_RINGBUFFER
    std::cerr << "RingBuffer<Type," << N << ">[" << this << "]::peek(dest, " << n << ", " << R << ")" << std::endl;
#endif

    int available = getReadSpace(R);
    if (n > available) {
#ifdef DEBUG_RINGBUFFER
	std::cerr << "WARNING: Only " << available << " samples available"
		  << std::endl;
#endif
	memset(destination + available, 0, (n - available) * sizeof(Type));
	n = available;
    }
    if (n == 0) return n;

    int reader = m_readers[R];
    int here = m_size - reader;
    const Type *const R__ bufbase = m_buffer + reader;

    if (here >= n) {
        for (int i = 0; i < n; ++i) {
            destination[i] = bufbase[i];
        }
    } else {
        for (int i = 0; i < here; ++i) {
            destination[i] = bufbase[i];
        }
        Type *const R__ destbase = destination + here;
        const int nh = n - here;
        for (int i = 0; i < nh; ++i) {
            destbase[i] = m_buffer[i];
        }
    }

#ifdef DEBUG_RINGBUFFER
    std::cerr << "RingBuffer<Type," << N << ">[" << this << "]::peek: read " << n << std::endl;
#endif

    return n;
}

template <typename Type, int N>
Type
RingBuffer<Type, N>::peekOne(int R) const
{
#ifdef DEBUG_RINGBUFFER
    std::cerr << "RingBuffer<Type," << N << ">[" << this << "]::peek(" << R << ")" << std::endl;
#endif

    if (m_writer == m_readers[R]) {
#ifdef DEBUG_RINGBUFFER
	std::cerr << "WARNING: No sample available"
		  << std::endl;
#endif
	return 0;
    }
    Type value = m_buffer[m_readers[R]];
    return value;
}

template <typename Type, int N>
int
RingBuffer<Type, N>::skip(int n, int R)
{
#ifdef DEBUG_RINGBUFFER
    std::cerr << "RingBuffer<Type," << N << ">[" << this << "]::skip(" << n << ", " << R << ")" << std::endl;
#endif

    int available = getReadSpace(R);
    if (n > available) {
#ifdef DEBUG_RINGBUFFER
	std::cerr << "WARNING: Only " << available << " samples available"
		  << std::endl;
#endif
	n = available;
    }
    if (n == 0) return n;

    int reader = m_readers[R];
    reader += n;
    while (reader >= m_size) reader -= m_size;
    m_readers[R] = reader;
    return n;
}

template <typename Type, int N>
int
RingBuffer<Type, N>::write(const Type *source, int n)
{
    Profiler profiler("RingBuffer::write");

#ifdef DEBUG_RINGBUFFER
    std::cerr << "RingBuffer<Type," << N << ">[" << this << "]::write(" << n << ")" << std::endl;
#endif

    int available = getWriteSpace();
    if (n > available) {
#ifdef DEBUG_RINGBUFFER
	std::cerr << "WARNING: Only room for " << available << " samples"
		  << std::endl;
#endif
	n = available;
    }
    if (n == 0) return n;

    int writer = m_writer;
    int here = m_size - writer;
    Type *const R__ bufbase = m_buffer + writer;

    if (here >= n) {
        for (int i = 0; i < n; ++i) {
            bufbase[i] = source[i];
        }
    } else {
        for (int i = 0; i < here; ++i) {
            bufbase[i] = source[i];
        }
        const int nh = n - here;
        const Type *const R__ srcbase = source + here;
        Type *const R__ buf = m_buffer;
        for (int i = 0; i < nh; ++i) {
            buf[i] = srcbase[i];
        }
    }

    writer += n;
    while (writer >= m_size) writer -= m_size;
    m_writer = writer;

#ifdef DEBUG_RINGBUFFER
    std::cerr << "RingBuffer<Type," << N << ">[" << this << "]::write: wrote " << n << ", writer now " << m_writer << std::endl;
#endif

    return n;
}

template <typename Type, int N>
int
RingBuffer<Type, N>::zero(int n)
{
    Profiler profiler("RingBuffer::zero");

#ifdef DEBUG_RINGBUFFER
    std::cerr << "RingBuffer<Type," << N << ">[" << this << "]::zero(" << n << ")" << std::endl;
#endif

    int available = getWriteSpace();
    if (n > available) {
#ifdef DEBUG_RINGBUFFER
	std::cerr << "WARNING: Only room for " << available << " samples"
		  << std::endl;
#endif
	n = available;
    }
    if (n == 0) return n;

    int writer = m_writer;
    int here = m_size - writer;
    Type *const R__ bufbase = m_buffer + writer;

    if (here >= n) {
        for (int i = 0; i < n; ++i) {
            bufbase[i] = 0;
        }
    } else {
        for (int i = 0; i < here; ++i) {
            bufbase[i] = 0;
        }
        const int nh = n - here;
        for (int i = 0; i < nh; ++i) {
            m_buffer[i] = 0;
        }
    }

    writer += n;
    while (writer >= m_size) writer -= m_size;
    m_writer = writer;

#ifdef DEBUG_RINGBUFFER
    std::cerr << "writer -> " << m_writer << std::endl;
#endif

    return n;
}

}

//#include "RingBuffer.cpp"

#endif
