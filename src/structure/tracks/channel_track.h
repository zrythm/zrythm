// SPDX-FileCopyrightText: © 2018-2019, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "structure/tracks/channel.h"
#include "structure/tracks/processable_track.h"

#define DEFINE_CHANNEL_TRACK_QML_PROPERTIES(ClassType) \
public: \
  Q_PROPERTY (Channel * channel READ channel CONSTANT)

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
  ~ChannelTrack () override = default;

  void init_loaded (
    plugins::PluginRegistry         &plugin_registry,
    dsp::PortRegistry               &port_registry,
    dsp::ProcessorParameterRegistry &param_registry) override;

  Channel * channel () const { return channel_.get (); }

  /**
   * Returns if the track is muted.
   */
  bool currently_muted () const override
  {
    return channel_->get_fader ().currently_muted ();
  }

  /**
   * Returns if the track is listened.
   */
  bool currently_listened () const override
  {
    return channel_->get_fader ().currently_listened ();
  }

  /**
   * Returns whether the fader is not soloed on its own but its direct out (or
   * its direct out's direct out, etc.) or its child (or its children's child,
   * etc.) is soloed.
   */
  bool get_implied_soloed () const override;

  bool currently_soloed () const override
  {
    return channel_->get_fader ().currently_soloed ();
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

  void set_output (std::optional<Track::Uuid> id) { output_track_uuid_ = id; }

  bool has_output () const { return output_track_uuid_.has_value (); }

  auto output_track () const
  {
    return track_registry_.find_by_id_or_throw (output_track_uuid_.value ());
  }

  GroupTargetTrack &output_track_as_group_target () const;

protected:
  friend void init_from (
    ChannelTrack          &obj,
    const ChannelTrack    &other,
    utils::ObjectCloneType clone_type);

  /**
   * @brief Initializes the channel.
   *
   * This is mainly used to set the parent on the channel.
   */
  void init_channel ();

private:
  static constexpr auto kChannelKey = "channel"sv;
  static constexpr auto kOutputIdKey = "outputId"sv;
  friend void           to_json (nlohmann::json &j, const ChannelTrack &c)
  {
    j[kChannelKey] = c.channel_;
    j[kOutputIdKey] = c.output_track_uuid_;
  }
  friend void from_json (const nlohmann::json &j, ChannelTrack &c)
  {
    c.channel_ = c.generate_channel ();
    j.at (kChannelKey).get_to (*c.channel_);
    j.at (kOutputIdKey).get_to (c.output_track_uuid_);
  }

  utils::QObjectUniquePtr<Channel> generate_channel ();

private:
  /**
   * @brief Owned channel object.
   */
  utils::QObjectUniquePtr<Channel> channel_;

  /** Output track. */
  std::optional<Uuid> output_track_uuid_;

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
