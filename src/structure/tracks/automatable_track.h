// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once
#include "structure/tracks/track.h"

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

namespace zrythm::structure::tracks
{
/**
 * Interface for a track that has automatable parameters.
 *
 * Tracks that can have plugins must implement this interface.
 */
class AutomatableTrack : virtual public Track
{
public:
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
    for (auto &at : atl.get_automation_tracks ())
      {
        for (auto * r : at->get_children_view ())
          {
            add_region_if_in_range (p1, p2, regions, r);
          }
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

  /**
   * Generates automatables for the plugin.
   *
   * @note The plugin must be instantiated already.
   */
  void generate_automation_tracks_for_plugin (const Plugin::Uuid &plugin_id);

protected:
  friend void init_from (
    AutomatableTrack       &obj,
    const AutomatableTrack &other,
    utils::ObjectCloneType  clone_type);

  void set_playback_caches () override
  {
    get_automation_tracklist ().set_caches (CacheType::PlaybackSnapshots);
  }

private:
  static constexpr auto kAutomationTracklistKey = "automationTracklist"sv;
  static constexpr auto kAutomationVisibleKey = "automationVisible"sv;
  friend void to_json (nlohmann::json &j, const AutomatableTrack &track)
  {
    j[kAutomationTracklistKey] = track.automation_tracklist_;
    j[kAutomationVisibleKey] = track.automation_visible_;
  }
  friend void from_json (const nlohmann::json &j, AutomatableTrack &track)
  {
    track.automation_tracklist_ = new AutomationTracklist (
      track.port_registry_, track.object_registry_, track);
    from_json (j, *track.automation_tracklist_);
    j.at (kAutomationVisibleKey).get_to (track.automation_visible_);
  }

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
}
