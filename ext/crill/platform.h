// crill - the Cross-platform Real-time, I/O, and Low-Latency Library
// Copyright (c) 2022 - Timur Doumler and Fabian Renn-Giles
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)

#ifndef CRILL_PLATFORM_H
#define CRILL_PLATFORM_H

#if defined (__arm__)
  #define CRILL_ARM 1
  #define CRILL_32BIT 1
  #define CRILL_ARM_32BIT 1
#elif defined (__arm64__)
  #define CRILL_ARM 1
  #define CRILL_64BIT 1
  #define CRILL_ARM_64BIT 1
#elif defined(i386) || defined(__i386__) || defined(__i386) || defined(_M_IX86)
  #define CRILL_INTEL 1
  #define CRILL_32BIT 1
  #define CRILL_INTEL_32BIT 1
#elif defined(__x86_64__) || defined(_M_X64)
  #define CRILL_INTEL 1
  #define CRILL_64BIT 1
  #define CRILL_INTEL_64BIT 1
#endif

#endif //CRILL_PLATFORM_H

