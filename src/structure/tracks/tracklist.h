// SPDX-FileCopyrightText: © 2018-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "dsp/parameter.h"
#include "dsp/port.h"
#include "structure/arrangement/arranger_object.h"
#include "structure/tracks/track.h"
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
class Tracklist final : public QAbstractListModel
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
    QVariant selectedTrack READ selectedTrack NOTIFY selectedTracksChanged)
  QML_UNCREATABLE ("")

public:
  using TrackUuid = Track::Uuid;
  using ArrangerObjectPtrVariant = arrangement::ArrangerObjectPtrVariant;
  using ArrangerObject = arrangement::ArrangerObject;

  enum TrackRoles
  {
    TrackPtrRole = Qt::UserRole + 1,
    TrackNameRole,
  };

public:
  explicit Tracklist (
    dsp::PortRegistry               &port_registry,
    dsp::ProcessorParameterRegistry &param_registry,
    TrackRegistry                   &track_registry,
    QObject *                        parent = nullptr);
  Z_DISABLE_COPY_MOVE (Tracklist)

  // ========================================================================
  // QML Interface
  // ========================================================================
  QHash<int, QByteArray> roleNames () const override;
  int rowCount (const QModelIndex &parent = QModelIndex ()) const override;
  QVariant
  data (const QModelIndex &index, int role = Qt::DisplayRole) const override;

  SingletonTracks * singletonTracks () const
  {
    return singleton_tracks_.get ();
  }

  TrackRouting * trackRouting () const { return track_routing_.get (); }

  Q_INVOKABLE void setExclusivelySelectedTrack (QVariant track);

  QVariant      selectedTrack () const;
  Q_SIGNAL void selectedTracksChanged ();

  // ========================================================================

  /**
   * @brief A list of track types that must be unique in the tracklist.
   */
  static constexpr std::array<Track::Type, 4> unique_track_types_ = {
    Track::Type::Chord, Track::Type::Marker, Track::Type::Modulator
  };

  auto get_track_span () const { return TrackSpan{ tracks_ }; }

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
    return insert_track (track_id, tracks_.size ());
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
   * @param always_before_pos Whether the track should always be put before the
   * track currently at @p pos. If this is true, when moving down, the resulting
   * track position will be @p pos - 1.
   * @param router If given, the processing graph will be soft-recalculated.
   */
  void move_track (TrackUuid track_id, int pos, bool always_before_pos);

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
   * @brief Returns the region at the given position, if any.
   *
   * Either @p at (for automation regions) or @p track (for other regions)
   * must be passed.
   *
   * @param track
   * @param at
   * @param include_region_end
   */
  static std::optional<ArrangerObjectPtrVariant> get_region_at_pos (
    signed_frame_t            pos_samples,
    Track *                   track,
    tracks::AutomationTrack * at,
    bool                      include_region_end = false);

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

  /**
   * Moves the Region to the given Track, maintaining the selection
   * status of the Region.
   *
   * Assumes that the Region is already in a TrackLane or
   * AutomationTrack.
   *
   * @param lane_or_at_index If MIDI or audio, lane position.
   *   If automation, automation track index in the automation
   * tracklist. If -1, the track lane or automation track index will be
   * inferred from the region.
   * @param index If MIDI or audio, index in lane in the new track to
   * insert the region to, or -1 to append. If automation, index in the
   * automation track.
   *
   * @throw ZrythmException on error.
   */
  void move_region_to_track (
    ArrangerObjectPtrVariant region,
    const Track::Uuid       &to_track_id,
    int                      lane_or_at_index,
    int                      index);

  std::optional<TrackUuidReference>
  get_track_for_plugin (const plugins::Plugin::Uuid &plugin_id) const;

#if 0
  /**
   * Handles a move or copy action based on a drag.
   *
   * @param this_track The track at the cursor (where the selection was
   * dropped to.
   * @param location Location relative to @p this_track.
   */
  void handle_move_or_copy (
    Track               &this_track,
    TrackWidgetHighlight location,
    GdkDragAction        action);
#endif

// TODO
#if 0
  /**
   * Adds the track's folder parents to the given vector.
   *
   * @param prepend Whether to prepend instead of append.
   */
  void add_folder_parents (
    const Track::Uuid            &track_id,
    std::vector<FoldableTrack *> &parents,
    bool                          prepend) const;

  /**
   * Returns the closest foldable parent or NULL.
   */
  FoldableTrack * get_direct_folder_parent (const Track::Uuid &track_id) const
  {
    std::vector<FoldableTrack *> parents;
    add_folder_parents (track_id, parents, true);
    if (!parents.empty ())
      {
        return parents.front ();
      }
    return nullptr;
  }

  /**
   * Remove the track from all folders.
   *
   * Used when deleting tracks.
   */
  void remove_from_folder_parents (const Track::Uuid &track_id);
#endif

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
  bool is_track_pinned (size_t index) const
  {
    return index < pinned_tracks_cutoff_;
  }

  auto get_track_index (const Track::Uuid &track_id) const
  {
    return std::distance (
      tracks_.begin (),
      std::ranges::find (tracks_, track_id, &TrackUuidReference::id));
  }

  auto get_track_at_index (size_t index) const
  {
    if (index >= tracks_.size ())
      throw std::out_of_range ("Track index out of range");
    return get_track_span ().at (index);
  }

  auto get_track_ref_at_index (size_t index) const
  {
    if (index >= tracks_.size ())
      throw std::out_of_range ("Track index out of range");
    return tracks_.at (index);
  }

  bool is_track_pinned (Track::Uuid track_id) const
  {
    return is_track_pinned (get_track_index (track_id));
  }

  TrackSelectionManager get_selection_manager () const
  {
    return *track_selection_manager_;
  }

#if 0
  /**
   * @brief Also selects the children of foldable tracks in the currently
   * selected tracks.
   */
  void select_foldable_children_of_current_selections ()
  {
    auto      selected_vec = std::ranges::to<std::vector> (selected_tracks_);
    TrackSpan span{ *track_registry_, selected_vec };
    std::ranges::for_each (span, [&] (auto &&track_var) {
      std::visit (
        [&] (auto &&track_ref) {
          using TrackT = base_type<decltype (track_ref)>;
          if constexpr (std::derived_from<TrackT, FoldableTrack>)
            {
              for (int i = 1; i < track_ref->size_; ++i)
                {
                  const size_t child_pos =
                    get_track_index (track_ref->get_uuid ()) + i;
                  const auto &child_track_var = get_track_at_index (child_pos);
                  get_selection_manager ().append_to_selection (
                    TrackSpan::uuid_projection (child_track_var));
                }
            }
        },
        track_var);
    });
  }
#endif

  auto get_track_route_target (const TrackUuid &source_track) const
  {
    return track_routing_->get_output_track (source_track);
  }

  auto get_pinned_tracks_cutoff_index () const { return pinned_tracks_cutoff_; }
  void set_pinned_tracks_cutoff_index (size_t index)
  {
    pinned_tracks_cutoff_ = index;
  }
  auto track_count () const { return tracks_.size (); }

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
      { kTracksKey,             t.tracks_               },
      { kSelectedTracksKey,     t.selected_tracks_      },
    };
  }
  friend void from_json (const nlohmann::json &j, Tracklist &t);

  void swap_tracks (size_t index1, size_t index2);

private:
  auto &get_track_registry () const { return track_registry_; }
  auto &get_track_registry () { return track_registry_; }

private:
  TrackRegistry                   &track_registry_;
  dsp::PortRegistry               &port_registry_;
  dsp::ProcessorParameterRegistry &param_registry_;

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
  std::vector<TrackUuidReference> tracks_;

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
  size_t pinned_tracks_cutoff_ = 0;

  /** When this is true, some tracks may temporarily be moved
   * beyond num_tracks. */
  std::atomic<bool> swapping_tracks_ = false;
};
}
