/*
 * Copyright (C) 2020 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * \file
 *
 * Meter DSP.
 */

#ifndef __AUDIO_METER_H__
#define __AUDIO_METER_H__

#include "utils/types.h"

/**
 * @addtogroup audio
 *
 * @{
 */

typedef enum MeterAlgorithm
{
  METER_ALGORITHM_DIGITAL_PEAK,
  METER_ALGORITHM_DIGITAL_PEAK_MAX,

  /** @note True peak is intensive, only use it
   * where needed (mixer). */
  METER_ALGORITHM_TRUE_PEAK,
  METER_ALGORITHM_TRUE_PEAK_MAX,
  METER_ALGORITHM_RMS,
} MeterAlgorithm;

#endif
