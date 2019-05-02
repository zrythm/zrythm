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

#ifndef __AUDIO_FADER_H__
#define __AUDIO_FADER_H__

#include "utils/yaml.h"

/**
 * @addtogroup audio
 *
 * @{
 */

typedef struct Channel Channel;

typedef struct Fader
{
  /**
   * Volume in dBFS. (-inf ~ +6)
   */
  float                volume;

  /**
   * Volume in amplitude (0.0 ~ 1.5)
   */
  float                amp;
  float                phase;        ///< used by the phase knob (0.0-360.0 value)
  float                pan; ///< (0~1) 0.5 is center

  /**
   * Current dBFS after procesing each output port.
   *
   * Transient variables only used by the GUI.
   */
  float                l_port_db;
  float                r_port_db;

  /** Owner channel. */
  Channel *            channel;
} Fader;

static const cyaml_schema_field_t
fader_fields_schema[] =
{
	CYAML_FIELD_FLOAT (
    "volume", CYAML_FLAG_DEFAULT,
    Fader, volume),
	CYAML_FIELD_FLOAT (
    "amp", CYAML_FLAG_DEFAULT,
    Fader, amp),
	CYAML_FIELD_FLOAT (
    "phase", CYAML_FLAG_DEFAULT,
    Fader, phase),
	CYAML_FIELD_FLOAT (
    "pan", CYAML_FLAG_DEFAULT,
    Fader, pan),

	CYAML_FIELD_END
};

static const cyaml_schema_value_t
fader_schema =
{
	CYAML_VALUE_MAPPING (
    CYAML_FLAG_POINTER,
	  Fader, fader_fields_schema),
};

/**
 * Inits fader to default values.
 */
void
fader_init (
  Fader * self,
  Channel * ch);

/**
 * Sets the amplitude of the fader. (0.0 to 2.0)
 */
void
fader_set_amp (
  void * self,
  float   amp);

/**
 * Adds (or subtracts if negative) to the amplitude
 * of the fader (clamped at 0.0 to 2.0).
 */
void
fader_add_amp (
  void * self,
  float   amp);

/**
 * Gets the fader amplitude (not db)
 */
float
fader_get_amp (
  void * self);

/**
 * Copy the struct members from source to dest.
 */
void
fader_copy (
  Fader * src,
  Fader * dest);

/**
 * @}
 */

#endif
