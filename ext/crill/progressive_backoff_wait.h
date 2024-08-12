// crill - the Cross-platform Real-time, I/O, and Low-Latency Library
// Copyright (c) 2022 - Timur Doumler and Fabian Renn-Giles
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)

#ifndef CRILL_PROGRESSIVE_BACKOFF_WAIT_H
#define CRILL_PROGRESSIVE_BACKOFF_WAIT_H

#include <crill/impl/progressive_backoff_wait_impl.h>
#include <crill/platform.h>

namespace crill
{

// Effects: Blocks the current thread until predicate returns true. Blocking is
// implemented by spinning on the predicate with a progressive backoff strategy.
//
// crill::progressive_backoff_wait is used to implement crill::spin_mutex, but
// is also useful on its own, in scenarios where a thread needs to wait on
// something else than a spinlock being released (for example, a CAS loop).
//
// Compared to a naive implementation like
//
//    while (!predicate()) /* spin */;
//
// the progressive backoff strategy prevents wasting energy, and allows other
// threads to progress by yielding from the waiting thread after a certain
// amount of time. This time is currently chosen to be approximately 1 ms on a
// typical 64-bit Intel or ARM based machine.
//
// On platforms other than x86, x86_64, and arm64, no implementation is
// currently available.
template <typename Predicate>
void
progressive_backoff_wait (Predicate &&pred)
{
#if defined(CRILL_INTEL) && CRILL_INTEL
  impl::progressive_backoff_wait_intel<5, 10, 3000> (
    std::forward<Predicate> (pred));
  // approx. 5x5 ns (= 25 ns), 10x40 ns (= 400 ns), and 3000x350 ns (~ 1 ms),
  // respectively, when measured on a 2.9 GHz Intel i9
#elif defined(CRILL_ARM_64BIT) && CRILL_ARM_64BIT
  impl::progressive_backoff_wait_armv8<2, 750> (std::forward<Predicate> (pred));
  // approx. 2x10 ns (= 20 ns) and 750x1333 ns (~ 1 ms), respectively, on an
  // Apple Silicon Mac or an armv8 based phone.
#else
#  error "Platform not supported!"
#endif
}

} // namespace crill

#endif // CRILL_PROGRESSIVE_BACKOFF_WAIT_H
