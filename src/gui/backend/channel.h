// SPDX-FileCopyrightText: © 2018-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __AUDIO_CHANNEL_H__
#define __AUDIO_CHANNEL_H__

#include "zrythm-config.h"

#include "dsp/channel.h"
#include "gui/dsp/channel_send.h"
#include "gui/dsp/ext_port.h"
#include "gui/dsp/fader.h"
#include "gui/dsp/plugin.h"
#include "gui/dsp/plugin_span.h"
#include "gui/dsp/track.h"
#include "utils/icloneable.h"

class AutomationTrack;
class ChannelTrack;
class GroupTargetTrack;
class ExtPort;

class Track;

namespace zrythm::gui
{

static constexpr float MAX_FADER_AMP = 1.42f;

struct PluginImportData;

/**
 * @brief Represents a channel strip on the mixer.
 *
 * The Channel class encapsulates the functionality of a channel strip,
 * including its plugins, fader, sends, and other properties. It provides
 * methods for managing the channel's state and processing the audio signal.
 *
 * Channels are owned by Tracks and handle the output part of the signal
 * chain, while TrackProcessor handles the input part.
 *
 * @see Track
 */
class Channel final
    : public QObject,
      public ICloneable<Channel>,
      public utils::serialization::ISerializable<Channel>,
      public IPortOwner
{
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY (Fader * fader READ getFader CONSTANT)
  Q_PROPERTY (Fader * preFader READ getPreFader CONSTANT)
  Q_PROPERTY (AudioPort * leftAudioOut READ getLeftAudioOut CONSTANT)
  Q_PROPERTY (AudioPort * rightAudioOut READ getRightAudioOut CONSTANT)
  Q_PROPERTY (MidiPort * midiOut READ getMidiOut CONSTANT)

public:
  static constexpr auto STRIP_SIZE = zrythm::dsp::STRIP_SIZE;
  using PortType = zrythm::dsp::PortType;
  using PortIdentifier = dsp::PortIdentifier;
  using TrackUuid = utils::UuidIdentifiableObject<Track>::Uuid;
  using Plugin = gui::old_dsp::plugins::Plugin;
  using PluginPtrVariant = gui::old_dsp::plugins::PluginPtrVariant;
  using PluginUuid = Plugin::Uuid;

  // FIXME: leftover from C port, fix/refactor how import works in channel.cpp
  friend struct PluginImportData;

public:
  Channel (const DeserializationDependencyHolder &dh);

  /**
   * @brief Main constructor used by the others.
   *
   * @param track_registry
   * @param plugin_registry
   * @param port_registry
   * @param track_id Must be passed when creating a new Channel identity
   * instance.
   */
  explicit Channel (
    TrackRegistry            &track_registry,
    PluginRegistry           &plugin_registry,
    PortRegistry             &port_registry,
    OptionalRef<ChannelTrack> track);

  /**
   * To be used when deserializing or cloning an existing identity.
   */
  explicit Channel (
    TrackRegistry  &track_registry,
    PluginRegistry &plugin_registry,
    PortRegistry   &port_registry)
      : Channel (track_registry, plugin_registry, port_registry, {})
  {
  }

private:
  auto &get_track_registry () { return track_registry_; }
  auto &get_track_registry () const { return track_registry_; }
  auto &get_plugin_registry () { return plugin_registry_; }
  auto &get_plugin_registry () const { return plugin_registry_; }
  auto &get_port_registry () { return port_registry_; }
  auto &get_port_registry () const { return port_registry_; }

public:
  // ============================================================================
  // QML Interface
  // ============================================================================

  Fader *       getFader () const { return fader_; }
  Fader *       getPreFader () const { return prefader_; }
  AudioPort *   getLeftAudioOut () const
  {
    return stereo_out_left_id_.has_value ()
             ? std::addressof (get_stereo_out_ports ().first)
             : nullptr;
  }
  AudioPort * getRightAudioOut () const
  {
    return stereo_out_left_id_.has_value ()
             ? std::addressof (get_stereo_out_ports ().second)
             : nullptr;
  }
  MidiPort * getMidiOut () const
  {
    return midi_out_id_.has_value () ? std::addressof (get_midi_out_port ()) : nullptr;
  }

  // ============================================================================

  /**
   * @brief Initializes the Channel (performs logic that needs the object to be
   * constructed).
   */
  void init ();

  bool is_in_active_project () const override;

  void set_port_metadata_from_owner (dsp::PortIdentifier &id, PortRange &range)
    const override;

  std::string
  get_full_designation_for_port (const dsp::PortIdentifier &id) const override;

  bool should_bounce_to_master (utils::audio::BounceStep step) const override;

  MidiPort &get_midi_out_port () const
  {
    return *std::get<MidiPort *> (midi_out_id_->get_object ());
  }
  std::pair<AudioPort &, AudioPort &> get_stereo_out_ports () const
  {
    auto * l = std::get<AudioPort *> (stereo_out_left_id_->get_object ());
    auto * r = std::get<AudioPort *> (stereo_out_right_id_->get_object ());
    return { *l, *r };
  }

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
    const zrythm::gui::old_dsp::plugins::Plugin *           pl,
    std::optional<PluginSpanVariant>                        plugins,
    const zrythm::gui::old_dsp::plugins::PluginDescriptor * descr,
    dsp::PluginSlot                                         slot,
    bool                                                    copy,
    bool                                                    ask_if_overwrite);

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
  PluginPtrVariant add_plugin (
    PluginUuidReference plugin_id,
    dsp::PluginSlot     slot,
    bool                confirm,
    bool                moving_plugin,
    bool                gen_automatables,
    bool                recalc_graph,
    bool                pub_events);

  ChannelTrack &get_track () const { return *track_; }

  GroupTargetTrack * get_output_track () const;

  /**
   * Called when the input has changed for Midi, Instrument or Audio tracks.
   */
  void reconnect_ext_input_ports (AudioEngine &engine);

  /**
   * Convenience function to get the automation track of the given type for
   * the channel.
   */
  AutomationTrack *
  get_automation_track (PortIdentifier::Flags port_flags) const;

  /**
   * Removes a plugin at the given slot from the channel.
   *
   * FIXME this is the same as modulator_track_remove_modulator().
   * TODO refactor into track_remove_plugin().
   *
   * @param moving_plugin Whether or not we are moving the plugin.
   * @param deleting_plugin Whether or not we are deleting the plugin.
   *
   * @return The plugin that was removed (in case we want to move it).
   */
  PluginUuid remove_plugin_from_channel (
    dsp::PluginSlot slot,
    bool            moving_plugin,
    bool            deleting_plugin);

  /**
   * @brief Returns all existing plugins in the channel.
   *
   * @param pls Vector to add plugins to.
   */
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

  std::optional<PluginPtrVariant>
  get_plugin_at_slot (dsp::PluginSlot slot) const;

  dsp::PluginSlot get_plugin_slot (const PluginUuid &plugin_id) const;

  std::optional<PluginPtrVariant> get_plugin_from_id (PluginUuid id) const;

  std::optional<PluginPtrVariant> get_instrument () const
  {
    return instrument_ ? std::make_optional (instrument_->get_object ()) : std::nullopt;
  }

  /**
   * Selects/deselects all plugins in the given slot type.
   */
  void select_all (dsp::PluginSlotType type, bool select);

  /**
   * Sets caches for processing.
   */
  void set_caches ();

  void
  init_after_cloning (const Channel &other, ObjectCloneType clone_type) override;

  /**
   * Disconnects the channel from the processing chain and removes any plugins
   * it contains.
   */
  void disconnect_channel ();

  /**
   * Connects the channel's ports.
   *
   * This should only be called on project tracks.
   */
  void connect_channel (PortConnectionsManager &mgr, AudioEngine &engine);

  void init_loaded ();

  /**
   * Handles the recording logic inside the process cycle.
   *
   * The MidiEvents are already dequeued at this point.
   *
   * @param g_frames_start Global start frames.
   * @param nframes Number of frames to process.
   */
  void handle_recording (long g_frames_start, nframes_t nframes);

  /**
   * Appends all channel ports and optionally plugin ports to the array.
   */
  void append_ports (std::vector<Port *> &ports, bool include_plugins);

  /**
   * Exposes the channel's ports to the backend.
   */
  void expose_ports_to_backend (AudioEngine &engine);

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

  bool has_output () const { return output_track_uuid_.has_value (); }

  Fader &get_post_fader () const { return *fader_; }
  Fader &get_pre_fader () const { return *prefader_; }

  auto &get_sends () const { return sends_; }

  DECLARE_DEFINE_FIELDS_METHOD ();

private:
  /**
   * Connect ports in the case of !prev && !next.
   */
  void connect_no_prev_no_next (zrythm::gui::old_dsp::plugins::Plugin &pl);

  /**
   * Connect ports in the case of !prev && next.
   */
  void connect_no_prev_next (
    zrythm::gui::old_dsp::plugins::Plugin &pl,
    zrythm::gui::old_dsp::plugins::Plugin &next_pl);

  /**
   * Connect ports in the case of prev && !next.
   */
  void connect_prev_no_next (
    zrythm::gui::old_dsp::plugins::Plugin &prev_pl,
    zrythm::gui::old_dsp::plugins::Plugin &pl);

  /**
   * Connect ports in the case of prev && next.
   */
  void connect_prev_next (
    zrythm::gui::old_dsp::plugins::Plugin &prev_pl,
    zrythm::gui::old_dsp::plugins::Plugin &pl,
    zrythm::gui::old_dsp::plugins::Plugin &next_pl);

  /**
   * Disconnect ports in the case of !prev && !next.
   */
  void disconnect_no_prev_no_next (zrythm::gui::old_dsp::plugins::Plugin &pl);

  /**
   * Disconnect ports in the case of !prev && next.
   */
  void disconnect_no_prev_next (
    zrythm::gui::old_dsp::plugins::Plugin &pl,
    zrythm::gui::old_dsp::plugins::Plugin &next_pl);

  /**
   * Connect ports in the case of prev && !next.
   */
  void disconnect_prev_no_next (
    zrythm::gui::old_dsp::plugins::Plugin &prev_pl,
    zrythm::gui::old_dsp::plugins::Plugin &pl);

  /**
   * Connect ports in the case of prev && next.
   */
  void disconnect_prev_next (
    zrythm::gui::old_dsp::plugins::Plugin &prev_pl,
    zrythm::gui::old_dsp::plugins::Plugin &pl,
    zrythm::gui::old_dsp::plugins::Plugin &next_pl);

  void connect_plugins ();

  /**
   * Inits the stereo ports of the Channel while exposing them to the backend.
   *
   * This assumes the caller already checked that this channel should have the
   * given ports enabled.
   *
   * @param loading 1 if loading a channel, 0 if new.
   */
  // void init_stereo_out_ports (bool loading);

  void disconnect_plugin_from_strip (
    dsp::PluginSlot                        slot,
    zrythm::gui::old_dsp::plugins::Plugin &pl);

  /**
   * Disconnects all hardware inputs from the port.
   */
  void disconnect_port_hardware_inputs (Port &port);

  // void disconnect_port_hardware_inputs (StereoPorts &ports);

public:
  TrackRegistry  &track_registry_;
  PortRegistry   &port_registry_;
  PluginRegistry &plugin_registry_;

  /**
   * The MIDI effect strip on instrument/MIDI tracks.
   *
   * This is processed before the instrument/inserts.
   */
  std::array<std::optional<PluginUuidReference>, STRIP_SIZE> midi_fx_;

  /** The channel insert strip. */
  std::array<std::optional<PluginUuidReference>, STRIP_SIZE> inserts_;

  /** The instrument plugin, if instrument track. */
  std::optional<PluginUuidReference> instrument_;

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
  std::array<bool, 16> midi_channels_{};

  /** If true, the channel will accept MIDI messages from all MIDI channels.
   */
  bool all_midi_channels_ = true;

  /** The channel fader. */
  Fader * fader_ = nullptr;

  /**
   * Prefader.
   *
   * The last plugin should connect to this.
   */
  Fader * prefader_ = nullptr;

  /**
   * MIDI output for sending MIDI signals to other destinations, such as
   * other channels when directly routed (eg MIDI track to ins track).
   */
  std::optional<PortUuidReference> midi_out_id_;

  /*
   * Ports for direct (track-to-track) routing with the exception of
   * master, which will route the output to monitor in.
   */
  std::optional<PortUuidReference> stereo_out_left_id_;
  std::optional<PortUuidReference> stereo_out_right_id_;

  /**
   * Whether or not output_pos corresponds to a Track or not.
   *
   * If not, the channel is routed to the engine.
   */
  // bool has_output_ = false;

  /** Output track. */
  std::optional<TrackUuid> output_track_uuid_;

  /** Track associated with this channel. */
  std::optional<TrackUuid> track_uuid_;

  /** Channel widget width - reserved for future use. */
  int width_ = 0;

  /** Owner track. */
  ChannelTrack * track_;
};

}; // namespace zrythm::gui

#endif
