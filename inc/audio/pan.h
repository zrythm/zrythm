/*
 * Copyright (C) 2018-2019 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __AUDIO_PAN_H__
#define __AUDIO_PAN_H__

/** \file
 */

/**
 * See https://www.hackaudio.com/digital-signal-processing/stereo-audio/square-law-panning/
 */
typedef enum PanLaw
{
  PAN_LAW_0DB,
  PAN_LAW_MINUS_3DB,
  PAN_LAW_MINUS_6DB
} PanLaw;

/**
 * See https://www.harmonycentral.com/articles/the-truth-about-panning-laws
 */
typedef enum PanAlgorithm
{
  PAN_ALGORITHM_LINEAR,
  PAN_ALGORITHM_SQUARE_ROOT,
  PAN_ALGORITHM_SINE_LAW
} PanAlgorithm;

#endif
