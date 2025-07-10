// SPDX-FileCopyrightText: © 2018-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "plugins/plugin_slot.h"
#include "structure/tracks/automation_track.h"

#include <QAbstractListModel>

namespace zrythm::structure::tracks
{
class AutomatableTrack;
class Channel;

/**
 * Each track has an automation tracklist with automation tracks to be generated
 * at runtime, and filled in with automation points/curves when loading projects.
 */
class AutomationTracklist final : public QAbstractListModel
{
  Q_OBJECT
  QML_ELEMENT

  using ArrangerObjectRegistry = arrangement::ArrangerObjectRegistry;

public:
  AutomationTracklist (
    dsp::FileAudioSourceRegistry    &file_audio_source_registry,
    dsp::PortRegistry               &port_registry,
    dsp::ProcessorParameterRegistry &param_registry,
    ArrangerObjectRegistry          &object_registry,
    AutomatableTrack                &track,
    QObject *                        parent = nullptr);

public:
  enum Roles
  {
    AutomationTrackPtrRole = Qt::UserRole + 1,
  };

  // ========================================================================
  // QML Interface
  // ========================================================================
  QHash<int, QByteArray> roleNames () const override;
  int rowCount (const QModelIndex &parent = QModelIndex ()) const override;
  QVariant
  data (const QModelIndex &index, int role = Qt::DisplayRole) const override;

  Q_INVOKABLE void
  showNextAvailableAutomationTrack (AutomationTrack * current_automation_track);
  Q_INVOKABLE void
  hideAutomationTrack (AutomationTrack * current_automation_track);

  // ========================================================================

  friend void init_from (
    AutomationTracklist       &obj,
    const AutomationTracklist &other,
    utils::ObjectCloneType     clone_type);

  /**
   * Inits a loaded AutomationTracklist.
   */
  [[gnu::cold]] void init_loaded ();

  TrackPtrVariant get_track () const;

  /**
   * @brief Adds the given automation track.
   *
   * This takes (QObject) ownership of the AutomationTrack.
   */
  AutomationTrack * add_automation_track (AutomationTrack &at);

  /**
   * Prints info about all the automation tracks.
   *
   * Used for debugging.
   */
  void print_ats () const;

  AutomationTrack * get_prev_visible_at (const AutomationTrack &at) const;

  AutomationTrack * get_next_visible_at (const AutomationTrack &at) const;

  void set_at_visible (AutomationTrack &at, bool visible);

  AutomationTrack * get_automation_track_at (size_t index)
  {
    return ats_.at (index).get ();
  }

  auto &get_automation_tracks_in_record_mode () { return ats_in_record_mode_; }
  auto &get_automation_tracks_in_record_mode () const
  {
    return ats_in_record_mode_;
  }

  /**
   * Returns the AutomationTrack after delta visible AutomationTrack's.
   *
   * Negative delta searches backwards.
   *
   * This function searches tracks only in the same Tracklist as the given one
   * (ie, pinned or not).
   */
  AutomationTrack *
  get_visible_at_after_delta (const AutomationTrack &at, int delta) const;

  int
  get_visible_at_diff (const AutomationTrack &src, const AutomationTrack &dest)
    const;

  /**
   * Removes the AutomationTrack from the AutomationTracklist, optionally
   * freeing it.
   *
   * @return The removed automation track (in case we want to move it). Can be
   * ignored to let it get free'd when it goes out of scope.
   */
  utils::QObjectUniquePtr<AutomationTrack>
  remove_at (AutomationTrack &at, bool free, bool fire_events);

  /**
   * Removes the AutomationTrack's associated with this channel from the
   * AutomationTracklist in the corresponding Track.
   */
  void remove_channel_ats (Channel * ch);

  /**
   * Unselects all arranger objects.
   */
  void unselect_all ();

  /**
   * Removes all objects recursively.
   */
  void clear_objects ();

  /**
   * Sets the index of the AutomationTrack and swaps it with the
   * AutomationTrack at that index or pushes the other AutomationTrack's down.
   *
   * A special case is when @p index == @ref ats_.size(). In this case, the
   * given automation track is set last and all the other automation tracks
   * are pushed upwards.
   *
   * @param push_down False to swap positions with the current
   * AutomationTrack, or true to push down all the tracks below.
   */
  void
  set_automation_track_index (AutomationTrack &at, int index, bool push_down);

  /**
   * Used when the add button is added and a new automation track is requested
   * to be shown.
   *
   * Marks the first invisible automation track as visible, or marks an
   * uncreated one as created if all invisible ones are visible, and returns
   * it.
   */
  AutomationTrack * get_first_invisible_at () const;

  void
  append_objects (std::vector<arrangement::ArrangerObject *> objects) const;

  /**
   * Returns the number of visible AutomationTrack's.
   */
  int get_num_visible () const;

  /**
   * Counts the total number of regions in the automation tracklist.
   */
  int get_num_regions () const;

  void print_regions () const;

  auto &get_visible_automation_tracks () { return visible_ats_; }
  auto &get_visible_automation_tracks () const { return visible_ats_; }

  auto &get_automation_tracks () { return ats_; }
  auto &get_automation_tracks () const { return ats_; }

  dsp::ProcessorParameter &get_port (dsp::ProcessorParameter::Uuid id) const;

  void set_caches (CacheType types);

private:
  static constexpr std::string_view kAutomationTracksKey = "automationTracks";
  friend void to_json (nlohmann::json &j, const AutomationTracklist &ats)
  {
    j[kAutomationTracksKey] = ats.ats_;
  }
  friend void from_json (const nlohmann::json &j, AutomationTracklist &ats);

  auto &get_port_registry () { return port_registry_; }
  auto &get_port_registry () const { return port_registry_; }

private:
  dsp::FileAudioSourceRegistry    &file_audio_source_registry_;
  ArrangerObjectRegistry          &object_registry_;
  dsp::PortRegistry               &port_registry_;
  dsp::ProcessorParameterRegistry &param_registry_;

  /**
   * @brief Automation tracks in this automation tracklist.
   *
   * These should be updated with ALL of the automatables available in the
   * channel and its plugins every time there is an update.
   *
   * Active automation lanes that are shown in the UI, including hidden ones,
   * can be found using @ref AutomationTrack.created_ and @ref
   * AutomationTrack.visible_.
   *
   * Automation tracks become active automation lanes when they have
   * automation or are selected.
   */
  std::vector<utils::QObjectUniquePtr<AutomationTrack>> ats_;

  /**
   * Cache of automation tracks in record mode, used in recording manager to
   * avoid looping over all automation tracks.
   */
  std::vector<QPointer<AutomationTrack>> ats_in_record_mode_;

  /**
   * Cache of visible automation tracks.
   */
  std::vector<QPointer<AutomationTrack>> visible_ats_;

  /**
   * Owner track.
   */
  AutomatableTrack &track_;
};

}
