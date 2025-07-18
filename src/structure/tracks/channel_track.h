// SPDX-FileCopyrightText: © 2018-2019, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "structure/tracks/channel.h"
#include "structure/tracks/processable_track.h"

#define DEFINE_CHANNEL_TRACK_QML_PROPERTIES(ClassType) \
public: \
  /* ================================================================ */ \
  /* channel */ \
  /* ================================================================ */ \
  Q_PROPERTY (Channel * channel READ getChannel CONSTANT) \
  Channel * getChannel () const \
  { \
    return channel_.get (); \
  }

namespace zrythm::structure::tracks
{
/**
 * Abstract class for a track that has a channel in the mixer.
 */
class ChannelTrack : virtual public ProcessableTrack
{
protected:
  ChannelTrack (FinalTrackDependencies dependencies);

public:
  ~ChannelTrack () override;

  void init_loaded (
    gui::old_dsp::plugins::PluginRegistry &plugin_registry,
    dsp::PortRegistry                     &port_registry,
    dsp::ProcessorParameterRegistry       &param_registry) override;

  Channel * get_channel () { return channel_.get (); }

  const Channel * get_channel () const { return channel_.get (); }

  /**
   * Returns if the track is muted.
   */
  bool currently_muted () const override
  {
    return channel_->get_post_fader ().currently_muted ();
  }

  /**
   * Returns if the track is listened.
   */
  bool currently_listened () const override
  {
    return channel_->get_post_fader ().currently_listened ();
  }

  bool get_implied_soloed () const override
  {
    return channel_->get_post_fader ().get_implied_soloed ();
  }

  bool currently_soloed () const override
  {
    return channel_->get_post_fader ().currently_soloed ();
  }

  /**
   * Generates a menu to be used for channel-related items, eg, fader buttons,
   * direct out, etc.
   */
  // GMenu * generate_channel_context_menu ();

  /**
   * Sets track soloed, updates UI and optionally adds the action to the undo
   * stack.
   *
   * @param auto_select Makes this track the only selection in the tracklist.
   * Useful when listening to a single track.
   * @param trigger_undo Create and perform an undoable action.
   * @param fire_events Fire UI events.
   */
  void set_listened (
    bool listen,
    bool trigger_undo,
    bool auto_select,
    bool fire_events);

  /**
   * Sets track soloed, updates UI and optionally adds the action to the undo
   * stack.
   *
   * @param auto_select Makes this track the only selection in the tracklist.
   * Useful when soloing a single track.
   * @param trigger_undo Create and perform an undoable action.
   * @param fire_events Fire UI events.
   */
  void
  set_soloed (bool solo, bool trigger_undo, bool auto_select, bool fire_events);

  /**
   * Sets track muted and optionally adds the action to the undo stack.
   */
  void
  set_muted (bool mute, bool trigger_undo, bool auto_select, bool fire_events);

  /**
   * Returns the Fader::Type.
   */
  Fader::Type get_fader_type () { return type_get_fader_type (type_); }

  /**
   * Returns the plugin at the given slot, if any.
   */
  std::optional<PluginPtrVariant>
  get_plugin_at_slot (plugins::PluginSlot slot) const
  {
    return get_channel ()->get_plugin_at_slot (slot);
  }

protected:
  friend void init_from (
    ChannelTrack          &obj,
    const ChannelTrack    &other,
    utils::ObjectCloneType clone_type);

  /**
   * @brief Initializes the channel.
   */
  void init_channel ();

private:
  static constexpr auto kChannelKey = "channel"sv;
  friend void to_json (nlohmann::json &j, const ChannelTrack &channel_track)
  {
    j[kChannelKey] = channel_track.channel_;
  }
  friend void from_json (const nlohmann::json &j, ChannelTrack &channel_track)
  {
    channel_track.channel_.reset (new Channel (
      channel_track.track_registry_, channel_track.plugin_registry_,
      channel_track.port_registry_, channel_track.param_registry_));
    j.at (kChannelKey).get_to (*channel_track.channel_);
  }

public:
  /**
   * @brief Owned channel object.
   */
  utils::QObjectUniquePtr<Channel> channel_;

private:
  TrackRegistry &track_registry_;
};

using ChannelTrackVariant = std::variant<
  AudioBusTrack,
  AudioTrack,
  ChordTrack,
  MidiBusTrack,
  MidiTrack,
  AudioGroupTrack,
  MidiGroupTrack,
  InstrumentTrack,
  MasterTrack>;
using ChannelTrackPtrVariant = to_pointer_variant<ChannelTrackVariant>;

} // namespace zrythm::structure::tracks
