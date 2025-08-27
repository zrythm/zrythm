// SPDX-FileCopyrightText: © 2018-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "structure/arrangement/arranger_object.h"
#include "structure/tracks/track.h"
#include "structure/tracks/track_collection.h"
#include "structure/tracks/track_routing.h"
#include "structure/tracks/track_span.h"

#include <QtQmlIntegration>

namespace zrythm::structure::tracks
{
class ChordTrack;
class ModulatorTrack;
class MasterTrack;
class MarkerTrack;

/**
 * @brief References to tracks that are singletons in the tracklist.
 */
class SingletonTracks : public QObject
{
  Q_OBJECT
  Q_PROPERTY (
    zrythm::structure::tracks::ChordTrack * chordTrack READ chordTrack CONSTANT)
  Q_PROPERTY (
    zrythm::structure::tracks::ModulatorTrack * modulatorTrack READ
      modulatorTrack CONSTANT)
  Q_PROPERTY (
    zrythm::structure::tracks::MasterTrack * masterTrack READ masterTrack
      CONSTANT)
  Q_PROPERTY (
    zrythm::structure::tracks::MarkerTrack * markerTrack READ markerTrack
      CONSTANT)
  QML_ELEMENT
  QML_UNCREATABLE ("")

  friend class Tracklist;

public:
  SingletonTracks (QObject * parent = nullptr) : QObject (parent) { }

  ChordTrack *     chordTrack () const { return chord_track_; }
  ModulatorTrack * modulatorTrack () const { return modulator_track_; }
  MasterTrack *    masterTrack () const { return master_track_; }
  MarkerTrack *    markerTrack () const { return marker_track_; }

private:
  QPointer<ChordTrack>     chord_track_;
  QPointer<ModulatorTrack> modulator_track_;
  QPointer<MasterTrack>    master_track_;
  QPointer<MarkerTrack>    marker_track_;
};

/**
 * The Tracklist contains all the tracks in the Project.
 *
 * There should be a clear separation between the Tracklist and the Mixer. The
 * Tracklist should be concerned with Tracks in the arranger, and the Mixer
 * should be concerned with Channels, routing and Port connections.
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
    QVariant selectedTrack READ selectedTrack NOTIFY selectedTracksChanged)
  Q_PROPERTY (
    int pinnedTracksCutoff READ pinnedTracksCutoff WRITE setPinnedTracksCutoff
      NOTIFY pinnedTracksCutoffChanged)
  QML_UNCREATABLE ("")

public:
  using TrackUuid = Track::Uuid;
  using ArrangerObjectPtrVariant = arrangement::ArrangerObjectPtrVariant;
  using ArrangerObject = arrangement::ArrangerObject;

public:
  explicit Tracklist (TrackRegistry &track_registry, QObject * parent = nullptr);
  Z_DISABLE_COPY_MOVE (Tracklist)
  ~Tracklist () override = default;

  // ========================================================================
  // QML Interface
  // ========================================================================

  TrackCollection * collection () const { return track_collection_.get (); }

  SingletonTracks * singletonTracks () const
  {
    return singleton_tracks_.get ();
  }

  TrackRouting * trackRouting () const { return track_routing_.get (); }

  Q_INVOKABLE void setExclusivelySelectedTrack (QVariant track);

  QVariant      selectedTrack () const;
  Q_SIGNAL void selectedTracksChanged ();

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

  /**
   * @brief A list of track types that must be unique in the tracklist.
   */
  static constexpr std::array<Track::Type, 4> unique_track_types_ = {
    Track::Type::Chord, Track::Type::Marker, Track::Type::Modulator
  };

  auto get_track_span () const { return collection ()->get_track_span (); }

  friend void init_from (
    Tracklist             &obj,
    const Tracklist       &other,
    utils::ObjectCloneType clone_type);

  /**
   * Adds given track to given spot in tracklist.
   *
   * @return Pointer to the newly added track.
   */
  TrackPtrVariant insert_track (const TrackUuidReference &track_id, int pos);

  /**
   * Calls insert_track with the given options.
   */
  TrackPtrVariant append_track (const TrackUuidReference &track_id)
  {
    return insert_track (
      track_id, static_cast<int> (collection ()->track_count ()));
  }

  /**
   * Removes the given track from the tracklist.
   *
   * Also disconnects the channel (breaks its internal & external connections)
   * and removes any plugins (if any).
   */
  void remove_track (const TrackUuid &track_id);

  /**
   * Moves a track from its current position to the position given by @p pos.
   *
   * @param pos Position to insert at, or -1 to insert at the end.
   */
  void move_track (TrackUuid track_id, int pos);

  std::optional<TrackPtrVariant> get_track (const TrackUuid &id) const
  {
    auto span = get_track_span ();
    auto it = std::ranges::find (span, id, TrackSpan::uuid_projection);
    if (it == span.end ()) [[unlikely]]
      {
        return std::nullopt;
      }
    return std::make_optional (*it);
  }

  /**
   * Returns a unique name for a new track based on the given name.
   */
  utils::Utf8String get_unique_name_for_track (
    const Track::Uuid       &track_to_skip,
    const utils::Utf8String &name) const;

  /**
   * Returns the first visible Track.
   *
   * @param pinned 1 to check the pinned tracklist,
   *   0 to check the non-pinned tracklist.
   */
  std::optional<TrackPtrVariant> get_first_visible_track (bool pinned) const;

  /**
   * Returns the previous visible Track in the same
   * Tracklist as the given one (ie, pinned or not).
   */
  std::optional<TrackPtrVariant>
  get_prev_visible_track (Track::Uuid track_id) const;

  /**
   * Returns the next visible Track in the same
   * Tracklist as the given one (ie, pinned or not).
   */
  std::optional<TrackPtrVariant>
  get_next_visible_track (Track::Uuid track_id) const;

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
   * Returns the number of visible Tracks between src and dest (negative if
   * dest is before src).
   *
   * The caller is responsible for checking that both tracks are in the same
   * tracklist (ie, pinned or not).
   */
  int
  get_visible_track_diff (Track::Uuid src_track, Track::Uuid dest_track) const;

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
   * Returns whether the track name is not taken.
   *
   * @param track_to_skip Track to skip when searching.
   */
  bool
  track_name_is_unique (const utils::Utf8String &name, TrackUuid track_to_skip)
    const;

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

  TrackSelectionManager get_selection_manager () const
  {
    return *track_selection_manager_;
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
  static constexpr auto kSelectedTracksKey = "selectedTracks"sv;
  friend void           to_json (nlohmann::json &j, const Tracklist &t)
  {
    j = nlohmann::json{
      { kPinnedTracksCutoffKey, t.pinned_tracks_cutoff_ },
      { kTracksKey,             t.track_collection_     },
      { kSelectedTracksKey,     t.selected_tracks_      },
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

  /**
   * @brief A subset of tracks that are currently selected.
   *
   * There must always be at least 1 selected track.
   */
  TrackSelectionManager::UuidSet selected_tracks_;

  std::unique_ptr<TrackSelectionManager> track_selection_manager_;

  utils::QObjectUniquePtr<SingletonTracks> singleton_tracks_;

  /**
   * Index starting from which tracks are unpinned.
   *
   * Tracks before this position will be considered as pinned.
   */
  int pinned_tracks_cutoff_ = 0;

  /** When this is true, some tracks may temporarily be moved
   * beyond num_tracks. */
  std::atomic<bool> swapping_tracks_ = false;
};
}
