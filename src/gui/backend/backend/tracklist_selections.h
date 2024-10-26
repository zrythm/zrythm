// SPDX-FileCopyrightText: © 2019-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __ACTIONS_TRACKLIST_SELECTIONS_H__
#define __ACTIONS_TRACKLIST_SELECTIONS_H__

#include "common/dsp/audio_bus_track.h"
#include "common/dsp/audio_group_track.h"
#include "common/dsp/audio_track.h"
#include "common/dsp/chord_track.h"
#include "common/dsp/folder_track.h"
#include "common/dsp/instrument_track.h"
#include "common/dsp/marker_track.h"
#include "common/dsp/master_track.h"
#include "common/dsp/midi_bus_track.h"
#include "common/dsp/midi_group_track.h"
#include "common/dsp/midi_track.h"
#include "common/dsp/modulator_track.h"
#include "common/dsp/tempo_track.h"
#include "common/dsp/track.h"

/**
 * @addtogroup gui_backend
 *
 * @{
 */

#define TRACKLIST_SELECTIONS (PROJECT->tracklist_selections_)

/**
 * Selections to be used for copying, undoing, etc.
 *
 * Each track in this class is a copy (clone) of a corresponding track in the
 * project.
 */
class TracklistSelections final
    : public ICloneable<TracklistSelections>,
      public ISerializable<TracklistSelections>
{
public:
  TracklistSelections () = default;
  TracklistSelections (const Track &track)
  {
    tracks_.emplace_back (clone_unique_with_variant<TrackVariant> (&track));
  }

  void init_after_cloning (const TracklistSelections &other) override;

  DECLARE_DEFINE_FIELDS_METHOD ();

  void init_loaded ()
  {
    for (auto &track : tracks_)
      {
        // track->ts_ = this;
        track->init_loaded ();
      }
  }

  /**
   * @brief Adds @p track to @ref tracks_.
   */
  void add_track (std::unique_ptr<Track> &&track);

  template <typename DerivedTrackType = Track, typename Predicate>
  bool contains_track_matching (Predicate predicate) const;

  bool contains_undeletable_track () const;

  bool contains_uncopyable_track () const;

  bool contains_track_index (int track_idx) const;

  void get_plugins (std::vector<zrythm::plugins::Plugin *> &arr);

  /**
   * Gets lowest track in the selections.
   */
  Track * get_lowest_track () const;

  /**
   * @brief Paste to the given position in the tracklist (performs a copy).
   *
   * @param pos
   * @throw ZrythmException if failed to paste.
   */
  void paste_to_pos (int pos);

  void sort ()
  {
    std::sort (
      tracks_.begin (), tracks_.end (),
      [] (const auto &a, const auto &b) { return a->pos_ < b->pos_; });
  }

public:
  /**
   * @brief Clones of a list of tracks.
   *
   * Always sorted by @ref Track.pos_.
   */
  std::vector<std::unique_ptr<Track>> tracks_;
};

/**
 * @brief This class tracks selected tracks in the currently opened project by
 * name.
 *
 * It is invalid to allow 0 selections. There must always be a Track selected.
 */
class SimpleTracklistSelections
  final : public ISerializable<SimpleTracklistSelections>
{
public:
  SimpleTracklistSelections () = default;
  SimpleTracklistSelections (Tracklist &tracklist) : tracklist_ (&tracklist) {};

  void init_loaded (Tracklist &tracklist) { tracklist_ = &tracklist; }

  DECLARE_DEFINE_FIELDS_METHOD ();

  /**
   * Gets highest track in the selections.
   */
  Track * get_highest_track () const;

  /**
   * Gets lowest track in the selections.
   */
  Track * get_lowest_track () const;

  /**
   * Marks the tracks to be bounced.
   *
   * @param with_parents Also mark all the track's parents recursively.
   * @param mark_master Also mark the master track.
   *   Set to true when exporting the mixdown, false otherwise.
   */
  void mark_for_bounce (bool with_parents, bool mark_master);

  /**
   * Simply calls @ref TracklistSelections::paste_to_pos() after @ref
   * gen_tracklist_selections().
   */
  void paste_to_pos (int pos);

  /**
   * Toggle visibility of the selected tracks.
   */
  void toggle_visibility ();

  /**
   * Selects the last visible track after clearing the selections.
   */
  void select_last_visible ();

  /**
   * Selects a single track after clearing the selections.
   */
  void select_single (Track &track, bool fire_events);

  /**
   * Selects all Track's.
   *
   * @param visible_only Only select visible tracks.
   */
  void select_all (bool visible_only);

  void remove_track (Track &track, int fire_events);

  /**
   * Returns if the Track is selected or not.
   */
  bool contains_track (const Track &track);

  /**
   * Returns whether the tracklist selections contains a track that cannot have
   * automation lanes.
   */
  bool contains_non_automatable_track () const;

  /**
   * Returns whether the selections contain a soloed track if @ref soloed is
   * true or an unsoloed track if @ref soloed is false.
   *
   * @param soloed Whether to check for soloed or unsoloed tracks.
   */
  bool contains_soloed_track (bool soloed) const;

  /**
   * Returns whether the selections contain a listened track if @ref listened
   * is true or an unlistened track if @ref listened is false.
   *
   * @param listened Whether to check for listened or unlistened tracks.
   */
  bool contains_listened_track (bool listened) const;

  /**
   * Returns whether the selections contain a muted track if @ref muted is true
   * or an unmuted track if @ref muted is false.
   *
   * @param muted Whether to check for muted or unmuted tracks.
   */
  bool contains_muted_track (bool muted) const;

  bool contains_enabled_track (bool enabled) const;

  bool contains_undeletable_track () const;

  bool contains_uncopyable_track () const;

  bool contains_uninstantiated_plugin () const;

  template <typename DerivedTrackType = Track, typename Predicate>
  bool contains_track_matching (Predicate predicate) const;

  /**
   * Handle a click selection.
   */
  void handle_click (Track &track, bool ctrl, bool shift, bool dragged);

  /**
   * Make sure all children of foldable tracks in the selection are also
   * selected.
   */
  void select_foldable_children ();

  /**
   * @brief Adds a track to the selections.
   *
   * @param track The project tracklist track to add. Non-const because it
   * might become armed for recording.
   * @param fire_events
   */
  void add_track (Track &track, bool fire_events);

  void add_tracks_in_range (int min_pos, int max_pos, bool fire_events);

  /**
   * Clears the selections.
   */
  void clear (bool fire_events);

  size_t get_num_tracks () const { return track_names_.size (); }

  bool has_any () const { return !empty (); }
  bool empty () const { return track_names_.empty (); }

  /**
   * @brief Generates a new TracklistSelections based on the currently selected
   * tracks.
   *
   * @throw ZrythmException on error.
   */
  std::unique_ptr<TracklistSelections> gen_tracklist_selections () const;

  void sort ()
  {
    std::sort (
      track_names_.begin (), track_names_.end (),
      [] (const auto &a, const auto &b) {
        return Track::find_by_name (a)->pos_ < Track::find_by_name (b)->pos_;
      });
  }

public:
  /**
   * @brief List of track names.
   *
   * Always sorted by their corresponding position in @ref tracklist_ (if set).
   */
  std::vector<std::string> track_names_;

  /** Pointer to owner tracklist. */
  Tracklist * tracklist_ = nullptr;
};

DEFINE_OBJECT_FORMATTER (TracklistSelections, [] (const TracklistSelections &c) {
  std::string ret;
  ret += "TracklistSelections { \n";
  for (auto &track : c.tracks_)
    {
      ret += fmt::format ("[{}] {}\n", track->pos_, track->name_);
    }
  ret += "}";
  return ret;
})

/**
 * @}
 */

#endif
