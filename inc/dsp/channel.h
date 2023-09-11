// SPDX-FileCopyrightText: © 2018-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * API for Channel, representing a channel strip on
 * the mixer.
 */

#ifndef __AUDIO_CHANNEL_H__
#define __AUDIO_CHANNEL_H__

#include "zrythm-config.h"

#include "dsp/channel_send.h"
#include "dsp/ext_port.h"
#include "dsp/fader.h"
#include "plugins/plugin.h"
#include "utils/audio.h"
#include "utils/yaml.h"

#include <gdk/gdk.h>

#ifdef HAVE_JACK
#  include "weak_libjack.h"
#endif

typedef struct AutomationTrack AutomationTrack;
typedef struct _ChannelWidget  ChannelWidget;
typedef struct Track           Track;
typedef struct _TrackWidget    TrackWidget;
typedef struct ExtPort         ExtPort;

/**
 * @addtogroup dsp
 *
 * @{
 */

#define CHANNEL_SCHEMA_VERSION 2

/** Magic number to identify channels. */
#define CHANNEL_MAGIC 8431676
#define IS_CHANNEL(x) (((Channel *) x)->magic == CHANNEL_MAGIC)
#define IS_CHANNEL_AND_NONNULL(x) (x && IS_CHANNEL (x))

#define FOREACH_STRIP for (int i = 0; i < STRIP_SIZE; i++)
#define FOREACH_AUTOMATABLE(ch) for (int i = 0; i < ch->num_automatables; i++)
#define MAX_FADER_AMP 1.42f

#define channel_is_in_active_project(self) \
  (self->track && track_is_in_active_project (self->track))

/**
 * A Channel is part of a Track (excluding Tracks that
 * don't have Channels) and contains information
 * related to routing and the Mixer.
 */
typedef struct Channel
{
  int schema_version;
  /**
   * The MIDI effect strip on instrument/MIDI tracks.
   *
   * This is processed before the instrument/inserts.
   */
  Plugin * midi_fx[STRIP_SIZE];

  /** The channel insert strip. */
  Plugin * inserts[STRIP_SIZE];

  /** The instrument plugin, if instrument track. */
  Plugin * instrument;

  /**
   * The sends strip.
   *
   * The first 6 (slots 0-5) are pre-fader and the
   * rest are post-fader.
   *
   * @note See CHANNEL_SEND_POST_FADER_START_SLOT.
   */
  ChannelSend * sends[STRIP_SIZE];

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
  ExtPort * ext_midi_ins[EXT_PORTS_MAX];
  int       num_ext_midi_ins;

  /** If 1, the channel will connect to all MIDI ins
   * found. */
  int all_midi_ins;

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
  ExtPort * ext_stereo_l_ins[EXT_PORTS_MAX];
  int       num_ext_stereo_l_ins;

  /** If 1, the channel will connect to all
   * stereo L ins found. */
  int all_stereo_l_ins;

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
  ExtPort * ext_stereo_r_ins[EXT_PORTS_MAX];
  int       num_ext_stereo_r_ins;

  /** If 1, the channel will connect to all
   * stereo R ins found. */
  int all_stereo_r_ins;

  /**
   * 1 or 0 flags for each channel to enable it or
   * disable it.
   *
   * If all_midi_channels is enabled, this is
   * ignored.
   */
  int midi_channels[16];

  /** If 1, the channel will accept MIDI messages
   * from all MIDI channels.
   */
  int all_midi_channels;

  /** The channel fader. */
  Fader * fader;

  /**
   * Prefader.
   *
   * The last plugin should connect to this.
   */
  Fader * prefader;

  /**
   * MIDI output for sending MIDI signals to other
   * destinations, such as other channels when
   * directly routed (eg MIDI track to ins track).
   */
  Port * midi_out;

  /*
   * Ports for direct (track-to-track) routing with
   * the exception of master, which will route the
   * output to monitor in.
   */
  StereoPorts * stereo_out;

  /**
   * Whether or not output_pos corresponds to
   * a Track or not.
   *
   * If not, the channel is routed to the engine.
   */
  int has_output;

  /** Output track. */
  unsigned int output_name_hash;

  /** Track associated with this channel. */
  int track_pos;

  /** Channel widget width - reserved for future
   * use. */
  int width;

  /** This must be set to CHANNEL_MAGIC. */
  int magic;

  /** The channel widget. */
  ChannelWidget * widget;

  /** Pointer to owner track. */
  Track * track;
} Channel;

static const cyaml_schema_field_t channel_fields_schema[] = {
  YAML_FIELD_INT (Channel, schema_version),
  YAML_FIELD_SEQUENCE_FIXED (Channel, midi_fx, plugin_schema, STRIP_SIZE),
  YAML_FIELD_SEQUENCE_FIXED (Channel, inserts, plugin_schema, STRIP_SIZE),
  YAML_FIELD_SEQUENCE_FIXED (Channel, sends, channel_send_schema, STRIP_SIZE),
  YAML_FIELD_MAPPING_PTR_OPTIONAL (Channel, instrument, plugin_fields_schema),
  YAML_FIELD_MAPPING_PTR (Channel, prefader, fader_fields_schema),
  YAML_FIELD_MAPPING_PTR (Channel, fader, fader_fields_schema),
  YAML_FIELD_MAPPING_PTR_OPTIONAL (Channel, midi_out, port_fields_schema),
  YAML_FIELD_MAPPING_PTR_OPTIONAL (Channel, stereo_out, stereo_ports_fields_schema),
  YAML_FIELD_UINT (Channel, has_output),
  YAML_FIELD_UINT (Channel, output_name_hash),
  YAML_FIELD_INT (Channel, track_pos),
  YAML_FIELD_FIXED_SIZE_PTR_ARRAY_VAR_COUNT (Channel, ext_midi_ins, ext_port_schema),
  YAML_FIELD_INT (Channel, all_midi_ins),
  YAML_FIELD_SEQUENCE_FIXED (Channel, midi_channels, int_schema, 16),
  YAML_FIELD_INT (Channel, all_midi_channels),
  YAML_FIELD_FIXED_SIZE_PTR_ARRAY_VAR_COUNT (
    Channel,
    ext_stereo_l_ins,
    ext_port_schema),
  YAML_FIELD_INT (Channel, all_stereo_l_ins),
  YAML_FIELD_FIXED_SIZE_PTR_ARRAY_VAR_COUNT (
    Channel,
    ext_stereo_r_ins,
    ext_port_schema),
  YAML_FIELD_INT (Channel, all_stereo_r_ins),
  YAML_FIELD_INT (Channel, width),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t channel_schema = {
  YAML_VALUE_PTR (Channel, channel_fields_schema),
};

NONNULL void
channel_init_loaded (Channel * channel, Track * track);

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
NONNULL void
channel_handle_recording (
  Channel *       self,
  const long      g_frames_start,
  const nframes_t nframes);

/**
 * Appends all channel ports and optionally
 * plugin ports to the array.
 */
void
channel_append_ports (Channel * self, GPtrArray * ports, bool include_plugins);

NONNULL void
channel_set_magic (Channel * self);

/**
 * Exposes the channel's ports to the backend.
 */
NONNULL void
channel_expose_ports_to_backend (Channel * ch);

/**
 * Connects the channel's ports.
 *
 * This should only be called on project tracks.
 */
void
channel_connect (Channel * ch);

NONNULL void
channel_set_phase (void * channel, float phase);

NONNULL float
channel_get_phase (void * channel);

NONNULL void
channel_set_balance_control (void * _channel, float pan);

/**
 * Adds to (or subtracts from) the pan.
 */
NONNULL void
channel_add_balance_control (void * _channel, float pan);

NONNULL float
channel_get_balance_control (void * _channel);

/**
 * Sets fader to 0.0.
 */
NONNULL void
channel_reset_fader (Channel * self, bool fire_events);

/**
 * Handles import (paste/drop) of plugins or plugin descriptors or mixer
 * selections.
 *
 * @param pl Passed when passing @p sel to be used for validation.
 * @param sel Plugins to import.
 * @param copy Whether to copy instead of move, when copying plugins.
 * @param ask_if_overwrite Whether to ask for permission if the import
 * overwrites existing plugins.
 */
void
channel_handle_plugin_import (
  Channel *                self,
  const Plugin *           pl,
  const MixerSelections *  sel,
  const PluginDescriptor * descr,
  int                      slot,
  PluginSlotType           slot_type,
  bool                     copy,
  bool                     ask_if_overwrite);

/**
 * Prepares the channel for processing.
 *
 * To be called before the main cycle each time on
 * all channels.
 */
NONNULL void
channel_prepare_process (Channel * channel);

/**
 * Creates a channel of the given type with the
 * given label.
 */
NONNULL Channel *
channel_new (Track * track);

/**
 * The process function prototype.
 * Channels must implement this.
 * It is used to perform processing of the audio signal at every cycle.
 *
 * Normally, the channel will call the process func on each of its plugins
 * in order.
 */
NONNULL void
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
 * @param moving_plugin Whether or not we are
 *   moving the plugin.
 * @param gen_automatables Generatate plugin
 *   automatables.
 *   To be used when creating a new plugin only.
 * @param recalc_graph Recalculate mixer graph.
 * @param pub_events Publish events.
 *
 * @return true if plugin added, false if not.
 */
NONNULL bool
channel_add_plugin (
  Channel *      channel,
  PluginSlotType slot_type,
  int            pos,
  Plugin *       plugin,
  bool           confirm,
  bool           moving_plugin,
  bool           gen_automatables,
  bool           recalc_graph,
  bool           pub_events);

NONNULL Track *
channel_get_track (Channel * self);

NONNULL Track *
channel_get_output_track (Channel * self);

/**
 * Called when the input has changed for Midi,
 * Instrument or Audio tracks.
 */
NONNULL void
channel_reconnect_ext_input_ports (Channel * ch);

/**
 * Convenience function to get the automation track
 * of the given type for the channel.
 */
NONNULL AutomationTrack *
channel_get_automation_track (Channel * channel, PortFlags port_flags);

/**
 * Removes a plugin at pos from the channel.
 *
 * FIXME this is the same as
 * modulator_track_remove_modulator().
 * TODO refactor into track_remove_plugin().
 *
 * @param moving_plugin Whether or not we are
 *   moving the plugin.
 * @param deleting_plugin Whether or not we are
 *   deleting the plugin.
 * @param deleting_channel If true, the automation
 *   tracks associated with the plugin are not
 *   deleted at this time.
 * @param recalc_graph Recalculate mixer graph.
 */
NONNULL void
channel_remove_plugin (
  Channel *      channel,
  PluginSlotType slot_type,
  int            slot,
  bool           moving_plugin,
  bool           deleting_plugin,
  bool           deleting_channel,
  bool           recalc_graph);

/**
 * Updates the track name hash in the channel and
 * all related ports and identifiers.
 */
NONNULL void
channel_update_track_name_hash (
  Channel *    self,
  unsigned int old_name_hash,
  unsigned int new_name_hash);

NONNULL int
channel_get_plugins (Channel * self, Plugin ** pls);

/**
 * Gets whether mono compatibility is enabled.
 */
NONNULL bool
channel_get_mono_compat_enabled (Channel * self);

/**
 * Sets whether mono compatibility is enabled.
 */
NONNULL void
channel_set_mono_compat_enabled (Channel * self, bool enabled, bool fire_events);

/**
 * Gets whether mono compatibility is enabled.
 */
NONNULL bool
channel_get_swap_phase (Channel * self);

/**
 * Sets whether mono compatibility is enabled.
 */
NONNULL void
channel_set_swap_phase (Channel * self, bool enabled, bool fire_events);

/**
 * @memberof Channel
 */
Plugin *
channel_get_plugin_at (const Channel * self, int slot, PluginSlotType slot_type);

/**
 * Selects/deselects all plugins in the given slot
 * type.
 */
void
channel_select_all (Channel * self, PluginSlotType type, bool select);

/**
 * Sets caches for processing.
 */
NONNULL void
channel_set_caches (Channel * self);

/**
 * Clones the channel recursively.
 *
 * @note The given track is not cloned.
 *
 * @param error To be filled if an error occurred.
 * @param track The track to use for getting the
 *   name.
 * @bool src_is_project Whether \ref ch is a project
 *   channel.
 */
NONNULL_ARGS (1, 2)
Channel * channel_clone (Channel * ch, Track * track, GError ** error);

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
NONNULL void
channel_disconnect (Channel * channel, bool remove_pl);

/**
 * Generates a context menu for either ChannelWidget
 * or FolderChannelWidget.
 */
GMenu *
channel_widget_generate_context_menu_for_track (Track * track);

/**
 * Frees the channel.
 *
 * @note Channels should never be free'd by
 * themselves in normal circumstances. Use
 * track_free() to free them.
 */
NONNULL void
channel_free (Channel * channel);

/**
 * @}
 */

#endif
