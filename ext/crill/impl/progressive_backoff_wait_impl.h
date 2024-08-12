// crill - the Cross-platform Real-time, I/O, and Low-Latency Library
// Copyright (c) 2022 - Timur Doumler and Fabian Renn-Giles
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)

#ifndef CRILL_PROGRESSIVE_BACKOFF_WAIT_IMPL_H
#define CRILL_PROGRESSIVE_BACKOFF_WAIT_IMPL_H

#include <thread>

#include <crill/platform.h>

#if CRILL_INTEL
#  include <emmintrin.h>
#elif CRILL_ARM_64BIT
#  include <arm_acle.h>
#endif

namespace crill::impl
{
#if defined(CRILL_INTEL) && CRILL_INTEL
template <std::size_t N0, std::size_t N1, std::size_t N2, typename Predicate>
void
progressive_backoff_wait_intel (Predicate &&pred)
{
  for (size_t i = 0; i < N0; ++i)
    {
      if (pred ())
        return;
    }

  for (size_t i = 0; i < N1; ++i)
    {
      if (pred ())
        return;

      _mm_pause ();
    }

  while (true)
    {
      for (size_t i = 0; i < N2; ++i)
        {
          if (pred ())
            return;

          // Do not roll these into a loop: not every compiler unrolls it
          _mm_pause ();
          _mm_pause ();
          _mm_pause ();
          _mm_pause ();
          _mm_pause ();
          _mm_pause ();
          _mm_pause ();
          _mm_pause ();
          _mm_pause ();
          _mm_pause ();
        }

      // waiting longer than we should, let's give other threads a chance to
      // recover
      std::this_thread::yield ();
    }
}
#endif // CRILL_INTEL

#if defined(CRILL_ARM_64BIT) && CRILL_ARM_64BIT
template <std::size_t N0, std::size_t N1, typename Predicate>
void
progressive_backoff_wait_armv8 (Predicate &&pred)
{
  for (size_t i = 0; i < N0; ++i)
    {
      if (pred ())
        return;
    }

  while (true)
    {
      for (size_t i = 0; i < N1; ++i)
        {
          if (pred ())
            return;

          __wfe ();
        }

      // waiting longer than we should, let's give other threads a chance to
      // recover
      std::this_thread::yield ();
    }
}
#endif // CRILL_ARM_64BIT
} // namespace crill::impl

#endif // CRILL_PROGRESSIVE_BACKOFF_WAIT_IMPL_H
