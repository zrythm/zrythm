// SPDX-FileCopyrightText: © 2018-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * @file
 *
 * Tracklist backend.
 */

#ifndef __AUDIO_TRACKLIST_H__
#define __AUDIO_TRACKLIST_H__

#include "gui/dsp/track.h"

#include <QtQmlIntegration>

struct FileImportInfo;

/**
 * @addtogroup dsp
 *
 * @{
 */

#define TRACKLIST (PROJECT->tracklist_)

class ChordTrack;
class TempoTrack;
class ModulatorTrack;
class MasterTrack;
class MarkerTrack;
class StringArray;

/**
 * The Tracklist contains all the tracks in the Project.
 *
 * There should be a clear separation between the Tracklist and the Mixer. The
 * Tracklist should be concerned with Tracks in the arranger, and the Mixer
 * should be concerned with Channels, routing and Port connections.
 */
class Tracklist final
    : public QAbstractListModel,
      public ICloneable<Tracklist>,
      virtual public zrythm::utils::serialization::ISerializable<Tracklist>
{
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY (TempoTrack * tempoTrack READ getTempoTrack CONSTANT FINAL)

public:
  /**
   * Used in track search functions.
   */
  enum class PinOption
  {
    PinnedOnly,
    UnpinnedOnly,
    Both,
  };

  enum TrackRoles
  {
    TrackPtrRole = Qt::UserRole + 1,
    TrackNameRole,
  };

public:
  Tracklist (QObject * parent = nullptr);
  Tracklist (Project &project, PortConnectionsManager * port_connections_manager);
  Tracklist (
    SampleProcessor         &sample_processor,
    PortConnectionsManager * port_connections_manager);
  Q_DISABLE_COPY_MOVE (Tracklist)
  ~Tracklist () override;

  // ========================================================================
  // QML Interface
  // ========================================================================
  QHash<int, QByteArray> roleNames () const override;
  int rowCount (const QModelIndex &parent = QModelIndex ()) const override;
  QVariant
  data (const QModelIndex &index, int role = Qt::DisplayRole) const override;

  TempoTrack * getTempoTrack () const;

  // ========================================================================

  /**
   * @brief A list of track types that must be unique in the tracklist.
   */
  static constexpr std::array<Track::Type, 4> unique_track_types_ = {
    Track::Type::Chord, Track::Type::Marker, Track::Type::Tempo,
    Track::Type::Modulator
  };

  bool is_in_active_project () const;

  bool is_auditioner () const
  {
    return sample_processor_ && is_in_active_project ();
  }

  void init_after_cloning (const Tracklist &other) override;

  /**
   * Initializes the tracklist when loading a project.
   */
  void init_loaded (Project * project, SampleProcessor * sample_processor);

  void init_loaded (Project &project) { init_loaded (&project, nullptr); }
  void init_loaded (SampleProcessor &sample_processor)
  {
    init_loaded (nullptr, &sample_processor);
  }

  /**
   * Selects or deselects all tracks.
   *
   * @note When deselecting the last track will become
   *   selected (there must always be >= 1 tracks
   *   selected).
   */
  void select_all (bool select, bool fire_events);

  size_t get_num_tracks () const { return tracks_.size (); }

  /**
   * Finds visible tracks and puts them in given array.
   */
  void get_visible_tracks (std::vector<Track *> visible_tracks) const;

  /**
   * Returns the Track matching the given name, if any.
   */
  std::optional<TrackPtrVariant>
  find_track_by_name (const std::string &name) const;

  /**
   * Returns the Track matching the given name, if any.
   */
  std::optional<TrackPtrVariant>
  find_track_by_name_hash (unsigned int hash) const;

  template <typename T> bool contains_track_type () const
  {
    return std::any_of (tracks_.begin (), tracks_.end (), [] (const auto &track) {
      return std::visit (
        [] (auto &&t) { return std::is_same_v<T, base_type<decltype (t)>>; },
        track);
    });
  }

  bool contains_master_track () const;

  bool contains_chord_track () const;

  /**
   * Prints the tracks (for debugging).
   */
  void print_tracks () const;

  /**
   * Adds given track to given spot in tracklist.
   *
   * @param publish_events Publish UI events.
   * @param recalc_graph Recalculate routing graph.
   * @return Pointer to the newly added track.
   */
  template <FinalTrackSubclass T>
  T * insert_track (
    std::unique_ptr<T> &&track,
    int                  pos,
    AudioEngine         &engine,
    bool                 publish_events,
    bool                 recalc_graph);

  Track * insert_track (
    std::unique_ptr<Track> &&track,
    int                      pos,
    AudioEngine             &engine,
    bool                     publish_events,
    bool                     recalc_graph);

  /**
   * Calls insert_track with the given options.
   */
  template <FinalTrackSubclass T>
  T * append_track (
    std::unique_ptr<T> &&track,
    AudioEngine         &engine,
    bool                 publish_events,
    bool                 recalc_graph)
  {
    return insert_track<T> (
      std::move (track), tracks_.size (), engine, publish_events, recalc_graph);
  }

  Track * append_track (
    std::unique_ptr<Track> &&track,
    AudioEngine             &engine,
    bool                     publish_events,
    bool                     recalc_graph);

  /**
   * Removes a track from the Tracklist and the TracklistSelections.
   *
   * Also disconnects the channel.
   *
   * @param rm_pl Remove plugins or not.
   * @param free Free the track or not (free later).
   * @param publish_events Push a track deleted event to the UI.
   * @param recalc_graph Recalculate the mixer graph.
   */
  void remove_track (
    Track &track,
    bool   rm_pl,
    bool   free_track,
    bool   publish_events,
    bool   recalc_graph);

  /**
   * Moves a track from its current position to the position given by @p pos.
   *
   * @param pos Position to insert at, or -1 to insert at the end.
   * @param always_before_pos Whether the track should always be put before the
   * track currently at @p pos. If this is true, when moving down, the resulting
   * track position will be @p pos - 1.
   * @param recalc_graph Recalculate routing graph.
   */
  void
  move_track (Track &track, int pos, bool always_before_pos, bool recalc_graph);

  /**
   * Pins or unpins the Track.
   */
  void set_track_pinned (
    Track &track,
    bool   pinned,
    int    publish_events,
    int    recalc_graph);

  bool validate () const;

  /**
   * Returns the track at the given index or NULL if the index is invalid.
   *
   * Not to be used in real-time code.
   */
  ATTR_HOT TrackPtrVariant get_track (int idx) { return tracks_.at (idx); }

  /**
   * Returns the index of the given Track.
   */
  int get_track_pos (Track &track) const;

  /**
   * Returns the first track found with the given type.
   */
  template <TrackSubclass T> T * get_track_by_type () const
  {
    for (const auto &track : tracks_)
      {
        if (std::holds_alternative<T *> (track))
          return std::get<T *> (track);
      }
    return nullptr;
  }

  ChordTrack * get_chord_track () const;

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
  get_prev_visible_track (const Track &track) const;

  /**
   * Returns the next visible Track in the same
   * Tracklist as the given one (ie, pinned or not).
   */
  std::optional<TrackPtrVariant>
  get_next_visible_track (const Track &track) const;

  /**
   * Returns the index of the last Track.
   *
   * @param pin_opt Pin option.
   * @param visible_only Only consider visible
   *   Track's.
   */
  int get_last_pos (
    const PinOption pin_opt = PinOption::Both,
    const bool      visible_only = false) const;

  /**
   * Returns the last Track.
   *
   * @param pin_opt Pin option.
   * @param visible_only Only consider visible  Track's.
   */
  std::optional<TrackPtrVariant> get_last_track (
    const PinOption pin_opt = PinOption::Both,
    const bool      visible_only = false) const;

  /**
   * Returns the Track after delta visible Track's.
   *
   * Negative delta searches backwards.
   *
   * This function searches tracks only in the same Tracklist as the given one
   * (ie, pinned or not).
   */
  std::optional<TrackPtrVariant>
  get_visible_track_after_delta (Track &track, int delta) const;

  /**
   * Returns the number of visible Tracks between src and dest (negative if
   * dest is before src).
   *
   * The caller is responsible for checking that both tracks are in the same
   * tracklist (ie, pinned or not).
   */
  int get_visible_track_diff (const Track &src, const Track &dest) const;

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
   * @brief Imports regions from a region array.
   *
   * @param region_arrays
   * @param import_info
   * @param ready_cb
   * @throw ZrythmException on error.
   */
  void import_regions (
    std::vector<std::vector<std::shared_ptr<Region>>> &region_arrays,
    const FileImportInfo *                             import_info,
    TracksReadyCallback                                ready_cb);

  /**
   * Begins file import Handles a file drop inside the timeline or in empty
   * space in the tracklist.
   *
   * @param uri_list URI list, if URI list was dropped.
   * @param file File, if FileDescriptor was dropped.
   * @param track Track, if any.
   * @param lane TrackLane, if any.
   * @param index Index to insert new tracks at, or -1 to insert at end.
   * @param pos Position the file was dropped at, if inside track.
   *
   * @throw ZrythmException on error.
   */
  void import_files (
    const StringArray *           uri_list,
    const FileDescriptor *        orig_file,
    const Track *                 track,
    const TrackLane *             lane,
    int                           index,
    const zrythm::dsp::Position * pos,
    TracksReadyCallback           ready_cb);

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

  /**
   * Returns whether the track name is not taken.
   *
   * @param track_to_skip Track to skip when searching.
   */
  bool
  track_name_is_unique (const std::string &name, Track * track_to_skip) const;

  /**
   * Returns if the tracklist has soloed tracks.
   */
  bool has_soloed () const;

  /**
   * Returns if the tracklist has listened tracks.
   */
  bool has_listened () const;

  int get_num_muted_tracks () const;

  int get_num_soloed_tracks () const;

  int get_num_listened_tracks () const;

  /**
   * @brief Returns the number of visible or invisible tracks.
   */
  int get_num_visible_tracks (bool visible) const;

  /**
   * Fills in the given array with all plugins in the tracklist.
   */
  void
  get_plugins (std::vector<zrythm::gui::old_dsp::plugins::Plugin *> &arr) const;

  /**
   * Activate or deactivate all plugins.
   *
   * This is useful for exporting: deactivating and
   * reactivating a plugin will reset its state.
   */
  void activate_all_plugins (bool activate);

  /**
   * Exposes each track's ports that should be exposed to the backend.
   *
   * This should be called after setting up the engine.
   */
  void expose_ports_to_backend (AudioEngine &engine);

  /**
   * Marks or unmarks all tracks for bounce.
   */
  void mark_all_tracks_for_bounce (bool bounce);

  /**
   * @brief Get the total (max) bars in the tracklist.
   *
   * @param total_bars Current known total bars
   * @return New total bars if > than @p total_bars, or @p total_bars.
   */
  int get_total_bars (const Transport &transport, int total_bars) const;

  /**
   * Set various caches (snapshots, track name hashes, plugin
   * input/output ports, etc).
   */
  void set_caches (CacheType types);

  DECLARE_DEFINE_FIELDS_METHOD ();

private:
  void swap_tracks (const size_t index1, const size_t index2);

  static void move_after_copying_or_moving_inside (
    TracklistSelections &after_tls,
    int                  diff_between_track_below_and_parent);

public:
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
  std::vector<TrackPtrVariant> tracks_;

  /** The chord track, for convenience. */
  ChordTrack * chord_track_ = nullptr;

  /** The marker track, for convenience. */
  MarkerTrack * marker_track_ = nullptr;

  /** The tempo track, for convenience. */
  TempoTrack * tempo_track_ = nullptr;

  /** The modulator track, for convenience. */
  ModulatorTrack * modulator_track_ = nullptr;

  /** The master track, for convenience. */
  MasterTrack * master_track_ = nullptr;

  /**
   * Index starting from which tracks are unpinned.
   *
   * Tracks before this position will be considered as pinned.
   */
  int pinned_tracks_cutoff_ = 0;

  /** When this is true, some tracks may temporarily be moved
   * beyond num_tracks. */
  bool swapping_tracks_ = false;

  /** Pointer to owner sample processor, if any. */
  SampleProcessor * sample_processor_ = nullptr;

  /** Pointer to owner project, if any. */
  Project * project_ = nullptr;

  /** Width of track widgets. */
  int width_ = 0;

  QPointer<PortConnectionsManager> port_connections_manager_;
};

/**
 * @}
 */

#endif
