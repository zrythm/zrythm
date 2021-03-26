/*
 * Copyright (C) 2020-2021 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, version 3.
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
typedef struct _ChannelSendWidget ChannelSendWidget;

/**
 * @addtogroup audio
 *
 * @{
 */

#define CHANNEL_SEND_SCHEMA_VERSION 1

/**
 * The slot where post-fader sends begin (starting
 * from 0).
 */
#define CHANNEL_SEND_POST_FADER_START_SLOT 6

#define channel_send_is_prefader(x) \
  (x->slot < CHANNEL_SEND_POST_FADER_START_SLOT)

/**
 * Channel send.
 */
typedef struct ChannelSend
{
  int            schema_version;

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

  /** If the send is a sidechain. */
  bool           is_sidechain;

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
  YAML_FIELD_INT (ChannelSend, schema_version),
  YAML_FIELD_INT (ChannelSend, track_pos),
  YAML_FIELD_INT (ChannelSend, slot),
  YAML_FIELD_FLOAT (ChannelSend, amount),
  YAML_FIELD_INT (ChannelSend, on),
  YAML_FIELD_INT (ChannelSend, is_empty),
  YAML_FIELD_INT (ChannelSend, is_sidechain),
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
  YAML_VALUE_DEFAULT (
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

/**
 * Gets the owner track.
 */
Track *
channel_send_get_track (
  ChannelSend * self);

/**
 * Returns whether the channel send target is a
 * sidechain port (rather than a target track).
 */
bool
channel_send_is_target_sidechain (
  ChannelSend * self);

/**
 * Gets the target track.
 */
Track *
channel_send_get_target_track (
  ChannelSend * self);

/**
 * Gets the target sidechain port.
 *
 * Returned StereoPorts instance must be free'd.
 */
StereoPorts *
channel_send_get_target_sidechain (
  ChannelSend * self);

/**
 * Gets the amount to be used in widgets (0.0-1.0).
 */
float
channel_send_get_amount_for_widgets (
  ChannelSend * self);

/**
 * Sets the amount from a widget amount (0.0-1.0).
 */
void
channel_send_set_amount_from_widget (
  ChannelSend * self,
  float         val);

/**
 * Connects a send to stereo ports.
 *
 * This function takes either \ref stereo or both
 * \ref l and \ref r.
 */
void
channel_send_connect_stereo (
  ChannelSend * self,
  StereoPorts * stereo,
  Port *        l,
  Port *        r,
  bool          sidechain);

/**
 * Connects a send to a midi port.
 */
void
channel_send_connect_midi (
  ChannelSend * self,
  Port *        port);

/**
 * Removes the connection at the given send.
 */
void
channel_send_disconnect (
  ChannelSend * self);

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

ChannelSend *
channel_send_clone (
  ChannelSend * self);

ChannelSendWidget *
channel_send_find_widget (
  ChannelSend * self);

/**
 * Finds the project send from a given send instance.
 */
ChannelSend *
channel_send_find (
  ChannelSend * self);

void
channel_send_free (
  ChannelSend * self);

/**
 * @}
 */

#endif
