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
#include "gui/dsp/track_span.h"

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
class Router;

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
  using TrackUuid = Track::Uuid;

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
  explicit Tracklist (
    Project                 &project,
    PortRegistry            &port_registry,
    TrackRegistry           &track_registry,
    PortConnectionsManager * port_connections_manager);
  explicit Tracklist (
    SampleProcessor         &sample_processor,
    PortRegistry            &port_registry,
    TrackRegistry           &track_registry,
    PortConnectionsManager * port_connections_manager);
  Z_DISABLE_COPY_MOVE (Tracklist)
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

  TrackRegistrySpan get_track_span () const;

  bool is_in_active_project () const;

  bool is_auditioner () const
  {
    return sample_processor_ && is_in_active_project ();
  }

  void init_after_cloning (const Tracklist &other, ObjectCloneType clone_type)
    override;

  /**
   * Initializes the tracklist when loading a project.
   */
  void init_loaded (
    PortRegistry     &port_registry,
    Project *         project,
    SampleProcessor * sample_processor);

  void init_loaded (PortRegistry &port_registry, Project &project)
  {
    init_loaded (port_registry, &project, nullptr);
  }
  void
  init_loaded (PortRegistry &port_registry, SampleProcessor &sample_processor)
  {
    init_loaded (port_registry, nullptr, &sample_processor);
  }

  /**
   * Adds given track to given spot in tracklist.
   *
   * @param publish_events Publish UI events.
   * @param recalc_graph Recalculate routing graph.
   * @return Pointer to the newly added track.
   */
  TrackPtrVariant insert_track (
    TrackUuid    track_id,
    int          pos,
    AudioEngine &engine,
    bool         publish_events,
    bool         recalc_graph);

  /**
   * Calls insert_track with the given options.
   */
  TrackPtrVariant append_track (
    TrackUuid    track_id,
    AudioEngine &engine,
    bool         publish_events,
    bool         recalc_graph)
  {
    return insert_track (
      track_id, tracks_.size (), engine, publish_events, recalc_graph);
  }

  /**
   * Removes a track from the Tracklist and the TracklistSelections.
   *
   * Also disconnects the channel.
   *
   * @param rm_pl Remove plugins or not.
   * @param router If given, the processing graph will be soft-recalculated.
   */
  void remove_track (
    TrackUuid                                     track_id,
    std::optional<std::reference_wrapper<Router>> router,
    bool                                          rm_pl);

  /**
   * Moves a track from its current position to the position given by @p pos.
   *
   * @param pos Position to insert at, or -1 to insert at the end.
   * @param always_before_pos Whether the track should always be put before the
   * track currently at @p pos. If this is true, when moving down, the resulting
   * track position will be @p pos - 1.
   * @param router If given, the processing graph will be soft-recalculated.
   */
  void move_track (
    TrackUuid                                     track_id,
    int                                           pos,
    bool                                          always_before_pos,
    std::optional<std::reference_wrapper<Router>> router);

  std::optional<TrackPtrVariant> get_track (const TrackUuid &id) const
  {
    auto span = get_track_span ();
    auto it = std::ranges::find (span, id, TrackRegistrySpan::uuid_projection);
    if (it == span.end ()) [[unlikely]]
      {
        return std::nullopt;
      }
    return std::make_optional (*it);
  }

  /**
   * Pins or unpins the Track.
   */
  void set_track_pinned (
    TrackUuid track_id,
    bool      pinned,
    int       publish_events,
    int       recalc_graph);

  bool validate () const;

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
  get_prev_visible_track (Track::TrackUuid track_id) const;

  /**
   * Returns the next visible Track in the same
   * Tracklist as the given one (ie, pinned or not).
   */
  std::optional<TrackPtrVariant>
  get_next_visible_track (Track::TrackUuid track_id) const;

  /**
   * Returns the index of the last Track.
   *
   * @param pin_opt Pin option.
   * @param visible_only Only consider visible
   *   Track's.
   */
  int
  get_last_pos (PinOption pin_opt = PinOption::Both, bool visible_only = false)
    const;

  /**
   * Returns the last Track.
   *
   * @param pin_opt Pin option.
   * @param visible_only Only consider visible  Track's.
   */
  std::optional<TrackPtrVariant>
  get_last_track (PinOption pin_opt = PinOption::Both, bool visible_only = false)
    const
  {
    return get_track_span ().get_track_by_pos (
      get_last_pos (pin_opt, visible_only));
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
  get_visible_track_after_delta (Track::TrackUuid track_id, int delta) const;

  /**
   * Returns the number of visible Tracks between src and dest (negative if
   * dest is before src).
   *
   * The caller is responsible for checking that both tracks are in the same
   * tracklist (ie, pinned or not).
   */
  int get_visible_track_diff (
    Track::TrackUuid src_track,
    Track::TrackUuid dest_track) const;

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
  track_name_is_unique (const std::string &name, TrackUuid track_to_skip) const;

  /**
   * @brief Returns whether the track at @p index is pinned.
   */
  bool is_track_pinned (size_t index) const
  {
    return index < pinned_tracks_cutoff_;
  }

  auto get_track_index (const Track::TrackUuid &track_id) const
  {
    return std::distance (
      tracks_.begin (), std::ranges::find (tracks_, track_id));
  }

  auto get_track_at_index (size_t index) const
  {
    if (index >= tracks_.size ())
      throw std::out_of_range ("Track index out of range");
    return get_track_span ().at (index);
  }

  bool is_track_pinned (Track::TrackUuid track_id) const
  {
    return is_track_pinned (get_track_index (track_id));
  }

  DECLARE_DEFINE_FIELDS_METHOD ();

private:
  void swap_tracks (size_t index1, size_t index2);

  TrackRegistry &get_track_registry () const;

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
  std::vector<Track::TrackUuid> tracks_;

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
  size_t pinned_tracks_cutoff_ = 0;

  /** When this is true, some tracks may temporarily be moved
   * beyond num_tracks. */
  std::atomic<bool> swapping_tracks_ = false;

  /** Pointer to owner sample processor, if any. */
  SampleProcessor * sample_processor_ = nullptr;

  /** Pointer to owner project, if any. */
  Project * project_ = nullptr;

  /** Width of track widgets. */
  int width_ = 0;

  QPointer<PortConnectionsManager> port_connections_manager_;

  std::optional<TrackRegistryRef>                         track_registry_;
  std::optional<PortRegistryRef>                          port_registry_;
  std::optional<gui::old_dsp::plugins::PluginRegistryRef> plugin_registry_;
};

/**
 * @}
 */

#endif
