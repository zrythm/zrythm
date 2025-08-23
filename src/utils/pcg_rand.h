// SPDX-FileCopyrightText: © 2021, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <cstdint>

/**
 * @brief Random number generator.
 */
class PCGRand
{
public:
  PCGRand ();

  /* unsigned float [0..1] */
  float uf ();

  /* signed float [-1..+1] */
  float sf ();

  uint32_t u32 ();

private:
  uint64_t _state = 0;
  uint64_t _inc = 0;
};
