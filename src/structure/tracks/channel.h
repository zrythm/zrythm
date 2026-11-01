// SPDX-FileCopyrightText: © 2018-2022, 2024-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "dsp/fader.h"
#include "dsp/passthrough_processors.h"
#include "plugins/plugin_group.h"
#include "structure/tracks/channel_send.h"
#include "utils/icloneable.h"
#include "utils/object_registry.h"

namespace zrythm::structure::tracks
{

class ChannelMidiPassthroughProcessor final : public dsp::MidiPassthroughProcessor
{
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE ("")

public:
  ChannelMidiPassthroughProcessor (
    utils::IObjectRegistry &registry,
    QObject *               parent = nullptr);
};

class ChannelAudioPassthroughProcessor final
    : public dsp::StereoPassthroughProcessor
{
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE ("")

public:
  ChannelAudioPassthroughProcessor (
    utils::IObjectRegistry &registry,
    QObject *               parent = nullptr);
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
class Channel : public QObject
{
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY (zrythm::dsp::Fader * fader READ fader CONSTANT)
  Q_PROPERTY (QVariant preFader READ preFader CONSTANT)
  Q_PROPERTY (zrythm::dsp::AudioPort * audioOutPort READ audioOutPort CONSTANT)
  Q_PROPERTY (zrythm::dsp::MidiPort * midiOut READ getMidiOut CONSTANT)
  Q_PROPERTY (
    zrythm::dsp::MidiPort * postMidiFxPort READ postMidiFxPort CONSTANT)
  Q_PROPERTY (zrythm::plugins::PluginGroup * inserts READ inserts CONSTANT)
  Q_PROPERTY (zrythm::plugins::PluginGroup * midiFx READ midiFx CONSTANT)
  Q_PROPERTY (
    zrythm::plugins::PluginGroup * instruments READ instruments CONSTANT)
  QML_UNCREATABLE ("")

public:
  using PortType = zrythm::dsp::PortType;
  using Plugin = plugins::Plugin;
  using PluginDescriptor = zrythm::plugins::PluginDescriptor;
  using PluginPtrVariant = plugins::PluginPtrVariant;
  using PluginUuid = Plugin::Uuid;

  using NameProvider = std::function<utils::Utf8String ()>;

public:
  explicit Channel (
    utils::IObjectRegistry           &registry,
    dsp::PortType                     input_signal_type,
    dsp::PortType                     output_signal_type,
    NameProvider                      name_provider,
    bool                              hard_limit_fader_output,
    dsp::Fader::ShouldBeMutedCallback should_be_muted_cb,
    QObject *                         parent = nullptr);

  // ============================================================================
  // QML Interface
  // ============================================================================

  dsp::Fader * fader () const { return fader_.get (); }
  QVariant     preFader () const
  {
    return is_midi () ? QVariant::fromValue (midi_prefader_.get ())
                      : QVariant::fromValue (audio_prefader_.get ());
  }
  dsp::AudioPort * audioOutPort () const
  {
    return is_audio ()
             ? std::addressof (audio_postfader_->get_audio_out_port ())
             : nullptr;
  }
  dsp::MidiPort * getMidiOut () const
  {
    return is_midi () ? std::addressof (midi_postfader_->get_midi_out_port (0))
                      : nullptr;
  }

  /**
   * @brief The MIDI stream after the channel's MIDI FX chain, for
   * observation.
   *
   * On MIDI-output channels this is the MIDI pre-fader output. On instrument
   * channels it is the MIDI event tap between the MIDI FX chain and the
   * instrument. The returned port stays valid for the channel's lifetime.
   * Null when the channel has no MIDI input.
   */
  dsp::MidiPort * postMidiFxPort () const
  {
    if (has_midi_event_tap ())
      {
        return std::addressof (midi_event_tap_->get_midi_out_port (0));
      }
    return has_midi_pre_fader ()
             ? std::addressof (midi_prefader_->get_midi_out_port (0))
             : nullptr;
  }

  plugins::PluginGroup * midiFx () const { return midi_fx_.get (); }
  plugins::PluginGroup * inserts () const { return inserts_.get (); }
  plugins::PluginGroup * instruments () const { return instruments_.get (); }

  // ============================================================================

  bool is_midi () const { return signal_type_ == PortType::Midi; }
  bool is_audio () const { return signal_type_ == PortType::Audio; }

  /** Whether the channel receives MIDI input (MIDI and instrument tracks). */
  bool has_midi_input () const { return input_signal_type_ == PortType::Midi; }

  /** Whether the channel owns a MIDI pre-fader processor. */
  bool has_midi_pre_fader () const { return midi_prefader_ != nullptr; }

  /**
   * Whether the channel owns the MIDI event tap between its MIDI FX chain
   * and instrument (instrument tracks).
   */
  bool has_midi_event_tap () const { return midi_event_tap_ != nullptr; }

  /**
   * @brief Returns all existing plugins in the channel.
   */
  std::vector<plugins::PluginUuidReference> get_plugins () const;

  /**
   * @brief Returns the plugin groups holding the channel's plugins, in
   * signal-flow order (MIDI FX, instruments, inserts).
   */
  std::vector<plugins::PluginGroup *> plugin_groups () const;

  /**
   * @brief Returns the channel's built-in processors, in signal-flow
   * order.
   *
   * These exist for the channel's lifetime and are chosen at construction
   * from its signal types: the MIDI event tap, the pre-faders, the fader
   * and the post-faders. User-added content is enumerated separately:
   * plugins (get_plugins()) and sends (pre_fader_sends() /
   * post_fader_sends()).
   */
  std::vector<dsp::ProcessorBase *> get_builtin_processors () const;

  /**
   * @brief Removes the given plugin.
   *
   * @note If moving the plugin, remember to also move automation tracks for
   * this plugin. This method is not concerned with that.
   */
  plugins::PluginUuidReference remove_plugin (plugins::Plugin::Uuid id);

  friend void init_from (
    Channel               &obj,
    const Channel         &other,
    utils::ObjectCloneType clone_type);

  dsp::Fader &get_fader () const { return *fader_; }
  auto       &get_midi_pre_fader () const { return *midi_prefader_; }
  auto       &get_midi_event_tap () const { return *midi_event_tap_; }
  auto       &get_audio_pre_fader () const { return *audio_prefader_; }
  auto       &get_midi_post_fader () const { return *midi_postfader_; }
  auto       &get_audio_post_fader () const { return *audio_postfader_; }

  auto &pre_fader_sends () const { return prefader_sends_; }
  auto &post_fader_sends () const { return postfader_sends_; }

private:
  static constexpr auto kMidiFxKey = "midiFx"sv;
  static constexpr auto kInstrumentsKey = "instruments"sv;
  static constexpr auto kInsertsKey = "inserts"sv;
  static constexpr auto kPreFaderSendsKey = "preFaderSends"sv;
  static constexpr auto kPostFaderSendsKey = "postFaderSends"sv;
  static constexpr auto kFaderKey = "fader"sv;

  friend void to_json (nlohmann::json &j, const Channel &c);
  friend void from_json (const nlohmann::json &j, Channel &c);

  utils::IObjectRegistry &registry () const { return registry_; }

private:
  utils::IObjectRegistry &registry_;

  utils::QObjectUniquePtr<utils::ObjectRegistry> local_registry_;

  NameProvider name_provider_;

  /** Input signal type (MIDI on MIDI and instrument tracks). */
  dsp::PortType input_signal_type_;

  /** Output signal type (audio on instrument, audio and master tracks). */
  dsp::PortType signal_type_;

  bool hard_limit_fader_output_;

  /**
   * The MIDI effect strip on instrument/MIDI tracks.
   *
   * This is processed before the instrument/inserts.
   */
  utils::QObjectUniquePtr<plugins::PluginGroup> midi_fx_;

  /** The instrument plugin, if instrument track. */
  utils::QObjectUniquePtr<plugins::PluginGroup> instruments_;

  /** The channel insert strip. */
  utils::QObjectUniquePtr<plugins::PluginGroup> inserts_;

  std::vector<utils::QObjectUniquePtr<ChannelSend>> prefader_sends_;
  std::vector<utils::QObjectUniquePtr<ChannelSend>> postfader_sends_;

  /** The channel fader. */
  utils::QObjectUniquePtr<dsp::Fader> fader_;

  /**
   * The MIDI pre-fader.
   *
   * Present on MIDI-output channels. The last MIDI FX plugin (or the track
   * processor) connects to its input, and its output feeds the MIDI fader.
   */
  utils::QObjectUniquePtr<ChannelMidiPassthroughProcessor> midi_prefader_;

  /**
   * The MIDI event tap.
   *
   * Present on instrument channels, between the MIDI FX group and the
   * instrument. It provides a stable post-MIDI-FX output port for event
   * observation; it is not a fader stage.
   */
  utils::QObjectUniquePtr<ChannelMidiPassthroughProcessor>  midi_event_tap_;
  utils::QObjectUniquePtr<ChannelAudioPassthroughProcessor> audio_prefader_;

  /**
   * @brief Post-fader passthrough processor.
   *
   * This is used so we avoid custom logic for the channel output by offloading
   * that task to these.
   */
  utils::QObjectUniquePtr<ChannelMidiPassthroughProcessor>  midi_postfader_;
  utils::QObjectUniquePtr<ChannelAudioPassthroughProcessor> audio_postfader_;
};

}; // namespace zrythm::structure::tracks
