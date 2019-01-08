/*
 * audio/pan.h - panning
 *
 * copyright (c) 2018 alexandros theodotou
 *
 * this file is part of zrythm
 *
 * zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the gnu general public license as published by
 * the free software foundation, either version 3 of the license, or
 * (at your option) any later version.
 *
 * zrythm is distributed in the hope that it will be useful,
 * but without any warranty; without even the implied warranty of
 * merchantability or fitness for a particular purpose.  see the
 * gnu general public license for more details.
 *
 * you should have received a copy of the gnu general public license
 * along with zrythm.  if not, see <https://www.gnu.org/licenses/>.
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
