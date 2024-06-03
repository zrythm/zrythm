// SPDX-FileCopyrightText: © 2021, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * PCGRandom float generator.
 */

#ifndef __UTILS_PCG_RAND_H__
#define __UTILS_PCG_RAND_H__

#include <cstdint>

#include "ext/juce/juce.h"

/**
 * @brief Random number generator.
 */
class PCGRand
{
public:
  PCGRand ();
  ~PCGRand () { clearSingletonInstance (); }

  /* unsigned float [0..1] */
  float uf ();

  /* signed float [-1..+1] */
  float sf ();

  uint32_t u32 ();

private:
  uint64_t _state = 0;
  uint64_t _inc = 0;

public:
  JUCE_DECLARE_SINGLETON_SINGLETHREADED (PCGRand, true)
};

#endif
