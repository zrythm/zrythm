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

#include <gdk/gdk.h>

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

/** Magic number to identify channels. */
#define CHANNEL_MAGIC 8431676
#define IS_CHANNEL(x) (((Channel *) x)->magic == CHANNEL_MAGIC)
#define IS_CHANNEL_AND_NONNULL(x) (x && IS_CHANNEL (x))

#define FOREACH_STRIP for (int i = 0; i < STRIP_SIZE; i++)
#define FOREACH_AUTOMATABLE(ch) for (int i = 0; i < ch->num_automatables; i++)
#define MAX_FADER_AMP 1.42f

/**
 * A Channel is always part of a Track (excluding Tracks that don't have
 * Channels) and contains information related to routing and the Mixer.
 */
struct Channel
{
  Channel () = delete;
  explicit Channel (Track &track);
  ~Channel ();

  bool is_in_active_project ();

  /**
   * Sets fader to 0.0.
   */
  void reset_fader (bool fire_events);

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
  void handle_plugin_import (
    const Plugin *           pl,
    const MixerSelections *  sel,
    const PluginDescriptor * descr,
    int                      slot,
    ZPluginSlotType          slot_type,
    bool                     copy,
    bool                     ask_if_overwrite);

  /**
   * @brief Prepares the channel for processing.
   *
   * To be called before the main cycle each time on all channels.
   */
  void prepare_process ();

  /**
   * @brief Perform processing of the audio signal.
   *
   * Normally, the channel will call the process func on each of its plugins
   * in order.
   */
  void process ();

  /**
   * Adds given plugin to given position in the strip.
   *
   * The plugin must be already instantiated at this point.
   *
   * @param channel The Channel.
   * @param pos The position in the strip starting from 0.
   * @param plugin The plugin to add.
   * @param confirm Confirm if an existing plugin will be overwritten.
   * @param moving_plugin Whether or not we are moving the plugin.
   * @param gen_automatables Generatate plugin automatables. To be used when
   * creating a new plugin only.
   * @param recalc_graph Recalculate mixer graph.
   * @param pub_events Publish events.
   *
   * @return true if plugin added, false if not.
   */
  NONNULL bool add_plugin (
    ZPluginSlotType slot_type,
    int             pos,
    Plugin *        plugin,
    bool            confirm,
    bool            moving_plugin,
    bool            gen_automatables,
    bool            recalc_graph,
    bool            pub_events);

  Track &get_track ();

  Track * get_output_track ();

  /**
   * Called when the input has changed for Midi, Instrument or Audio tracks.
   */
  void reconnect_ext_input_ports ();

  /**
   * Convenience function to get the automation track of the given type for the
   * channel.
   */
  AutomationTrack * get_automation_track (PortIdentifier::Flags port_flags);

  /**
   * Removes a plugin at the given slot from the channel.
   *
   * FIXME this is the same as modulator_track_remove_modulator().
   * TODO refactor into track_remove_plugin().
   *
   * @param moving_plugin Whether or not we are moving the plugin.
   * @param deleting_plugin Whether or not we are deleting the plugin.
   * @param deleting_channel If true, the automation tracks associated with the
   * plugin are not deleted at this time.
   * @param recalc_graph Recalculate mixer graph.
   */
  void remove_plugin (
    ZPluginSlotType slot_type,
    int             slot,
    bool            moving_plugin,
    bool            deleting_plugin,
    bool            deleting_channel,
    bool            recalc_graph);

  /**
   * Updates the track name hash in the channel and all related ports and
   * identifiers.
   */
  void
  update_track_name_hash (unsigned int old_name_hash, unsigned int new_name_hash);

  NONNULL int get_plugins (Plugin ** pls);

  /**
   * Gets whether mono compatibility is enabled.
   */
  bool get_mono_compat_enabled ();

  /**
   * Sets whether mono compatibility is enabled.
   */
  void set_mono_compat_enabled (bool enabled, bool fire_events);

  /**
   * Gets whether mono compatibility is enabled.
   */
  bool get_swap_phase ();

  /**
   * Sets whether mono compatibility is enabled.
   */
  void set_swap_phase (bool enabled, bool fire_events);

  Plugin * get_plugin_at (int slot, ZPluginSlotType slot_type) const;

  /**
   * Selects/deselects all plugins in the given slot type.
   */
  void select_all (ZPluginSlotType type, bool select);

  /**
   * Sets caches for processing.
   */
  void set_caches ();

  /**
   * Clones the channel recursively.
   *
   * @param error To be filled if an error occurred.
   * @param track The track to use for getting the name. This track is not
   * cloned.
   */
  Channel * clone (Track &track, GError ** error);

  /**
   * Disconnects the channel from the processing chain.
   *
   * This should be called immediately when the channel is getting deleted, and
   * channel_free should be designed to be called later after an arbitrary delay.
   *
   * @param remove_pl Remove the Plugin from the Channel. Useful when deleting
   * the channel.
   * @param recalc_graph Recalculate mixer graph.
   */
  void disconnect (bool remove_pl);

  void init_loaded (Track &track);

  /**
   * Handles the recording logic inside the process cycle.
   *
   * The MidiEvents are already dequeued at this point.
   *
   * @param g_frames_start Global start frames.
   * @param nframes Number of frames to process.
   */
  void handle_recording (const long g_frames_start, const nframes_t nframes);

  /**
   * Appends all channel ports and optionally plugin ports to the array.
   */
  void append_ports (GPtrArray * ports, bool include_plugins);

  /**
   * @brief Sets the magic member on owned plugins.
   */
  void set_magic ();

  /**
   * Exposes the channel's ports to the backend.
   */
  void expose_ports_to_backend ();

  /**
   * Connects the channel's ports.
   *
   * This should only be called on project tracks.
   */
  void connect ();

  static void set_phase (void * channel, float phase);

  static float get_phase (void * channel);

  static void set_balance_control (void * _channel, float pan);

  void set_balance_control (float val);

  /**
   * Adds to (or subtracts from) the pan.
   */
  static void add_balance_control (void * _channel, float pan);

  static float get_balance_control (void * _channel);

  float get_balance_control () const;

  /**
   * The MIDI effect strip on instrument/MIDI tracks.
   *
   * This is processed before the instrument/inserts.
   */
  Plugin * midi_fx[STRIP_SIZE] = {};

  /** The channel insert strip. */
  Plugin * inserts[STRIP_SIZE] = {};

  /** The instrument plugin, if instrument track. */
  Plugin * instrument = nullptr;

  /**
   * The sends strip.
   *
   * The first 6 (slots 0-5) are pre-fader and the rest are post-fader.
   *
   * @note See CHANNEL_SEND_POST_FADER_START_SLOT.
   */
  ChannelSend * sends[STRIP_SIZE] = {};

  /**
   * External MIDI inputs that are currently connected to this channel as
   * official inputs, unless all_midi_ins is enabled.
   *
   * These should be serialized every time and connected to when the project
   * gets loaded if \ref Channel.all_midi_ins is not enabled.
   *
   * If all_midi_ins is enabled, these are ignored.
   */
  ExtPort * ext_midi_ins[EXT_PORTS_MAX] = {};
  int       num_ext_midi_ins = 0;

  /** If true, the channel will connect to all MIDI ins found. */
  bool all_midi_ins = false;

  /**
   * External audio L inputs that are currently connected to this channel as
   * official inputs, unless all_stereo_l_ins is enabled.
   *
   * These should be serialized every time and if all_stereo_l_ins is not
   * enabled, connected to when the project gets loaded.
   *
   * If all_stereo_l_ins is enabled, these are ignored.
   */
  ExtPort * ext_stereo_l_ins[EXT_PORTS_MAX] = {};
  int       num_ext_stereo_l_ins = 0;

  /** If true, the channel will connect to all stereo L ins found. */
  bool all_stereo_l_ins = false;

  /**
   * External audio R inputs that are currently connected to this channel as
   * official inputs, unless all_stereo_r_ins is enabled.
   *
   * These should be serialized every time and if all_stereo_r_ins is not
   * enabled, connected to when the project gets loaded.
   *
   * If all_stereo_r_ins is enabled, these are ignored.
   */
  ExtPort * ext_stereo_r_ins[EXT_PORTS_MAX] = {};
  int       num_ext_stereo_r_ins = 0;

  /** If true, the channel will connect to all stereo R ins found. */
  bool all_stereo_r_ins = false;

  /**
   * 1 or 0 flags for each channel to enable it or disable it.
   *
   * If all_midi_channels is enabled, this is ignored.
   */
  int midi_channels[16] = {};

  /** If true, the channel will accept MIDI messages from all MIDI channels.
   */
  bool all_midi_channels = false;

  /** The channel fader. */
  Fader * fader = nullptr;

  /**
   * Prefader.
   *
   * The last plugin should connect to this.
   */
  Fader * prefader = nullptr;

  /**
   * MIDI output for sending MIDI signals to other destinations, such as other
   * channels when directly routed (eg MIDI track to ins track).
   */
  Port * midi_out = nullptr;

  /*
   * Ports for direct (track-to-track) routing with the exception of master,
   * which will route the output to monitor in.
   */
  StereoPorts * stereo_out = nullptr;

  /**
   * Whether or not output_pos corresponds to a Track or not.
   *
   * If not, the channel is routed to the engine.
   */
  bool has_output = false;

  /** Output track. */
  unsigned int output_name_hash = 0;

  /** Track associated with this channel. */
  int track_pos = 0;

  /** Channel widget width - reserved for future use. */
  int width = 0;

  /** This must be set to CHANNEL_MAGIC. */
  int magic = 0;

  /** The channel widget. */
  ChannelWidget * widget = nullptr;

  /** Owner track. */
  Track &track_;
};

/**
 * @}
 */

#endif
