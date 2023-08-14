// SPDX-FileCopyrightText: © 2020 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Fades.
 */

#ifndef __AUDIO_FADE_H__
#define __AUDIO_FADE_H__

#include "utils/yaml.h"

typedef struct CurveOptions CurveOptions;

/**
 * @addtogroup dsp
 *
 * @{
 */

/**
 * Gets the normalized Y for a normalized X.
 *
 * @param x Normalized x.
 * @param fade_in 1 for in, 0 for out.
 */
double
fade_get_y_normalized (
  double         x,
  CurveOptions * opts,
  int            fade_in);

/**
 * @}
 */

#endif
