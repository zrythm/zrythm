/*
 * Copyright (C) 2018-2020 Alexandros Theodotou <alex at zrythm dot org>
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
 * API for Channel, representing a channel strip on
 * the mixer.
 */

#ifndef __AUDIO_CHANNEL_H__
#define __AUDIO_CHANNEL_H__

#include "zrythm-config.h"

#include "audio/channel_send.h"
#include "audio/ext_port.h"
#include "audio/fader.h"
#include "plugins/plugin.h"
#include "utils/audio.h"
#include "utils/yaml.h"

#include <gdk/gdk.h>

#ifdef HAVE_JACK
#include "weak_libjack.h"
#endif

typedef struct AutomationTrack AutomationTrack;
typedef struct _ChannelWidget ChannelWidget;
typedef struct Track Track;
typedef struct _TrackWidget TrackWidget;
typedef struct ExtPort ExtPort;

/**
 * @addtogroup audio
 *
 * @{
 */

/** Magic number to identify channels. */
#define CHANNEL_MAGIC 8431676
#define IS_CHANNEL(tr) \
  (tr && tr->magic == CHANNEL_MAGIC)

#define FOREACH_STRIP for (int i = 0; i < STRIP_SIZE; i++)
#define FOREACH_AUTOMATABLE(ch) for (int i = 0; i < ch->num_automatables; i++)
#define MAX_FADER_AMP 1.42f

/**
 * A Channel is part of a Track (excluding Tracks that
 * don't have Channels) and contains information
 * related to routing and the Mixer.
 */
typedef struct Channel
{
  /**
   * The MIDI effect strip on instrument/MIDI tracks.
   *
   * This is processed before the instrument/inserts.
   */
  Plugin *         midi_fx[STRIP_SIZE];

  /** The channel insert strip. */
  Plugin *         inserts[STRIP_SIZE];

  /** The instrument plugin, if instrument track. */
  Plugin *         instrument;

  /**
   * The sends strip.
   *
   * The first 6 (slots 0-5) are pre-fader and the
   * rest are post-fader.
   *
   * @note See CHANNEL_SEND_POST_FADER_START_SLOT.
   */
  ChannelSend      sends[STRIP_SIZE];

  /**
   * External MIDI inputs that are currently
   * connected to this channel as official inputs,
   * unless all_midi_ins is enabled.
   *
   * These should be serialized every time and
   * connected to when the project gets loaded
   * if \ref Channel.all_midi_ins is not enabled.
   *
   * If all_midi_ins is enabled, these are ignored.
   */
  ExtPort *        ext_midi_ins[EXT_PORTS_MAX];
  int              num_ext_midi_ins;

  /** If 1, the channel will connect to all MIDI ins
   * found. */
  int              all_midi_ins;

  /**
   * External audio L inputs that are currently
   * connected to this channel as official inputs,
   * unless all_stereo_l_ins is enabled.
   *
   * These should be serialized every time and
   * if all_stereo_l_ins is not enabled, connected to
   * when the project gets loaded.
   *
   * If all_stereo_l_ins is enabled, these are
   * ignored.
   */
  ExtPort *        ext_stereo_l_ins[EXT_PORTS_MAX];
  int              num_ext_stereo_l_ins;

  /** If 1, the channel will connect to all
   * stereo L ins found. */
  int              all_stereo_l_ins;

  /**
   * External audio R inputs that are currently
   * connected to this channel as official inputs,
   * unless all_stereo_r_ins is enabled.
   *
   * These should be serialized every time and
   * if all_stereo_r_ins is not enabled, connected to
   * when the project gets loaded.
   *
   * If all_stereo_r_ins is enabled, these are
   * ignored.
   */
  ExtPort *        ext_stereo_r_ins[EXT_PORTS_MAX];
  int              num_ext_stereo_r_ins;

  /** If 1, the channel will connect to all
   * stereo R ins found. */
  int              all_stereo_r_ins;

  /**
   * 1 or 0 flags for each channel to enable it or
   * disable it.
   *
   * If all_midi_channels is enabled, this is
   * ignored.
   */
  int              midi_channels[16];

  /** If 1, the channel will accept MIDI messages
   * from all MIDI channels.
   */
  int              all_midi_channels;

  /** The channel fader. */
  Fader *          fader;

  /**
   * Prefader.
   *
   * The last plugin should connect to this.
   */
  Fader *          prefader;

  /**
   * MIDI output for sending MIDI signals to other
   * destinations, such as other channels when
   * directly routed (eg MIDI track to ins track).
   */
  Port *           midi_out;

  /*
   * Ports for direct (track-to-track) routing with
   * the exception of master, which will route the
   * output to monitor in.
   */
  StereoPorts *    stereo_out;

  /**
   * Whether or not output_pos corresponds to
   * a Track or not.
   *
   * If not, the channel is routed to the engine.
   */
  int              has_output;

  /** Output track. */
  int              output_pos;

  /** Track associated with this channel. */
  int              track_pos;

  /** This must be set to CHANNEL_MAGIC. */
  int              magic;

  /** The channel widget. */
  ChannelWidget *  widget;

  /**
   * Whether record was set automatically when
   * the channel was selected.
   *
   * This is so that it can be unset when selecting
   * another channel. If we don't do this all the
   * channels end up staying on record mode.
   */
  int              record_set_automatically;

  /**
   * Pointer back to Track.
   *
   * This is an exception to most other objects
   * since each channel is always fixed to the same
   * track.
   */
  Track *          track;
} Channel;

static const cyaml_schema_field_t
channel_fields_schema[] =
{
  YAML_FIELD_SEQUENCE_FIXED (
    Channel, midi_fx,
    plugin_schema, STRIP_SIZE),
  YAML_FIELD_SEQUENCE_FIXED (
    Channel, inserts,
    plugin_schema, STRIP_SIZE),
  YAML_FIELD_SEQUENCE_FIXED (
    Channel, sends,
    channel_send_schema, STRIP_SIZE),
  YAML_FIELD_MAPPING_PTR_OPTIONAL (
    Channel, instrument, plugin_fields_schema),
  YAML_FIELD_MAPPING_PTR (
    Channel, prefader,
    fader_fields_schema),
  YAML_FIELD_MAPPING_PTR (
    Channel, fader, fader_fields_schema),
  YAML_FIELD_MAPPING_PTR_OPTIONAL (
    Channel, midi_out,
    port_fields_schema),
  YAML_FIELD_MAPPING_PTR_OPTIONAL (
    Channel, stereo_out,
    stereo_ports_fields_schema),
  YAML_FIELD_INT (
    Channel, has_output),
  YAML_FIELD_INT (
    Channel, output_pos),
  YAML_FIELD_INT (
    Channel, track_pos),
  CYAML_FIELD_SEQUENCE_COUNT (
    "ext_midi_ins", CYAML_FLAG_DEFAULT,
    Channel, ext_midi_ins, num_ext_midi_ins,
    &ext_port_schema, 0, CYAML_UNLIMITED),
  YAML_FIELD_INT (
    Channel, all_midi_ins),
  CYAML_FIELD_SEQUENCE_FIXED (
    "midi_channels", CYAML_FLAG_DEFAULT,
    Channel, midi_channels,
    &int_schema, 16),
  CYAML_FIELD_SEQUENCE_COUNT (
    "ext_stereo_l_ins", CYAML_FLAG_DEFAULT,
    Channel, ext_stereo_l_ins, num_ext_stereo_l_ins,
    &ext_port_schema, 0, CYAML_UNLIMITED),
  YAML_FIELD_INT (
    Channel, all_stereo_l_ins),
  CYAML_FIELD_SEQUENCE_COUNT (
    "ext_stereo_r_ins", CYAML_FLAG_DEFAULT,
    Channel, ext_stereo_r_ins, num_ext_stereo_r_ins,
    &ext_port_schema, 0, CYAML_UNLIMITED),
  YAML_FIELD_INT (
    Channel, all_stereo_r_ins),
  YAML_FIELD_INT (
    Channel, all_midi_channels),
  YAML_FIELD_INT (
    Channel, record_set_automatically),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t
channel_schema =
{
  CYAML_VALUE_MAPPING (
    CYAML_FLAG_POINTER,
    Channel, channel_fields_schema),
};

void
channel_init_loaded (
  Channel * channel,
  bool      project);

/**
 * Handles the recording logic inside the process
 * cycle.
 *
 * The MidiEvents are already dequeued at this
 * point.
 *
 * @param g_frames_start Global start frames.
 * @param nframes Number of frames to process.
 */
void
channel_handle_recording (
  Channel *       self,
  const long      g_frames_start,
  const nframes_t nframes);

/**
 * Appends all channel ports and optionally
 * plugin ports to the array.
 *
 * @param size Current array count.
 * @param is_dynamic Whether the array can be
 *   dynamically resized.
 * @param max_size Current array size, if dynamic.
 */
void
channel_append_all_ports (
  Channel * ch,
  Port ***  ports,
  int *     size,
  bool      is_dynamic,
  int *     max_size,
  bool      include_plugins);

/**
 * Exposes the channel's ports to the backend.
 */
void
channel_expose_ports_to_backend (
  Channel * ch);

/**
 * Connects the channel's ports.
 *
 * This should only be called on new tracks.
 */
void
channel_connect (
  Channel * ch);

void
channel_set_phase (void * channel, float phase);

float
channel_get_phase (void * channel);


void
channel_set_balance_control (
  void * _channel, float pan);

/**
 * Adds to (or subtracts from) the pan.
 */
void
channel_add_balance_control (
  void * _channel, float pan);

float
channel_get_balance_control (void * _channel);

#if 0
/* ---- getters ---- */

/**
 * MIDI peak.
 *
 * @note Used by the UI.
 */
float
channel_get_current_midi_peak (
  void * _channel);

/**
 * Digital peak.
 *
 * @note Used by the UI.
 */
float
channel_get_current_l_digital_peak (
  void * _channel);

/**
 * Digital peak.
 *
 * @note Used by the UI.
 */
float
channel_get_current_r_digital_peak (
  void * _channel);

/**
 * Digital peak.
 *
 * @note Used by the UI.
 */
float
channel_get_current_l_digital_peak_max (
  void * _channel);

/**
 * Digital peak.
 *
 * @note Used by the UI.
 */
float
channel_get_current_r_digital_peak_max (
  void * _channel);

/* ---- end getters ---- */

void
channel_set_current_l_db (
  Channel * channel, float val);

void
channel_set_current_r_db (
  Channel * channel, float val);

#endif

/**
 * Sets fader to 0.0.
 */
void
channel_reset_fader (Channel * channel);

/**
 * Prepares the channel for processing.
 *
 * To be called before the main cycle each time on
 * all channels.
 */
void
channel_prepare_process (Channel * channel);

/**
 * Creates a channel of the given type with the
 * given label.
 */
Channel *
channel_new (
  Track * track);

/**
 * The process function prototype.
 * Channels must implement this.
 * It is used to perform processing of the audio signal at every cycle.
 *
 * Normally, the channel will call the process func on each of its plugins
 * in order.
 */
void
channel_process (Channel * channel);

/**
 * Adds given plugin to given position in the strip.
 *
 * The plugin must be already instantiated at this
 * point.
 *
 * @param channel The Channel.
 * @param pos The position in the strip starting
 *   from 0.
 * @param plugin The plugin to add.
 * @param confirm Confirm if an existing plugin
 *   will be overwritten.
 * @param gen_automatables Generatate plugin
 *   automatables.
 *   To be used when creating a new plugin only.
 * @param recalc_graph Recalculate mixer graph.
 * @param pub_events Publish events.
 *
 * @return 1 if plugin added, 0 if not.
 */
int
channel_add_plugin (
  Channel * channel,
  PluginSlotType slot_type,
  int       pos,
  Plugin *  plugin,
  int       confirm,
  int       gen_automatables,
  int       recalc_graph,
  int       pub_events);

Track *
channel_get_track (
  Channel * self);

Track *
channel_get_output_track (
  Channel * self);

/**
 * Called when the input has changed for Midi,
 * Instrument or Audio tracks.
 */
void
channel_reconnect_ext_input_ports (
  Channel * ch);

/**
 * Connects or disconnects the MIDI editor key press port to the channel's
 * first plugin
 */
void
channel_reattach_midi_editor_manual_press_port (
  Channel * channel,
  int       connect,
  const int recalc_graph);

/**
 * Convenience function to get the automation track
 * of the given type for the channel.
 */
AutomationTrack *
channel_get_automation_track (
  Channel *       channel,
  PortFlags       type);

/**
 * Removes a plugin at pos from the channel.
 *
 * If deleting_channel is 1, the automation tracks
 * associated with he plugin are not deleted at
 * this time.
 *
 * This function will always recalculate the graph
 * in order to avoid situations where the plugin
 * might be used during processing.
 *
 * @param deleting_plugin
 * @param deleting_channel
 * @param recalc_graph Recalculate mixer graph.
 */
void
channel_remove_plugin (
  Channel * channel,
  PluginSlotType type,
  int       pos,
  bool      deleting_plugin,
  bool      deleting_channel,
  bool      recalc_graph);

/**
 * Updates the track position in the channel and
 * all related ports and identifiers.
 */
void
channel_update_track_pos (
  Channel * self,
  int       pos);

int
channel_get_plugins (
  Channel * self,
  Plugin ** pls);

/**
 * Clones the channel recursively.
 *
 * @note The given track is not cloned.
 *
 * @param track The track to use for getting the
 *   name.
 * @bool src_is_project Whether \ref ch is a project
 *   channel.
 */
Channel *
channel_clone (
  Channel * ch,
  Track *   track,
  bool      src_is_project);

/**
 * Disconnects the channel from the processing
 * chain.
 *
 * This should be called immediately when the
 * channel is getting deleted, and channel_free
 * should be designed to be called later after
 * an arbitrary delay.
 *
 * @param remove_pl Remove the Plugin from the
 *   Channel. Useful when deleting the channel.
 * @param recalc_graph Recalculate mixer graph.
 */
void
channel_disconnect (
  Channel * channel,
  bool      remove_pl);

/**
 * Frees the channel.
 *
 * @note Channels should never be free'd by
 * themselves in normal circumstances. Use
 * track_free() to free them.
 */
void
channel_free (Channel * channel);

/**
 * @}
 */

#endif
