// SPDX-FileCopyrightText: © 2018-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __AUDIO_CHANNEL_H__
#define __AUDIO_CHANNEL_H__

#include "zrythm-config.h"

#include "common/dsp/channel_send.h"
#include "common/dsp/ext_port.h"
#include "common/dsp/fader.h"
#include "common/plugins/plugin.h"
#include "common/utils/audio.h"
#include "common/utils/icloneable.h"
#include <gdk/gdk.h>

class AutomationTrack;
class ChannelTrack;
class GroupTargetTrack;
TYPEDEF_STRUCT_UNDERSCORED (ChannelWidget);
TYPEDEF_STRUCT_UNDERSCORED (TrackWidget);
class ExtPort;

/**
 * @addtogroup dsp
 *
 * @{
 */

/** Magic number to identify channels. */
static constexpr int CHANNEL_MAGIC = 8431676;

static constexpr float MAX_FADER_AMP = 1.42f;

#define IS_CHANNEL(x) (((Channel *) x)->magic_ == CHANNEL_MAGIC)
#define IS_CHANNEL_AND_NONNULL(x) (x && IS_CHANNEL (x))

/**
 * @brief Represents a channel strip on the mixer.
 *
 * The Channel class encapsulates the functionality of a channel strip,
 * including its plugins, fader, sends, and other properties. It provides
 * methods for managing the channel's state and processing the audio signal.
 *
 * Channels are owned by Tracks and handle the output part of the signal chain,
 * while TrackProcessor handles the input part.
 *
 * @see Track
 */
class Channel final
    : public ICloneable<Channel>,
      public std::enable_shared_from_this<Channel>,
      public ISerializable<Channel>
{
public:
  Channel () = default;
  explicit Channel (ChannelTrack &track);

  /**
   * @brief Initializes the Channel (performs logic that needs the object to be
   * constructed).
   */
  void init ();

  bool is_in_active_project () const;

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
    PluginSlotType           slot_type,
    bool                     copy,
    bool                     ask_if_overwrite);

  /**
   * @brief Prepares the channel for processing.
   *
   * To be called before the main cycle each time on all channels.
   */
  void prepare_process (nframes_t nframes);

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
   * @param slot The position in the strip starting from 0.
   * @param plugin The plugin to add.
   * @param confirm Confirm if an existing plugin will be overwritten.
   * @param moving_plugin Whether or not we are moving the plugin.
   * @param gen_automatables Generatate plugin automatables. To be used when
   * creating a new plugin only.
   * @param recalc_graph Recalculate mixer graph.
   * @param pub_events Publish events.
   *
   * @throw ZrythmException on error.
   */
  Plugin * add_plugin (
    std::unique_ptr<Plugin> &&plugin,
    PluginSlotType            slot_type,
    int                       slot,
    bool                      confirm,
    bool                      moving_plugin,
    bool                      gen_automatables,
    bool                      recalc_graph,
    bool                      pub_events);

  ChannelTrack * get_track () const { return track_; }

  GroupTargetTrack * get_output_track () const;

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
   *
   * @return The plugin that was removed (in case we want to move it).
   */
  std::unique_ptr<Plugin> remove_plugin (
    PluginSlotType slot_type,
    int            slot,
    bool           moving_plugin,
    bool           deleting_plugin,
    bool           deleting_channel,
    bool           recalc_graph);

  /**
   * Updates the track name hash in the channel and all related ports and
   * identifiers.
   */
  void
  update_track_name_hash (unsigned int old_name_hash, unsigned int new_name_hash);

  void get_plugins (std::vector<Plugin *> &pls);

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

  Plugin * get_plugin_at_slot (int slot, PluginSlotType slot_type) const;

  /**
   * Selects/deselects all plugins in the given slot type.
   */
  void select_all (PluginSlotType type, bool select);

  /**
   * Sets caches for processing.
   */
  void set_caches ();

  void init_after_cloning (const Channel &other) override;

  /**
   * Disconnects the channel from the processing chain.
   *
   * This should be called immediately when the channel is getting deleted, and
   * channel_free should be designed to be called later after an arbitrary
   * delay.
   *
   * @param remove_pl Remove the Plugin from the Channel. Useful when deleting
   * the channel.
   * @param recalc_graph Recalculate mixer graph.
   */
  void disconnect (bool remove_pl);

  void init_loaded (ChannelTrack &track);

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
  void append_ports (std::vector<Port *> &ports, bool include_plugins);

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

  void set_phase (float phase);

  float get_phase () const;

  void set_balance_control (float val);

  /**
   * Adds to (or subtracts from) the pan.
   */
  void add_balance_control (float pan);

  float get_balance_control () const;

  /**
   * @brief Set the track ptr to the channel and all its internals that
   * reference a track (Plugin, Fader, etc.)
   *
   * @param track The owner track.
   *
   * This is intended to be used when cloning ChannelTracks where we can't use
   * the constructor.
   */
  void set_track_ptr (ChannelTrack &track);

  DECLARE_DEFINE_FIELDS_METHOD ();

private:
  /**
   * Connect ports in the case of !prev && !next.
   */
  void connect_no_prev_no_next (Plugin &pl);

  /**
   * Connect ports in the case of !prev && next.
   */
  void connect_no_prev_next (Plugin &pl, Plugin &next_pl);

  /**
   * Connect ports in the case of prev && !next.
   */
  void connect_prev_no_next (Plugin &prev_pl, Plugin &pl);

  /**
   * Connect ports in the case of prev && next.
   */
  void connect_prev_next (Plugin &prev_pl, Plugin &pl, Plugin &next_pl);

  /**
   * Disconnect ports in the case of !prev && !next.
   */
  void disconnect_no_prev_no_next (Plugin &pl);

  /**
   * Disconnect ports in the case of !prev && next.
   */
  void disconnect_no_prev_next (Plugin &pl, Plugin &next_pl);

  /**
   * Connect ports in the case of prev && !next.
   */
  void disconnect_prev_no_next (Plugin &prev_pl, Plugin &pl);

  /**
   * Connect ports in the case of prev && next.
   */
  void disconnect_prev_next (Plugin &prev_pl, Plugin &pl, Plugin &next_pl);

  void connect_plugins ();

  /**
   * Inits the stereo ports of the Channel while exposing them to the backend.
   *
   * This assumes the caller already checked that this channel should have the
   * given ports enabled.
   *
   * @param loading 1 if loading a channel, 0 if new.
   */
  void init_stereo_out_ports (bool loading);

  void disconnect_plugin_from_strip (int pos, Plugin &pl);

public:
  /**
   * The MIDI effect strip on instrument/MIDI tracks.
   *
   * This is processed before the instrument/inserts.
   */
  std::array<std::unique_ptr<Plugin>, STRIP_SIZE> midi_fx_;

  /** The channel insert strip. */
  std::array<std::unique_ptr<Plugin>, STRIP_SIZE> inserts_;

  /** The instrument plugin, if instrument track. */
  std::unique_ptr<Plugin> instrument_;

  /**
   * The sends strip.
   *
   * The first 6 (slots 0-5) are pre-fader and the rest are post-fader.
   *
   * @note See CHANNEL_SEND_POST_FADER_START_SLOT.
   */
  std::array<std::unique_ptr<ChannelSend>, STRIP_SIZE> sends_;

  /**
   * External MIDI inputs that are currently connected to this channel as
   * official inputs, unless all_midi_ins is enabled.
   *
   * These should be serialized every time and connected to when the
   * project gets loaded if @ref Channel.all_midi_ins is not enabled.
   *
   * If all_midi_ins is enabled, these are ignored.
   */
  std::vector<std::unique_ptr<ExtPort>> ext_midi_ins_;

  /** If true, the channel will connect to all MIDI ins found. */
  bool all_midi_ins_ = true;

  /**
   * External audio L inputs that are currently connected to this channel
   * as official inputs, unless all_stereo_l_ins is enabled.
   *
   * These should be serialized every time and if all_stereo_l_ins is not
   * enabled, connected to when the project gets loaded.
   *
   * If all_stereo_l_ins is enabled, these are ignored.
   */
  std::vector<std::unique_ptr<ExtPort>> ext_stereo_l_ins_;

  /** If true, the channel will connect to all stereo L ins found. */
  bool all_stereo_l_ins_ = false;

  /**
   * External audio R inputs that are currently connected to this channel
   * as official inputs, unless all_stereo_r_ins is enabled.
   *
   * These should be serialized every time and if all_stereo_r_ins is not
   * enabled, connected to when the project gets loaded.
   *
   * If all_stereo_r_ins is enabled, these are ignored.
   */
  std::vector<std::unique_ptr<ExtPort>> ext_stereo_r_ins_;

  /** If true, the channel will connect to all stereo R ins found. */
  bool all_stereo_r_ins_ = false;

  /**
   * 1 or 0 flags for each channel to enable it or disable it.
   *
   * If all_midi_channels is enabled, this is ignored.
   */
  std::array<bool, 16> midi_channels_;

  /** If true, the channel will accept MIDI messages from all MIDI channels.
   */
  bool all_midi_channels_ = true;

  /** The channel fader. */
  std::unique_ptr<Fader> fader_;

  /**
   * Prefader.
   *
   * The last plugin should connect to this.
   */
  std::unique_ptr<Fader> prefader_;

  /**
   * MIDI output for sending MIDI signals to other destinations, such as
   * other channels when directly routed (eg MIDI track to ins track).
   */
  std::unique_ptr<MidiPort> midi_out_;

  /*
   * Ports for direct (track-to-track) routing with the exception of
   * master, which will route the output to monitor in.
   */
  std::unique_ptr<StereoPorts> stereo_out_;

  /**
   * Whether or not output_pos corresponds to a Track or not.
   *
   * If not, the channel is routed to the engine.
   */
  bool has_output_ = false;

  /** Output track. */
  unsigned int output_name_hash_ = 0;

  /** Track associated with this channel. */
  int track_pos_ = 0;

  /** Channel widget width - reserved for future use. */
  int width_ = 0;

  int magic_ = CHANNEL_MAGIC;

  /** The channel widget. */
  ChannelWidget * widget_ = nullptr;

  /** Owner track. */
  ChannelTrack * track_;
};

/**
 * @}
 */

#endif
