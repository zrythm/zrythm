/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
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

#include "audio/fader.h"
#include "utils/math.h"

/**
 * Inits fader to default values.
 *
 * @param self The Fader to init.
 * @param type The FaderType.
 * @param ch Channel, if this is a channel Fader.
 */
void
fader_init (
  Fader * self,
  FaderType type,
  Channel * ch)
{
  self->type = type;
  self->channel = ch;

  /* set volume, phase, pan */
  self->volume = 0.0f;
  self->amp = 1.0f;
  self->fader_val =
    math_get_fader_val_from_amp (self->amp);
  self->phase = 0.0f;
  self->pan = 0.5f;
  self->l_port_db = 0.f;
  self->r_port_db = 0.f;
}

/**
 * Sets the amplitude of the fader. (0.0 to 2.0)
 */
void
fader_set_amp (void * _fader, float amp)
{
  Fader * self = (Fader *) _fader;
  self->amp = amp;

  /* calculate volume */
  self->volume = math_amp_to_dbfs (amp);

  self->fader_val =
    math_get_fader_val_from_amp (amp);
}

/**
 * Adds (or subtracts if negative) to the amplitude
 * of the fader (clamped at 0.0 to 2.0).
 */
void
fader_add_amp (
  void * _self,
  float   amp)
{
  Fader * self = (Fader *) _self;

  self->amp =
    CLAMP (self->amp + amp, 0.0, 2.0);

  self->fader_val =
    math_get_fader_val_from_amp (self->amp);

  self->volume =
    math_amp_to_dbfs (self->amp);
}

float
fader_get_amp (void * _self)
{
  Fader * self = (Fader *) _self;
  return self->amp;
}

/**
 * Sets the fader levels from a normalized value
 * 0.0-1.0 (such as in widgets).
 */
void
fader_set_fader_val (
  Fader * self,
  float   fader_val)
{
  self->fader_val = fader_val;
  self->amp =
    math_get_amp_val_from_fader (fader_val);
  self->volume = math_amp_to_dbfs (self->amp);
}

/**
 * Copy the struct members from source to dest.
 */
void
fader_copy (
  Fader * src,
  Fader * dest)
{
  dest->volume = src->volume;
  dest->amp = src->amp;
  dest->phase = src->phase;
  dest->pan = src->pan;
}
