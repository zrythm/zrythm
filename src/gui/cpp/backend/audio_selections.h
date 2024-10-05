// SPDX-FileCopyrightText: © 2020-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * @file
 *
 * API for selections in the AudioArrangerWidget.
 */

#ifndef __GUI_BACKEND_AUDIO_SELECTIONS_H__
#define __GUI_BACKEND_AUDIO_SELECTIONS_H__

#include "common/dsp/position.h"
#include "common/dsp/region_identifier.h"
#include "gui/cpp/backend/arranger_selections.h"

class Region;

/**
 * @addtogroup gui_backend
 *
 * @{
 */

#define AUDIO_SELECTIONS (PROJECT->audio_selections_)

/**
 * Selections to be used for the AudioArrangerWidget's current selections,
 * copying, undoing, etc.
 */
class AudioSelections final
    : public ArrangerSelections,
      public ICloneable<AudioSelections>,
      public ISerializable<AudioSelections>
{
public:
  AudioSelections ();

  /**
   * Sets whether a range selection exists and sends events to update the UI.
   */
  void set_has_range (bool has_range);

  bool contains_clip (const AudioClip &clip) const final;

  bool has_any () const override { return has_selection_; }

  void init_after_cloning (const AudioSelections &other) override
  {
    ArrangerSelections::copy_members_from (other);
    has_selection_ = other.has_selection_;
    sel_start_ = other.sel_start_;
    sel_end_ = other.sel_end_;
    pool_id_ = other.pool_id_;
    region_id_ = other.region_id_;
  }

  void sort_by_indices (bool desc) override { }

  DECLARE_DEFINE_FIELDS_METHOD ();

private:
  bool can_be_pasted_at_impl (const Position pos, const int idx) const final;

public:
  /** Whether or not a selection exists. */
  bool has_selection_ = false;

  /**
   * Selected range.
   *
   * The start position must always be before the end position.
   *
   * Start position is included in the range, end position is excluded.
   *
   * @note These are global positions and must be adjusted for the region's
   * start position.
   */
  Position sel_start_ = {};
  Position sel_end_ = {};

  /**
   * Audio pool ID of the associated audio file, used during serialization.
   *
   * Set to -1 if unused.
   */
  int pool_id_ = -1;

  /**
   * Identifier of the current region.
   *
   * Other types of selections don't need this since their objects refer to it.
   */
  RegionIdentifier region_id_ = {};
};

/**
 * @}
 */

#endif
