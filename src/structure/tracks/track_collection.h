// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "structure/tracks/track_span.h"

#include <QtQmlIntegration/qqmlintegration.h>

namespace zrythm::structure::tracks
{

/**
 * @brief A collection of tracks that provides a QAbstractListModel interface.
 *
 * Tracks are stored in a flat list, and the model provides facilities to get
 * whether a track is foldable, expanded, and its depth (0 means not part of a
 * foldable parent).
 */
class TrackCollection : public QAbstractListModel
{
  Q_OBJECT
  Q_PROPERTY (
    int numSoloedTracks READ numSoloedTracks NOTIFY numSoloedTracksChanged)
  Q_PROPERTY (
    int numMutedTracks READ numMutedTracks NOTIFY numMutedTracksChanged)
  Q_PROPERTY (
    int numListenedTracks READ numListenedTracks NOTIFY numListenedTracksChanged)
  QML_ELEMENT
  QML_UNCREATABLE ("")

public:
  enum TrackRoles
  {
    TrackPtrRole = Qt::UserRole + 1,

    // Whether a track is foldable
    TrackFoldableRole,

    // Whether a foldable track is expanded
    TrackExpandedRole,

    // A foldable track's depth
    TrackDepthRole,

    TrackNameRole,
  };
  Q_ENUM (TrackRoles)

  TrackCollection (
    TrackRegistry &track_registry,
    QObject *      parent = nullptr) noexcept;
  ~TrackCollection () noexcept override;
  Z_DISABLE_COPY_MOVE (TrackCollection)

  // ========================================================================
  // QML Interface
  // ========================================================================

  QHash<int, QByteArray> roleNames () const override;
  int rowCount (const QModelIndex &parent = QModelIndex ()) const override;
  QVariant
  data (const QModelIndex &index, int role = Qt::DisplayRole) const override;

  Q_INVOKABLE void setTrackExpanded (const Track * track, bool expanded);

  int           numSoloedTracks () const;
  Q_SIGNAL void numSoloedTracksChanged ();
  int           numMutedTracks () const;
  Q_SIGNAL void numMutedTracksChanged ();
  int           numListenedTracks () const;
  Q_SIGNAL void numListenedTracksChanged ();

  // ========================================================================
  // Track Management
  // ========================================================================

  /**
   * @brief Get the track at the given index.
   */
  TrackPtrVariant get_track_at_index (size_t index) const;

  /**
   * @brief Get the track reference at the given index.
   */
  TrackUuidReference get_track_ref_at_index (size_t index) const;

  /**
   * @brief Get the index of the track with the given UUID.
   */
  auto get_track_index (const Track::Uuid &track_id) const
  {
    return std::distance (
      tracks_.begin (),
      std::ranges::find (tracks_, track_id, &TrackUuidReference::id));
  }

  TrackUuidReference track_ref_at_id (const Track::Uuid &track_id) const
  {
    return tracks ().at (get_track_index (track_id));
  }

  /**
   * @brief Get the number of tracks in the collection.
   */
  auto track_count () const { return tracks_.size (); }

  /**
   * @brief Check if the collection contains a track with the given UUID.
   */
  bool contains (const Track::Uuid &track_id) const;

  /**
   * @brief Add a track to the collection.
   */
  void add_track (const TrackUuidReference &track_id);

  /**
   * @brief Insert a track at the given position.
   */
  void insert_track (const TrackUuidReference &track_id, int pos);

  /**
   * @brief Remove a track from the collection.
   */
  void remove_track (const Track::Uuid &track_id);

  /**
   * @brief Remove a track from the collection without clearing its folder
   * metadata (expanded state and folder parent relationships).
   *
   * Use this when repositioning tracks that will be re-inserted immediately
   * (e.g., during undo/redo of a move operation), so that folder metadata is
   * preserved across the remove+insert cycle.
   */
  void detach_track (const Track::Uuid &track_id);

  /**
   * @brief Insert a track without initializing or modifying folder metadata.
   *
   * Unlike @ref insert_track, this does not auto-expand foldable tracks or
   * modify @c expanded_tracks_. Use after @ref detach_track to re-insert a
   * track while preserving its original folder state.
   */
  void reattach_track (const TrackUuidReference &track_id, int pos);

  /**
   * @brief Move a track from one position to another.
   */
  void move_track (const Track::Uuid &track_id, int pos);

  /**
   * @brief Clear all tracks from the collection.
   */
  void clear ();

  /**
   * @brief Get a span view of all tracks.
   */
  auto get_track_span () const { return TrackSpan{ tracks_ }; }

  /**
   * @brief Get the underlying tracks vector.
   */
  const std::vector<TrackUuidReference> &tracks () const { return tracks_; }

  /**
   * @brief Get the track registry.
   */
  TrackRegistry &get_track_registry () const { return track_registry_; }

  /**
   * @brief Set the expanded state of a foldable track.
   */
  void set_track_expanded (const Track::Uuid &track_id, bool expanded);

  /**
   * @brief Get the expanded state of a foldable track.
   */
  bool get_track_expanded (const Track::Uuid &track_id) const;

  /**
   * @brief Set the folder parent for a track.
   *
   * When @p auto_reposition is true, the child is automatically moved to be
   * the last child of the parent. When false, only the folder_parent_ entry
   * is updated (use when the caller handles positioning separately).
   */
  void set_folder_parent (
    const Track::Uuid &child_id,
    const Track::Uuid &parent_id,
    bool               auto_reposition = false);

  /**
   * @brief Get the folder parent for a track.
   */
  std::optional<Track::Uuid>
  get_folder_parent (const Track::Uuid &child_id) const;

  /**
   * @brief Remove the folder parent for a track.
   *
   * When @p auto_reposition is true, the child is automatically moved to
   * after the folder's last child. When false, only the folder_parent_ entry
   * is erased (use when the caller handles positioning separately).
   */
  void remove_folder_parent (
    const Track::Uuid &child_id,
    bool               auto_reposition = false);

  /**
   * @brief Check if a track is foldable.
   */
  bool is_track_foldable (const Track::Uuid &track_id) const;

  /**
   * @brief Check if @p possible_ancestor is an ancestor of @p track_id.
   *
   * Walks the folder_parent_ chain from @p track_id upward. Returns true if
   * @p possible_ancestor is found in the chain.
   */
  bool is_ancestor_of (
    const Track::Uuid &possible_ancestor,
    const Track::Uuid &track_id) const;

  /**
   * @brief Get the innermost enclosing folder at the given track index.
   *
   * Walks backward from the given index to find the nearest expanded foldable
   * track whose child range covers the index. Returns nullopt if the position
   * is not inside any folder.
   */
  std::optional<Track::Uuid> get_enclosing_folder (size_t index) const;

  /**
   * @brief Get the number of children for a foldable track.
   */
  size_t get_child_count (const Track::Uuid &parent_id) const;

  /**
   * @brief Get all descendant track UUIDs of a folder, in list order.
   *
   * Walks forward from the folder's position, collecting all tracks whose
   * folder_parent chain leads back to @p parent_id (direct children and
   * nested descendants).
   */
  std::vector<Track::Uuid>
  get_all_descendants (const Track::Uuid &parent_id) const;

  /**
   * @brief Get the last child index for a foldable track.
   */
  size_t get_last_child_index (const Track::Uuid &parent_id) const;

private:
  // ========================================================================
  // Serialization
  // ========================================================================

  static constexpr auto kTracksKey = "tracks"sv;
  static constexpr auto kExpandedTracksKey = "expandedTracks"sv;
  static constexpr auto kFolderParentsKey = "folderParents"sv;
  friend void to_json (nlohmann::json &j, const TrackCollection &collection);
  friend void from_json (const nlohmann::json &j, TrackCollection &collection);

private:
  TrackRegistry                  &track_registry_;
  std::vector<TrackUuidReference> tracks_;

  /**
   * @brief UUIDs of expanded foldable tracks.
   *
   * Tracks not in this set are considered collapsed.
   */
  std::unordered_set<Track::Uuid> expanded_tracks_;

  /**
   * @brief A map of folder parents.
   *
   * Key: child, value: parent.
   */
  std::unordered_map<Track::Uuid, Track::Uuid> folder_parent_;
};

} // namespace zrythm::structure::tracks
