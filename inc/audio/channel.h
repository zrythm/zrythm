/*
 * Copyright (C) 2018-2019 Alexandros Theodotou <alex at zrythm dot org>
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

#ifndef __AUDIO_CHANNEL_H__
#define __AUDIO_CHANNEL_H__

/**
 * \file
 *
 * API for Channel, representing a channel strip on
 * the mixer.
 */

#include "config.h"

#include "audio/automatable.h"
#include "audio/ext_port.h"
#include "audio/fader.h"
#include "audio/passthrough_processor.h"
#include "plugins/plugin.h"
#include "utils/audio.h"
#include "utils/yaml.h"

#include <gdk/gdk.h>

#ifdef HAVE_JACK
#include <jack/jack.h>
#endif

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
#define channel_get_fader_automatable(ch) \
  channel_get_automatable (\
    ch, AUTOMATABLE_TYPE_CHANNEL_FADER)

typedef struct _ChannelWidget ChannelWidget;
typedef struct Track Track;
typedef struct _TrackWidget TrackWidget;
typedef struct ExtPort ExtPort;

/**
 * A Channel is part of a Track (excluding Tracks that
 * don't have Channels) and contains information
 * related to routing and the Mixer.
 */
typedef struct Channel
{
  /**
   * The channel strip.
   *
   * Note: the first plugin is special in MIDI
   * channels.
   */
  Plugin *         plugins[STRIP_SIZE];

  /**
   * A subset of the automation tracks in the
   * automation tracklist of the track of this
   * channel.
   *
   * These are not meant to be serialized.
   */
  AutomationTrack ** ats;
  int                num_ats;
  size_t             ats_size;

  /**
   * External MIDI inputs that are currently
   * connected to this channel as official inputs,
   * unless all_midi_ins is enabled.
   *
   * These should be serialized every time and
   * if all_midi_ins is not enabled, connected to
   * when the project gets loaded.
   *
   * If all_midi_ins is enabled, these are ignored.
   */
  ExtPort *         ext_midi_ins[EXT_PORTS_MAX];
  int               num_ext_midi_ins;

  /** If 1, the channel will connect to all MIDI ins
   * found. */
  int               all_midi_ins;

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
  ExtPort *         ext_stereo_l_ins[EXT_PORTS_MAX];
  int               num_ext_stereo_l_ins;

  /** If 1, the channel will connect to all
   * stereo L ins found. */
  int               all_stereo_l_ins;

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
  ExtPort *         ext_stereo_r_ins[EXT_PORTS_MAX];
  int               num_ext_stereo_r_ins;

  /** If 1, the channel will connect to all
   * stereo R ins found. */
  int               all_stereo_r_ins;

  /**
   * 1 or 0 flags for each channel to enable it or
   * disable it.
   *
   * If all_midi_channels is enabled, this is
   * ignored.
   */
  int               midi_channels[16];

  /** If 1, the channel will accept MIDI messages
   * from all MIDI channels.
   */
  int               all_midi_channels;

  /** The channel fader. */
  Fader            fader;

  /**
   * Prefader.
   *
   * The last plugin should connect to this.
   */
  PassthroughProcessor prefader;

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

  /** Output channel to route signal to. */
  Track *          output;
  /** For serializing. */
  int              output_pos;

  /** Track associated with this channel. */
  Track *          track;

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
  int                  record_set_automatically;
} Channel;

static const cyaml_schema_field_t
channel_fields_schema[] =
{
  CYAML_FIELD_SEQUENCE_FIXED (
    "plugins",
    CYAML_FLAG_DEFAULT,
    Channel, plugins,
    &plugin_schema, STRIP_SIZE),
  CYAML_FIELD_MAPPING (
    "prefader", CYAML_FLAG_DEFAULT,
    Channel, prefader,
    passthrough_processor_fields_schema),
  CYAML_FIELD_MAPPING (
    "fader", CYAML_FLAG_DEFAULT,
    Channel, fader, fader_fields_schema),
  CYAML_FIELD_MAPPING_PTR (
    "midi_out",
    CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL,
    Channel, midi_out,
    port_fields_schema),
  CYAML_FIELD_MAPPING_PTR (
    "stereo_out",
    CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL,
    Channel, stereo_out,
    stereo_ports_fields_schema),
  CYAML_FIELD_INT (
    "output_pos", CYAML_FLAG_DEFAULT,
    Channel, output_pos),
  CYAML_FIELD_SEQUENCE_COUNT (
    "ext_midi_ins", CYAML_FLAG_DEFAULT,
    Channel, ext_midi_ins, num_ext_midi_ins,
    &ext_port_schema, 0, CYAML_UNLIMITED),
  CYAML_FIELD_INT (
    "all_midi_ins", CYAML_FLAG_DEFAULT,
    Channel, all_midi_ins),
  CYAML_FIELD_SEQUENCE_FIXED (
    "midi_channels", CYAML_FLAG_DEFAULT,
    Channel, midi_channels,
    &int_schema, 16),
  CYAML_FIELD_SEQUENCE_COUNT (
    "ext_stereo_l_ins", CYAML_FLAG_DEFAULT,
    Channel, ext_stereo_l_ins, num_ext_stereo_l_ins,
    &ext_port_schema, 0, CYAML_UNLIMITED),
  CYAML_FIELD_INT (
    "all_stereo_l_ins", CYAML_FLAG_DEFAULT,
    Channel, all_stereo_l_ins),
  CYAML_FIELD_SEQUENCE_COUNT (
    "ext_stereo_r_ins", CYAML_FLAG_DEFAULT,
    Channel, ext_stereo_r_ins, num_ext_stereo_r_ins,
    &ext_port_schema, 0, CYAML_UNLIMITED),
  CYAML_FIELD_INT (
    "all_stereo_r_ins", CYAML_FLAG_DEFAULT,
    Channel, all_stereo_r_ins),
  CYAML_FIELD_INT (
    "all_midi_channels", CYAML_FLAG_DEFAULT,
    Channel, all_midi_channels),
  CYAML_FIELD_INT (
    "record_set_automatically", CYAML_FLAG_DEFAULT,
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
channel_init_loaded (Channel * channel);

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
 * The array must be large enough.
 */
void
channel_append_all_ports (
  Channel * ch,
  Port ** ports,
  int *   size,
  int     include_plugins);

/**
 * Exposes the channel's ports to the backend.
 */
void
channel_expose_ports_to_backend (
  Channel * ch);

/**
 * Connects the channel's ports.
 */
void
channel_connect (
  Channel * ch);

void
channel_set_phase (void * channel, float phase);

float
channel_get_phase (void * channel);


void
channel_set_pan (void * _channel, float pan);

/**
 * Adds to (or subtracts from) the pan.
 */
void
channel_add_pan (void * _channel, float pan);

float
channel_get_pan (void * _channel);

float
channel_get_current_l_db (void * _channel);

float
channel_get_current_r_db (void * _channel);

void
channel_set_current_l_db (
  Channel * channel, float val);

void
channel_set_current_r_db (
  Channel * channel, float val);

/**
 * Prepares the Channel for serialization.
 */
void
channel_prepare_for_serialization (
  Channel * ch);

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
 * Removes the AutomationTrack's associated with
 * this channel from the AutomationTracklist in the
 * corresponding Track.
 */
void
channel_remove_ats_from_automation_tracklist (
  Channel * ch);

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
 *
 * @return 1 if plugin added, 0 if not.
 */
int
channel_add_plugin (
  Channel * channel,
  int       pos,
  Plugin *  plugin,
  int       confirm,
  int       gen_automatables,
  int       recalc_graph);

/**
 * Updates the output of the Channel (where the
 * Channel routes to.
 */
void
channel_update_output (
  Channel * ch,
  Track * output);

/**
 * Called when the input has changed for Midi,
 * Instrument or Audio tracks.
 */
void
channel_reconnect_ext_input_ports (
  Channel * ch);

/**
 * Returns the index of the last active slot.
 */
int
channel_get_last_active_slot_index (
  Channel * channel);

/**
 * Returns the last Plugin in the strip.
 */
Plugin *
channel_get_last_plugin (
  Channel * self);

/**
 * Returns the index on the mixer.
 */
int
channel_get_index (Channel * channel);

/**
 * Returns the plugin's strip index on the channel
 */
int
channel_get_plugin_index (Channel * channel,
                          Plugin *  plugin);

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
 * Convenience method to get the first active plugin in the channel
 */
Plugin *
channel_get_first_plugin (Channel * channel);

/**
 * Convenience function to get the fader automatable of the channel.
 */
Automatable *
channel_get_automatable (
  Channel *       channel,
  AutomatableType type);

/**
 * Convenience function to get the automation track
 * of the given type for the channel.
 */
AutomationTrack *
channel_get_automation_track (
  Channel *       channel,
  AutomatableType type);

/**
 * Generates automatables for the channel.
 *
 * Should be called as soon as the track is
 * created.
 */
void
channel_generate_automation_tracks (
  Channel * channel);

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
  int pos,
  int deleting_plugin,
  int deleting_channel,
  int recalc_graph);

/**
 * Clones the channel recursively.
 *
 * @param track The track to use for getting the name.
 */
Channel *
channel_clone (
  Channel * ch,
  Track *   track);

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
  int       remove_pl,
  int       recalc_graph);

/**
 * Frees the channel.
 *
 * Channels should never be free'd by themselves
 * in normal circumstances. Use track_free to
 * free them.
 */
void
channel_free (Channel * channel);

/**
 * @}
 */

#endif
