// SPDX-FileCopyrightText: © 2020-2021 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Channel send.
 */

#ifndef __AUDIO_CHANNEL_SEND_H__
#define __AUDIO_CHANNEL_SEND_H__

#include <stdbool.h>

#include "dsp/port.h"
#include "dsp/port_identifier.h"
#include "utils/yaml.h"

typedef struct StereoPorts            StereoPorts;
typedef struct Track                  Track;
typedef struct Port                   Port;
typedef struct _ChannelSendWidget     ChannelSendWidget;
typedef struct PortConnectionsManager PortConnectionsManager;

/**
 * @addtogroup dsp
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

#define channel_send_is_in_active_project(self) \
  (self->track && track_is_in_active_project (self->track))

/**
 * Target type.
 */
typedef enum ChannelSendTargetType
{
  /** Remove send. */
  CHANNEL_SEND_TARGET_TYPE_NONE,

  /** Send to track inputs. */
  CHANNEL_SEND_TARGET_TYPE_TRACK,

  /** Send to plugin sidechain inputs. */
  CHANNEL_SEND_TARGET_TYPE_PLUGIN_SIDECHAIN,
} ChannelSendTargetType;

/**
 * Send target (used in list views).
 */
typedef struct ChannelSendTarget
{
  ChannelSendTargetType type;

  int track_pos;

  PluginIdentifier pl_id;

  char * port_group;
} ChannelSendTarget;

/**
 * Returns a string describing @p self
 * (track/plugin name/etc.).
 */
char *
channel_send_target_describe (const ChannelSendTarget * self);

char *
channel_send_target_get_icon (const ChannelSendTarget * self);

void
channel_send_target_free (ChannelSendTarget * self);

/**
 * Channel send.
 *
 * The actual connection is tracked separately by
 * PortConnectionsManager.
 */
typedef struct ChannelSend
{
  int schema_version;

  /** Slot index in the channel sends. */
  int slot;

  /**
   * Stereo input if audio send.
   *
   * Prefader or fader stereo out should connect here.
   */
  StereoPorts * stereo_in;

  /**
   * MIDI input if MIDI send.
   *
   * Prefader or fader MIDI out should connect here.
   */
  Port * midi_in;

  /**
   * Stereo output if audio send.
   *
   * This should connect to the send destination, if any.
   */
  StereoPorts * stereo_out;

  /**
   * MIDI output if MIDI send.
   *
   * This should connect to the send destination, if any.
   */
  Port * midi_out;

  /** Send amount (amplitude), 0 to 2 for audio, velocity multiplier for
   * MIDI. */
  Port * amount;

  /**
   * Whether the send is currently enabled.
   *
   * If enabled, corresponding connection(s) will exist in
   * PortConnectionsManager.
   */
  Port * enabled;

  /** If the send is a sidechain. */
  bool is_sidechain;

  /** Pointer back to owner track. */
  Track * track;

  /** Track name hash (used in actions). */
  unsigned int track_name_hash;

} ChannelSend;

NONNULL_ARGS (1)
void channel_send_init_loaded (ChannelSend * self, Track * track);

/**
 * Creates a channel send instance.
 */
ChannelSend *
channel_send_new (unsigned int track_name_hash, int slot, Track * track);

/**
 * Gets the owner track.
 */
NONNULL Track *
channel_send_get_track (const ChannelSend * self);

NONNULL bool
channel_send_is_enabled (const ChannelSend * self);

#define channel_send_is_empty(x) (!channel_send_is_enabled (x))

/**
 * Returns whether the channel send target is a
 * sidechain port (rather than a target track).
 */
NONNULL bool
channel_send_is_target_sidechain (ChannelSend * self);

/**
 * Gets the target track.
 *
 * @param owner The owner track of the send
 *   (optional).
 */
Track *
channel_send_get_target_track (ChannelSend * self, Track * owner);

/**
 * Gets the target sidechain port.
 *
 * Returned StereoPorts instance must be free'd.
 */
NONNULL StereoPorts *
channel_send_get_target_sidechain (ChannelSend * self);

/**
 * Gets the amount to be used in widgets (0.0-1.0).
 */
NONNULL float
channel_send_get_amount_for_widgets (ChannelSend * self);

/**
 * Sets the amount from a widget amount (0.0-1.0).
 */
NONNULL void
channel_send_set_amount_from_widget (ChannelSend * self, float val);

/**
 * Connects a send to stereo ports.
 *
 * This function takes either \ref stereo or both
 * \ref l and \ref r.
 */
bool
channel_send_connect_stereo (
  ChannelSend * self,
  StereoPorts * stereo,
  Port *        l,
  Port *        r,
  bool          sidechain,
  bool          recalc_graph,
  bool          validate,
  GError **     error);

/**
 * Connects a send to a midi port.
 */
NONNULL bool
channel_send_connect_midi (
  ChannelSend * self,
  Port *        port,
  bool          recalc_graph,
  bool          validate,
  GError **     error);

/**
 * Removes the connection at the given send.
 */
NONNULL void
channel_send_disconnect (ChannelSend * self, bool recalc_graph);

NONNULL void
channel_send_set_amount (ChannelSend * self, float amount);

/**
 * Get the name of the destination, or a placeholder
 * text if empty.
 */
NONNULL void
channel_send_get_dest_name (ChannelSend * self, char * buf);

NONNULL void
channel_send_copy_values (ChannelSend * dest, const ChannelSend * src);

NONNULL ChannelSend *
channel_send_clone (const ChannelSend * src);

NONNULL ChannelSendWidget *
channel_send_find_widget (ChannelSend * self);

/**
 * Connects the ports to the owner track if not
 * connected.
 *
 * Only to be called on project sends.
 */
void
channel_send_connect_to_owner (ChannelSend * self);

void
channel_send_append_ports (ChannelSend * self, GPtrArray * ports);

/**
 * Appends the connection(s), if non-empty, to the
 * given array (if not NULL) and returns the number
 * of connections added.
 */
int
channel_send_append_connection (
  const ChannelSend *            self,
  const PortConnectionsManager * mgr,
  GPtrArray *                    arr);

void
channel_send_prepare_process (ChannelSend * self);

void
channel_send_process (
  ChannelSend *   self,
  const nframes_t local_offset,
  const nframes_t nframes);

/**
 * Returns whether the send is connected to the
 * given ports.
 */
bool
channel_send_is_connected_to (
  const ChannelSend * self,
  const StereoPorts * stereo,
  const Port *        midi);

/**
 * Finds the project send from a given send instance.
 */
NONNULL ChannelSend *
channel_send_find (ChannelSend * self);

NONNULL bool
channel_send_validate (ChannelSend * self);

NONNULL void
channel_send_free (ChannelSend * self);

/**
 * @}
 */

#endif
