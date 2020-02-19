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
 * A simple processor that copies the buffers of its
 * inputs to its outputs.
 */

#ifndef __AUDIO_PASSTHROUGH_PROCESSOR_H__
#define __AUDIO_PASSTHROUGH_PROCESSOR_H__

#include "utils/yaml.h"

typedef struct StereoPorts StereoPorts;
typedef struct Port Port;
typedef struct Channel Channel;

/**
 * @addtogroup audio
 *
 * @{
 */

typedef enum PassthroughProcessorType
{
  PP_TYPE_NONE,

  /** Audio fader for Channel's. */
  PP_TYPE_AUDIO_CHANNEL,

  /* MIDI fader for Channel's. */
  PP_TYPE_MIDI_CHANNEL,
} PassthroughProcessorType;

/**
 * A simple processor that copies the buffers of its
 * inputs to its outputs.
 *
 * Used for the pre fader processor.
 */
typedef struct PassthroughProcessor
{
  /** 0.0 ~ 1.0 for widgets. */
  float            passthrough_processor_val;

  PassthroughProcessorType type;

  /**
   * L & R audio input ports.
   */
  StereoPorts *    stereo_in;

  /**
   * L & R audio output ports.
   */
  StereoPorts *    stereo_out;

  /**
   * MIDI in port.
   */
  Port *           midi_in;

  /**
   * MIDI out port.
   */
  Port *           midi_out;

  /**
   * Current dBFS after procesing each output port.
   *
   * Transient variables only used by the GUI.
   */
  float            l_port_db;
  float            r_port_db;

  /** Track position. */
  int              track_pos;
} PassthroughProcessor;

static const cyaml_strval_t
passthrough_processor_type_strings[] =
{
  { "none",           PP_TYPE_NONE    },
  { "audio channel",  PP_TYPE_AUDIO_CHANNEL   },
  { "midi channel",   PP_TYPE_MIDI_CHANNEL   },
};

static const cyaml_schema_field_t
passthrough_processor_fields_schema[] =
{
  CYAML_FIELD_ENUM (
    "type", CYAML_FLAG_DEFAULT,
    PassthroughProcessor, type,
    passthrough_processor_type_strings,
    CYAML_ARRAY_LEN (
      passthrough_processor_type_strings)),
  CYAML_FIELD_MAPPING_PTR (
    "midi_in",
    CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL,
    PassthroughProcessor, midi_in,
    port_fields_schema),
  CYAML_FIELD_MAPPING_PTR (
    "midi_out",
    CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL,
    PassthroughProcessor, midi_out,
    port_fields_schema),
  CYAML_FIELD_MAPPING_PTR (
    "stereo_in",
    CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL,
    PassthroughProcessor, stereo_in,
    stereo_ports_fields_schema),
  CYAML_FIELD_MAPPING_PTR (
    "stereo_out",
    CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL,
    PassthroughProcessor, stereo_out,
    stereo_ports_fields_schema),
  CYAML_FIELD_INT (
    "track_pos", CYAML_FLAG_DEFAULT,
    PassthroughProcessor, track_pos),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t
passthrough_processor_schema =
{
  CYAML_VALUE_MAPPING (
    CYAML_FLAG_POINTER,
    PassthroughProcessor,
    passthrough_processor_fields_schema),
};

/**
 * Inits a PassthroughProcessor after loading a
 * project.
 */
void
passthrough_processor_init_loaded (
  PassthroughProcessor * self);

/**
 * Inits passthrough_processor to default values.
 *
 * @param self The PassthroughProcessor to init.
 * @param ch Channel.
 */
void
passthrough_processor_init (
  PassthroughProcessor * self,
  PassthroughProcessorType type,
  Channel * ch);

/**
 * Clears all buffers.
 */
void
passthrough_processor_clear_buffers (
  PassthroughProcessor * self);

/**
 * Sets the passthrough_processor levels from a normalized value
 * 0.0-1.0 (such as in widgets).
 */
void
passthrough_processor_set_widget_val (
  PassthroughProcessor * self,
  float   val);

Track *
passthrough_processor_get_track (
  PassthroughProcessor * self);

/**
 * Disconnects all ports connected to the processor.
 */
void
passthrough_processor_disconnect_all (
  PassthroughProcessor * self);

/**
 * Copy the struct members from source to dest.
 */
void
passthrough_processor_copy (
  PassthroughProcessor * src,
  PassthroughProcessor * dest);

/**
 * Process the PassthroughProcessor.
 *
 * @param g_frames The global start frames.
 * @param nframes The number of frames to process.
 */
void
passthrough_processor_process (
  PassthroughProcessor * self,
  const nframes_t g_frames,
  const nframes_t nframes);

/**
 * Updates the track pos of the fader.
 */
void
passthrough_processor_update_track_pos (
  PassthroughProcessor * self,
  int     pos);

/**
 * @}
 */

#endif
