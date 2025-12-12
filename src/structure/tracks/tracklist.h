// SPDX-FileCopyrightText: © 2018-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "structure/arrangement/arranger_object.h"
#include "structure/tracks/singleton_tracks.h"
#include "structure/tracks/track.h"
#include "structure/tracks/track_collection.h"
#include "structure/tracks/track_routing.h"
#include "structure/tracks/track_span.h"

#include <QtQmlIntegration/qqmlintegration.h>

namespace zrythm::structure::tracks
{

/**
 * @brief A higher level wrapper over a track collection that serves as the
 * project's only tracklist.
 */
class Tracklist : public QObject
{
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY (
    zrythm::structure::tracks::TrackRouting * trackRouting READ trackRouting
      CONSTANT)
  Q_PROPERTY (
    zrythm::structure::tracks::SingletonTracks * singletonTracks READ
      singletonTracks CONSTANT)
  Q_PROPERTY (
    zrythm::structure::tracks::TrackCollection * collection READ collection
      CONSTANT)
  Q_PROPERTY (
    int pinnedTracksCutoff READ pinnedTracksCutoff WRITE setPinnedTracksCutoff
      NOTIFY pinnedTracksCutoffChanged)
  QML_UNCREATABLE ("")

public:
  using ArrangerObjectPtrVariant = arrangement::ArrangerObjectPtrVariant;
  using ArrangerObject = arrangement::ArrangerObject;

public:
  explicit Tracklist (TrackRegistry &track_registry, QObject * parent = nullptr);
  Z_DISABLE_COPY_MOVE (Tracklist)
  ~Tracklist () override;

  // ========================================================================
  // QML Interface
  // ========================================================================

  TrackCollection * collection () const { return track_collection_.get (); }

  SingletonTracks * singletonTracks () const
  {
    return singleton_tracks_.get ();
  }

  Q_INVOKABLE Track * getTrackForTimelineObject (
    const arrangement::ArrangerObject * timelineObject) const;
  Q_INVOKABLE TrackLane * getTrackLaneForObject (
    const arrangement::ArrangerObject * timelineObject) const;

  TrackRouting * trackRouting () const { return track_routing_.get (); }

  int  pinnedTracksCutoff () const { return pinned_tracks_cutoff_; }
  void setPinnedTracksCutoff (int index)
  {
    if (index == pinned_tracks_cutoff_)
      return;

    pinned_tracks_cutoff_ = index;
    Q_EMIT pinnedTracksCutoffChanged (index);
  }
  Q_SIGNAL void pinnedTracksCutoffChanged (int index);

  // ========================================================================

  friend void init_from (
    Tracklist             &obj,
    const Tracklist       &other,
    utils::ObjectCloneType clone_type);

  std::optional<TrackPtrVariant> get_track (const TrackUuid &id) const
  {
    auto span = collection ()->get_track_span ();
    auto it = std::ranges::find (span, id, TrackSpan::uuid_projection);
    if (it == span.end ()) [[unlikely]]
      {
        return std::nullopt;
      }
    return std::make_optional (*it);
  }

  /**
   * Returns the Track after delta visible Track's.
   *
   * Negative delta searches backwards.
   *
   * This function searches tracks only in the same Tracklist as the given one
   * (ie, pinned or not).
   */
  std::optional<TrackPtrVariant>
  get_visible_track_after_delta (Track::Uuid track_id, int delta) const;

  /**
   * Multiplies all tracks' heights and returns if the operation was valid.
   *
   * @param visible_only Only apply to visible tracks.
   */
  bool multiply_track_heights (
    double multiplier,
    bool   visible_only,
    bool   check_only,
    bool   fire_events);

  /**
   * Handle a click selection.
   */
  void handle_click (TrackUuid track_id, bool ctrl, bool shift, bool dragged);

  std::vector<ArrangerObjectPtrVariant> get_timeline_objects () const;

  /**
   * @brief Clears either the timeline selections or the clip editor selections.
   *
   * @param object_id The object that is part of the target selections.
   */
  void
  clear_selections_for_object_siblings (const ArrangerObject::Uuid &object_id);

  std::optional<TrackUuidReference>
  get_track_for_plugin (const plugins::Plugin::Uuid &plugin_id) const;

  /**
   * Returns whether the track should be visible.
   *
   * Takes into account Track.visible and whether any of the track's foldable
   * parents are folded.
   */
  bool should_be_visible (const Track::Uuid &track_id) const;

  /**
   * @brief Returns whether the track at @p index is pinned.
   */
  bool is_track_pinned (int index) const
  {
    return index < pinned_tracks_cutoff_;
  }

  bool is_track_pinned (Track::Uuid track_id) const
  {
    return is_track_pinned (
      static_cast<int> (collection ()->get_track_index (track_id)));
  }

  auto get_track_route_target (const TrackUuid &source_track) const
  {
    return track_routing_->get_output_track (source_track);
  }

  /**
   * Marks the track for bouncing.
   *
   * @param mark_children Whether to mark all children tracks as well. Used
   * when exporting stems on the specific track stem only. IMPORTANT:
   * Track.bounce_to_master must be set beforehand if this is true.
   * @param mark_parents Whether to mark all parent tracks as well.
   */
  void mark_track_for_bounce (
    TrackPtrVariant track_var,
    bool            bounce,
    bool            mark_regions,
    bool            mark_children,
    bool            mark_parents);

private:
  static constexpr auto kPinnedTracksCutoffKey = "pinnedTracksCutoff"sv;
  static constexpr auto kTracksKey = "tracks"sv;
  friend void           to_json (nlohmann::json &j, const Tracklist &t)
  {
    j = nlohmann::json{
      { kPinnedTracksCutoffKey, t.pinned_tracks_cutoff_ },
      { kTracksKey,             t.track_collection_     },
    };
  }
  friend void from_json (const nlohmann::json &j, Tracklist &t);

private:
  auto &get_track_registry () const { return track_registry_; }
  auto &get_track_registry () { return track_registry_; }

private:
  TrackRegistry &track_registry_;

  /**
   * All tracks that exist.
   *
   * These should always be sorted in the same way they should appear in the
   * GUI and include hidden tracks.
   *
   * Pinned tracks should have lower indices. Ie, the sequence must be:
   * {
   *   pinned track,
   *   pinned track,
   *   pinned track,
   *   track,
   *   track,
   *   track,
   *   ...
   * }
   */
  utils::QObjectUniquePtr<TrackCollection> track_collection_;

  utils::QObjectUniquePtr<TrackRouting> track_routing_;

  utils::QObjectUniquePtr<SingletonTracks> singleton_tracks_;

  /**
   * Index starting from which tracks are unpinned.
   *
   * Tracks before this position will be considered as pinned.
   */
  int pinned_tracks_cutoff_ = 0;
};
}
