// SPDX-FileCopyrightText: © 2019-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __GUI_BACKEND_TL_SELECTIONS_H__
#define __GUI_BACKEND_TL_SELECTIONS_H__

#include "common/dsp/midi_region.h"
#include "common/dsp/region.h"
#include "gui/backend/backend/arranger_selections.h"

/**
 * @addtogroup gui_backend
 *
 * @{
 */

#define TL_SELECTIONS (PROJECT->timeline_selections_)

/**
 * Selections to be used for the timeline's current selections, copying,
 * undoing, etc.
 */
class TimelineSelections final
    : public QObject,
      public ArrangerSelections,
      public ICloneable<TimelineSelections>,
      public ISerializable<TimelineSelections>
{
  Q_OBJECT
  QML_ELEMENT
  DEFINE_ARRANGER_SELECTIONS_QML_PROPERTIES (TimelineSelections)

public:
  TimelineSelections (QObject * parent = nullptr);

  /**
   * Creates a new TimelineSelections instance for the given range.
   */
  TimelineSelections (const Position &start_pos, const Position &end_pos);

  bool contains_looped () const override;

  bool can_be_merged () const override;

  void merge () override;

  /**
   * Returns whether all the selections are regions on the same lane (track lane
   * or automation lane).
   */
  bool all_on_same_lane () const;

  bool contains_clip (const AudioClip &clip) const override;

  void sort_by_indices (bool desc) override;

  /**
   * @brief Returns whether the selections contain a single region of type T.
   *
   * @param has_only_regions If true, it is required that no other types of
   * objects are included.
   * @return The single region if there is one, otherwise nullptr.
   */
  template <typename T = Region>
  T * contains_single_region (bool has_only_regions = false) const
  {
    T * r = nullptr;
    for (auto cur_obj : objects_ | type_is<T> ())
      {
        r = cur_obj;
      }
    if (!r)
      {
        return nullptr;
      }

    if (has_only_regions && !contains_only_regions ())
      {
        return nullptr;
      }

    return r;
  }

  /**
   * Gets the highest track in the selections.
   */
  OptionalTrackPtrVariant get_first_track () const;

  /**
   * Gets the lowest track in the selections.
   */
  OptionalTrackPtrVariant get_last_track () const;

  /**
   * @brief Returns a new vector containing only the objects derived from
   * @p T.
   *
   * @tparam T
   * @return std::vector<std::shared_ptr<T>>
   */
  template <typename T>
  std::vector<std::shared_ptr<T>> get_objects_of_type () const
  {
    std::vector<std::shared_ptr<T>> result;

    for (const auto &obj : objects_)
      {
        if (auto casted = std::dynamic_pointer_cast<T> (obj))
          {
            result.push_back (casted);
          }
      }
    return result;
  }

  /**
   * Replaces the track positions in each object with visible track indices
   * starting from 0.
   *
   * Used during copying.
   */
  void set_vis_track_indices ();

  /**
   * @param with_parents Also mark all the track's
   *   parents recursively.
   */
  void mark_for_bounce (bool with_parents);

  /**
   * Move the selected regions to new automation tracks.
   *
   * @return True if moved.
   */
  bool move_regions_to_new_ats (int vis_at_diff);

  /**
   * Move the selected Regions to new lanes.
   *
   * @param diff The delta to move the tracks.
   *
   * @return True if moved.
   */
  bool move_regions_to_new_lanes (int diff);

  /**
   * Move the selected Regions to the new Track.
   *
   * @param new_track_is_before 1 if the Region's should move to
   *   their previous tracks, 0 for their next tracks.
   *
   * @return True if moved.
   */
  bool move_regions_to_new_tracks (int vis_track_diff);

  /**
   * Sets the regions'
   * @ref Region.index_in_prev_lane.
   */
  void set_index_in_prev_lane ();

  [[nodiscard]] bool contains_only_regions () const;

  [[nodiscard]] bool contains_only_region_types (RegionType types) const;

  /**
   * Exports the selections to the given MIDI file.
   *
   * @throw ZrythmException on error.
   */
  void export_to_midi_file (
    const char * full_path,
    int          midi_version,
    bool         export_full_regions,
    bool         lanes_as_tracks) const;

  void init_after_cloning (const TimelineSelections &other) override
  {
    ArrangerSelections::copy_members_from (other);
    region_track_vis_index_ = other.region_track_vis_index_;
    chord_track_vis_index_ = other.chord_track_vis_index_;
    marker_track_vis_index_ = other.marker_track_vis_index_;
  }

  DECLARE_DEFINE_FIELDS_METHOD ();

private:
  bool move_regions_to_new_lanes_or_tracks_or_ats (
    int vis_track_diff,
    int lane_diff,
    int vis_at_diff);

  /**
   * Returns whether the selections can be pasted.
   *
   * Zrythm only supports pasting all the selections into a
   * single destination track.
   *
   * @param pos Position to paste to.
   * @param idx Track index to start pasting to.
   */
  bool can_be_pasted_at_impl (Position pos, int idx) const override;

public:
  /** Visible track index, used during copying. */
  int region_track_vis_index_ = 0;

  /** Visible track index, used during copying. */
  int chord_track_vis_index_ = 0;

  /** Visible track index, used during copying. */
  int marker_track_vis_index_ = 0;
};

DEFINE_OBJECT_FORMATTER (TimelineSelections, [] (const TimelineSelections &sel) {
  std::string objs;
  for (const auto &obj : sel.objects_)
    {
      std::visit ([&] (auto &&o) { objs += fmt::format ("{}\n", *o); }, obj);
    }
  return fmt::format (
    "TimelineSelections [{} objects: [\n{}]]", sel.get_num_objects (), objs);
})

/**
 * @}
 */

#endif
