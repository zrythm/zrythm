// SPDX-FileCopyrightText: © 2018-2019, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __AUDIO_CHANNEL_TRACK_H__
#define __AUDIO_CHANNEL_TRACK_H__

#include "gui/backend/channel.h"
#include "gui/dsp/processable_track.h"

#define DEFINE_CHANNEL_TRACK_QML_PROPERTIES(ClassType) \
public: \
  /* ================================================================ */ \
  /* channel */ \
  /* ================================================================ */ \
  Q_PROPERTY (Channel * channel READ getChannel CONSTANT) \
  Channel * getChannel () const \
  { \
    return channel_; \
  }

/**
 * Abstract class for a track that has a channel in the mixer.
 */
class ChannelTrack
    : virtual public ProcessableTrack,
      public zrythm::utils::serialization::ISerializable<ChannelTrack>
{
public:
  using Channel = zrythm::gui::Channel;

public:
  ~ChannelTrack () override;

  void init_loaded () override;

  auto &get_channel () { return channel_; }

  auto &get_channel () const { return channel_; }

  /**
   * Returns the Fader (if applicable).
   *
   * @param post_fader True to get post fader, false to get pre fader.
   */
  Fader * get_fader (bool post_fader);

  /**
   * Returns if the track is muted.
   */
  bool get_muted () const override { return channel_->fader_->get_muted (); }

  /**
   * Returns if the track is listened.
   */
  bool get_listened () const override
  {
    return channel_->fader_->get_listened ();
  }

  bool get_implied_soloed () const override
  {
    return channel_->fader_->get_implied_soloed ();
  }

  bool get_soloed () const override { return channel_->fader_->get_soloed (); }

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
  Fader::Type get_fader_type () { return get_prefader_type (); }

  Fader::Type get_prefader_type () { return type_get_prefader_type (type_); }

protected:
  ChannelTrack ();

  void
  append_member_ports (std::vector<Port *> &ports, bool include_plugins) const;

  void copy_members_from (const ChannelTrack &other);

  bool validate_base () const;

  /**
   * @brief Initializes the channel.
   */
  void init_channel ();

  DECLARE_DEFINE_BASE_FIELDS_METHOD ();

private:
  /**
   * Removes the AutomationTrack's associated with this channel from the
   * AutomationTracklist in the corresponding Track.
   */
  void remove_ats_from_automation_tracklist (bool fire_events);

public:
  /**
   * @brief Owned channel object.
   */
  Channel * channel_ = nullptr;
};

#endif // __AUDIO_CHANNEL_TRACK_H__
