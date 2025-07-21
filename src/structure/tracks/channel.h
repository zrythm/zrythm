// SPDX-FileCopyrightText: © 2018-2022, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "dsp/passthrough_processors.h"
#include "gui/dsp/plugin.h"
#include "gui/dsp/plugin_span.h"
#include "structure/tracks/channel_send.h"
#include "structure/tracks/fader.h"
#include "structure/tracks/track.h"
#include "utils/icloneable.h"

namespace zrythm::structure::tracks
{
class ChannelTrack;
class GroupTargetTrack;

static constexpr float MAX_FADER_AMP = 1.42f;

struct PluginImportData;

class MidiPreFader final : public QObject, public dsp::MidiPassthroughProcessor
{
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE ("")

public:
  MidiPreFader (
    dsp::ProcessorBase::ProcessorBaseDependencies dependencies,
    QObject *                                     parent = nullptr);
};

class AudioPreFader final : public QObject, public dsp::StereoPassthroughProcessor
{
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE ("")

public:
  AudioPreFader (
    dsp::ProcessorBase::ProcessorBaseDependencies dependencies,
    QObject *                                     parent = nullptr);
};

/**
 * @brief Represents a channel strip on the mixer.
 *
 * The Channel class encapsulates the functionality of a channel strip,
 * including its plugins, fader, sends, and other properties.
 *
 * Channels are owned by Track's and handle the second part of the signal
 * chain when processing a track, where the signal is fed to each Channel
 * subcomponent. (TrackProcessor handles the first part where any track inputs
 * and arranger events are processed).
 *
 * @see ChannelTrack, ProcessableTrack and TrackProcessor.
 */
class Channel final : public QObject
{
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY (Fader * fader READ fader CONSTANT)
  Q_PROPERTY (QVariant preFader READ preFader CONSTANT)
  Q_PROPERTY (dsp::AudioPort * leftAudioOut READ getLeftAudioOut CONSTANT)
  Q_PROPERTY (dsp::AudioPort * rightAudioOut READ getRightAudioOut CONSTANT)
  Q_PROPERTY (dsp::MidiPort * midiOut READ getMidiOut CONSTANT)
  QML_UNCREATABLE ("")

public:
  using PortType = zrythm::dsp::PortType;
  using TrackUuid = utils::UuidIdentifiableObject<Track>::Uuid;
  using Plugin = gui::old_dsp::plugins::Plugin;
  using PluginDescriptor = zrythm::plugins::PluginDescriptor;
  using PluginSlot = zrythm::plugins::PluginSlot;
  using PluginSlotType = zrythm::plugins::PluginSlotType;
  using PluginPtrVariant = gui::old_dsp::plugins::PluginPtrVariant;
  using PluginUuid = Plugin::Uuid;

  // FIXME: leftover from C port, fix/refactor how import works in channel.cpp
  friend struct PluginImportData;

  friend class ChannelSubgraphBuilder;

  /**
   * Number of plugin slots per channel.
   */
  static constexpr auto STRIP_SIZE = 9u;

public:
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
    TrackRegistry                   &track_registry,
    PluginRegistry                  &plugin_registry,
    dsp::PortRegistry               &port_registry,
    dsp::ProcessorParameterRegistry &param_registry,
    OptionalRef<ChannelTrack>        track);

  /**
   * To be used when deserializing or cloning an existing identity.
   */
  explicit Channel (
    TrackRegistry                   &track_registry,
    PluginRegistry                  &plugin_registry,
    dsp::PortRegistry               &port_registry,
    dsp::ProcessorParameterRegistry &param_registry)
      : Channel (track_registry, plugin_registry, port_registry, param_registry, {})
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

  Fader *  fader () const { return fader_.get (); }
  QVariant preFader () const
  {
    return midi_out_id_.has_value ()
             ? QVariant::fromValue (midi_prefader_.get ())
             : QVariant::fromValue (audio_prefader_.get ());
  }
  dsp::AudioPort * getLeftAudioOut () const
  {
    return stereo_out_left_id_.has_value ()
             ? std::addressof (get_stereo_out_ports ().first)
             : nullptr;
  }
  dsp::AudioPort * getRightAudioOut () const
  {
    return stereo_out_left_id_.has_value ()
             ? std::addressof (get_stereo_out_ports ().second)
             : nullptr;
  }
  dsp::MidiPort * getMidiOut () const
  {
    return midi_out_id_.has_value () ? std::addressof (get_midi_out_port ()) : nullptr;
  }

  // ============================================================================

  /**
   * @brief Initializes the Channel (performs logic that needs the object to be
   * constructed).
   */
  void init ();

  utils::Utf8String get_full_designation_for_port (const dsp::Port &port) const;

  dsp::MidiPort &get_midi_out_port () const
  {
    return *std::get<dsp::MidiPort *> (midi_out_id_->get_object ());
  }
  std::pair<dsp::AudioPort &, dsp::AudioPort &> get_stereo_out_ports () const
  {
    auto * l = std::get<dsp::AudioPort *> (stereo_out_left_id_->get_object ());
    auto * r = std::get<dsp::AudioPort *> (stereo_out_right_id_->get_object ());
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
    const Plugin *            pl,
    std::optional<PluginSpan> plugins,
    const PluginDescriptor *  descr,
    PluginSlot                slot,
    bool                      copy,
    bool                      ask_if_overwrite);

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
    PluginSlot          slot,
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
  // void reconnect_ext_input_ports (engine::device_io::AudioEngine &engine);

  /**
   * @brief Returns all existing plugins in the channel.
   *
   * @param pls Vector to add plugins to.
   */
  void get_plugins (std::vector<Plugin *> &pls);

  /**
   * Paste the selections starting at the slot in the given channel.
   *
   * This calls gen_full_from_this() internally to generate FullMixerSelections
   * with cloned plugins (calling init_loaded() on each), which are then pasted.
   */
  void paste_plugins_to_slot (PluginSpan plugins, PluginSlot slot);

  std::optional<PluginPtrVariant> get_plugin_at_slot (PluginSlot slot) const;

  auto get_plugin_slot (const PluginUuid &plugin_id) const -> PluginSlot;

  std::optional<PluginPtrVariant> get_plugin_from_id (PluginUuid id) const;

  std::optional<PluginPtrVariant> get_instrument () const
  {
    return instrument_ ? std::make_optional (instrument_->get_object ()) : std::nullopt;
  }

  /**
   * Selects/deselects all plugins in the given slot type.
   */
  void select_all (PluginSlotType type, bool select);

  /**
   * Sets caches for processing.
   */
  void set_caches ();

  friend void init_from (
    Channel               &obj,
    const Channel         &other,
    utils::ObjectCloneType clone_type);

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
  auto  &get_midi_pre_fader () const { return *midi_prefader_; }
  auto  &get_audio_pre_fader () const { return *audio_prefader_; }

  auto &get_sends () const { return sends_; }

private:
  static constexpr auto kMidiFxKey = "midiFx"sv;
  static constexpr auto kInsertsKey = "inserts"sv;
  static constexpr auto kSendsKey = "sends"sv;
  static constexpr auto kInstrumentKey = "instrument"sv;
  static constexpr auto kMidiPrefaderKey = "midiPrefader"sv;
  static constexpr auto kAudioPrefaderKey = "audioPrefader"sv;
  static constexpr auto kFaderKey = "fader"sv;
  static constexpr auto kMidiOutKey = "midiOut"sv;
  static constexpr auto kStereoOutLKey = "stereoOutL"sv;
  static constexpr auto kStereoOutRKey = "stereoOutR"sv;
  static constexpr auto kOutputIdKey = "outputId"sv;
  static constexpr auto kTrackIdKey = "trackId"sv;
  static constexpr auto kExtMidiInputsKey = "extMidiIns"sv;
  static constexpr auto kMidiChannelsKey = "midiChannels"sv;
  static constexpr auto kExtStereoLInputsKey = "extStereoLIns"sv;
  static constexpr auto kExtStereoRInputsKey = "extStereoRIns"sv;
  static constexpr auto kWidthKey = "width"sv;

  friend void to_json (nlohmann::json &j, const Channel &c)
  {
    j[kMidiFxKey] = c.midi_fx_;
    j[kInsertsKey] = c.inserts_;
    j[kSendsKey] = c.sends_;
    j[kInstrumentKey] = c.instrument_;
    j[kMidiPrefaderKey] = c.midi_prefader_;
    j[kAudioPrefaderKey] = c.audio_prefader_;
    j[kFaderKey] = c.fader_;
    j[kMidiOutKey] = c.midi_out_id_;
    j[kStereoOutLKey] = c.stereo_out_left_id_;
    j[kStereoOutRKey] = c.stereo_out_right_id_;
    j[kOutputIdKey] = c.output_track_uuid_;
    j[kTrackIdKey] = c.track_uuid_;
    j[kExtMidiInputsKey] = c.ext_midi_ins_;
    j[kMidiChannelsKey] = c.midi_channels_;
    j[kExtStereoLInputsKey] = c.ext_stereo_l_ins_;
    j[kExtStereoRInputsKey] = c.ext_stereo_r_ins_;
    j[kWidthKey] = c.width_;
  }
  friend void from_json (const nlohmann::json &j, Channel &c);

  /**
   * Inits the stereo ports of the Channel while exposing them to the backend.
   *
   * This assumes the caller already checked that this channel should have the
   * given ports enabled.
   *
   * @param loading 1 if loading a channel, 0 if new.
   */
  // void init_stereo_out_ports (bool loading);

  /**
   * Disconnects all hardware inputs from the port.
   */
  void disconnect_port_hardware_inputs (dsp::Port &port);

  // void disconnect_port_hardware_inputs (StereoPorts &ports);

public:
  TrackRegistry                   &track_registry_;
  dsp::PortRegistry               &port_registry_;
  dsp::ProcessorParameterRegistry &param_registry_;
  PluginRegistry                  &plugin_registry_;

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
   * External MIDI inputs (juce::MidiDeviceInfo::identifier) that are currently
   * connected to this channel as official inputs, unless all_midi_ins is
   * enabled.
   *
   * These should be serialized every time and connected to when the
   * project gets loaded if @ref Channel.all_midi_ins is not enabled.
   *
   * If nullopt, the channel will accept all MIDI inputs.
   */
  std::optional<std::vector<utils::Utf8String>> ext_midi_ins_;

  /**
   * External audio L inputs  that are
   * currently connected to this channel as official inputs, unless
   * all_stereo_l_ins is enabled.
   *
   * These should be serialized every time and if all_stereo_l_ins is not
   * enabled, connected to when the project gets loaded.
   *
   * If nullopt, the channel will connect to all stereo L inputs found.
   *
   * @warning Currently not sure what to save here.
   */
  std::optional<std::vector<utils::Utf8String>> ext_stereo_l_ins_;

  /**
   * External audio R inputs that are currently connected to this channel
   * as official inputs, unless all_stereo_r_ins is enabled.
   *
   * These should be serialized every time and if all_stereo_r_ins is not
   * enabled, connected to when the project gets loaded.
   *
   * If nullopt, the channel will connect to all stereo R inputs found.
   *
   * @warning Currently not sure what to save here.
   */
  std::optional<std::vector<utils::Utf8String>> ext_stereo_r_ins_;

  /**
   * 1 or 0 flags for each channel to enable it or disable it.
   *
   * If nullopt, the channel will accept MIDI messages from all MIDI channels.
   */
  std::optional<std::array<bool, 16>> midi_channels_;

  /** The channel fader. */
  utils::QObjectUniquePtr<Fader> fader_;

  /**
   * Prefader.
   *
   * The last plugin should connect to this.
   */
  utils::QObjectUniquePtr<MidiPreFader>  midi_prefader_;
  utils::QObjectUniquePtr<AudioPreFader> audio_prefader_;

  /**
   * MIDI output for sending MIDI signals to other destinations, such as
   * other channels when directly routed (eg MIDI track to ins track).
   */
  std::optional<dsp::PortUuidReference> midi_out_id_;

  /*
   * Ports for direct (track-to-track) routing with the exception of
   * master, which will route the output to monitor in.
   */
  std::optional<dsp::PortUuidReference> stereo_out_left_id_;
  std::optional<dsp::PortUuidReference> stereo_out_right_id_;

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

}; // namespace zrythm::structure::tracks
