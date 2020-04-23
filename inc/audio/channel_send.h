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
 * Channel send.
 */

#ifndef __AUDIO_CHANNEL_SEND_H__
#define __AUDIO_CHANNEL_SEND_H__

#include <stdbool.h>

#include "audio/port_identifier.h"
#include "utils/yaml.h"

typedef struct StereoPorts StereoPorts;
typedef struct Track Track;
typedef struct Port Port;

/**
 * @addtogroup audio
 *
 * @{
 */

#define channel_send_is_prefader(x) (x->slot < 6)

/**
 * Channel send.
 */
typedef struct ChannelSend
{
  /** Owner track position. */
  int            track_pos;

  /** Slot index in the channel sends. */
  int            slot;

  /** Send amount (amplitude), 0 to 2 for audio. */
  float          amount;

  /** On/off for MIDI. */
  bool           on;

  /** If the send is currently empty. */
  bool           is_empty;

  /** Whether the connection is enabled or not. */
  bool           enabled;

  /** Destination L port. */
  PortIdentifier dest_l_id;

  /** Destination R port. */
  PortIdentifier dest_r_id;

  /** Destination midi port, if midi. */
  PortIdentifier dest_midi_id;

} ChannelSend;

static const cyaml_schema_field_t
channel_send_fields_schema[] =
{
  YAML_FIELD_INT (
    ChannelSend, track_pos),
  YAML_FIELD_INT (
    ChannelSend, slot),
  YAML_FIELD_FLOAT (
    ChannelSend, amount),
  YAML_FIELD_INT (
    ChannelSend, on),
  YAML_FIELD_INT (
    ChannelSend, is_empty),
  YAML_FIELD_MAPPING_EMBEDDED (
    ChannelSend, dest_l_id,
    port_identifier_fields_schema),
  YAML_FIELD_MAPPING_EMBEDDED (
    ChannelSend, dest_r_id,
    port_identifier_fields_schema),
  YAML_FIELD_MAPPING_EMBEDDED (
    ChannelSend, dest_midi_id,
    port_identifier_fields_schema),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t
channel_send_schema =
{
  CYAML_VALUE_MAPPING (
    CYAML_FLAG_DEFAULT,
    ChannelSend, channel_send_fields_schema),
};


/**
 * Inits a channel send.
 */
void
channel_send_init (
  ChannelSend * self,
  int           track_pos,
  int           slot);

Track *
channel_send_get_track (
  ChannelSend * self);

/**
 * Connects a send to stereo ports.
 */
void
channel_send_connect_stereo (
  ChannelSend * self,
  StereoPorts * stereo);

/**
 * Connects a send to a midi port.
 */
void
channel_send_connect_midi (
  ChannelSend * self,
  Port *        port);

void
channel_send_set_amount (
  ChannelSend * self,
  float         amount);

void
channel_send_set_on (
  ChannelSend * self,
  bool          on);

/**
 * Get the name of the destination.
 */
void
channel_send_get_dest_name (
  ChannelSend * self,
  char *        buf);

/**
 * @}
 */

#endif
