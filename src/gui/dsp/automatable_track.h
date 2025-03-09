// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __AUDIO_AUTOMATABLE_TRACK_H__
#define __AUDIO_AUTOMATABLE_TRACK_H__

#include "gui/dsp/track.h"

#define DEFINE_AUTOMATABLE_TRACK_QML_PROPERTIES(ClassType) \
public: \
  /* ================================================================ */ \
  /* automationVisible */ \
  /* ================================================================ */ \
  Q_PROPERTY ( \
    bool automationVisible READ getAutomationVisible WRITE \
      setAutomationVisible NOTIFY automationVisibleChanged) \
  bool getAutomationVisible () const \
  { \
    return automation_visible_; \
  } \
  void setAutomationVisible (bool visible) \
  { \
    if (automation_visible_ == visible) \
      return; \
\
    automation_visible_ = visible; \
    Q_EMIT automationVisibleChanged (visible); \
  } \
\
  Q_SIGNAL void automationVisibleChanged (bool visible); \
\
  /* ================================================================ */ \
  /* automationTracks */ \
  /* ================================================================ */ \
  Q_PROPERTY ( \
    AutomationTracklist * automationTracks READ getAutomationTracks CONSTANT) \
  AutomationTracklist * getAutomationTracks () const \
  { \
    return automation_tracklist_; \
  }

/**
 * Interface for a track that has automatable parameters.
 *
 * Tracks that can have plugins must implement this interface.
 */
class AutomatableTrack
    : virtual public Track,
      public zrythm::utils::serialization::ISerializable<AutomatableTrack>
{
public:
  AutomatableTrack (const DeserializationDependencyHolder &dh);
  AutomatableTrack (PortRegistry &port_registry, bool new_identity);

  ~AutomatableTrack () override = default;

  using Plugin = gui::old_dsp::plugins::Plugin;
  using PluginRegistry = gui::old_dsp::plugins::PluginRegistry;

  void
  init_loaded (PluginRegistry &plugin_registry, PortRegistry &port_registry)
    override;

  AutomationTracklist &get_automation_tracklist () const
  {
    return *automation_tracklist_;
  }

  /**
   * Set automation visible and fire events.
   */
  void set_automation_visible (bool visible);

  /**
   * Generates automatables for the track.
   *
   * Should be called as soon as the track is created.
   */
  void generate_automation_tracks ();

  void clear_objects () override { automation_tracklist_->clear_objects (); }

  void get_regions_in_range (
    std::vector<Region *> &regions,
    const Position *       p1,
    const Position *       p2) override
  {
    auto &atl = get_automation_tracklist ();
    for (auto &at : atl.ats_)
      {
        at->foreach_region ([&] (auto &r) {
          add_region_if_in_range (p1, p2, regions, &r);
        });
      }
  }

  /**
   * Returns the plugin at the given slot, if any.
   */
  template <typename DerivedT>
  std::optional<PluginPtrVariant>
  get_plugin_at_slot (this DerivedT &&self, PluginSlot slot)
    requires std::derived_from<base_type<DerivedT>, AutomatableTrack>
             && FinalClass<base_type<DerivedT>>
  {
    if constexpr (std::derived_from<DerivedT, ChannelTrack>)
      {
        return std::forward (self).get_channel ()->get_plugin_at_slot (slot);
      }
    else if constexpr (std::derived_from<DerivedT, ModulatorTrack>)
      {
        if (
          slot.is_modulator ()
          && slot.get_slot_with_index ().second
               < (int) std::forward (self).modulators_.size ())
          {
            return std::forward (self)
              .modulators_[slot.get_slot_with_index ().second]
              .get ();
          }
      }

    return std::nullopt;
  }

protected:
  void
  copy_members_from (const AutomatableTrack &other, ObjectCloneType clone_type);

  bool validate_base () const;

  void set_playback_caches () override
  {
    get_automation_tracklist ().set_caches (CacheType::PlaybackSnapshots);
  }

  DECLARE_DEFINE_BASE_FIELDS_METHOD ();

public:
  AutomationTracklist * automation_tracklist_ = nullptr;

  /** Flag to set automations visible or not. */
  bool automation_visible_ = false;
};

using AutomatableTrackVariant = std::variant<
  InstrumentTrack,
  MidiTrack,
  MasterTrack,
  MidiGroupTrack,
  AudioGroupTrack,
  MidiBusTrack,
  AudioBusTrack,
  AudioTrack,
  ChordTrack,
  ModulatorTrack,
  TempoTrack>;
using AutomatableTrackPtrVariant = to_pointer_variant<AutomatableTrackVariant>;

#endif // __AUDIO_AUTOMATABLE_TRACK_H__
