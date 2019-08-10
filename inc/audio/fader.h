/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
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
 * Backend for faders or other volume/gain controls.
 */

#ifndef __AUDIO_FADER_H__
#define __AUDIO_FADER_H__

#include "utils/yaml.h"

typedef struct StereoPorts StereoPorts;

/**
 * @addtogroup audio
 *
 * @{
 */

typedef enum FaderType
{
  /** For Channel's. */
  FADER_TYPE_CHANNEL,

  /** For generic uses. */
  FADER_TYPE_GENERIC,
} FaderType;

typedef struct Channel Channel;

/**
 * A Fader is a processor that is used for volume
 * controls and pan.
 *
 * It does not necessarily have to correspond to
 * a FaderWidget. It can be used as a backend to
 * KnobWidget's.
 */
typedef struct Fader
{
  /**
   * Volume in dBFS. (-inf ~ +6)
   */
  float            volume;

  /**
   * Volume in amplitude (0.0 ~ 1.5)
   */
  float            amp;

  /** Used by the phase knob (0.0 ~ 360.0). */
  float            phase;

  /** (0.0 ~ 1.0) 0.5 is center. */
  float            pan;

  /** 0.0 ~ 1.0 for widgets. */
  float            fader_val;

  /**
   * L & R audio input ports.
   */
  StereoPorts *    stereo_in;

  /**
   * L & R audio output ports.
   */
  StereoPorts *    stereo_out;

  /**
   * Current dBFS after procesing each output port.
   *
   * Transient variables only used by the GUI.
   */
  float            l_port_db;
  float            r_port_db;

  FaderType        type;

  /** Owner channel, if channel fader. */
  Channel *        channel;
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
 *
 * @param self The Fader to init.
 * @param type The FaderType.
 * @param ch Channel, if this is a channel Fader.
 */
void
fader_init (
  Fader * self,
  FaderType type,
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
 * FIXME is void * necessary? do it in the caller.
 */
float
fader_get_amp (
  void * self);

/**
 * Sets the fader levels from a normalized value
 * 0.0-1.0 (such as in widgets).
 */
void
fader_set_fader_val (
  Fader * self,
  float   fader_val);

/**
 * Copy the struct members from source to dest.
 */
void
fader_copy (
  Fader * src,
  Fader * dest);

/**
 * Process the Fader.
 *
 * @param g_frames The global start frames.
 * @param nframes The number of frames to process.
 */
void
fader_process (
  Fader * self,
  long    g_frames,
  int     nframes);

/**
 * @}
 */

#endif
