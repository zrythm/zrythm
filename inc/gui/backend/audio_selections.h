// SPDX-FileCopyrightText: © 2020-2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * API for selections in the AudioArrangerWidget.
 */

#ifndef __GUI_BACKEND_AUDIO_SELECTIONS_H__
#define __GUI_BACKEND_AUDIO_SELECTIONS_H__

#include "dsp/position.h"
#include "dsp/region_identifier.h"
#include "gui/backend/arranger_selections.h"

typedef struct ZRegion ZRegion;

/**
 * @addtogroup gui_backend
 *
 * @{
 */

#define AUDIO_SELECTIONS (PROJECT->audio_selections)

/**
 * Selections to be used for the AudioArrangerWidget's
 * current selections, copying, undoing, etc.
 *
 * @extends ArrangerSelections
 */
typedef struct AudioSelections
{
  ArrangerSelections base;

  /** Whether or not a selection exists. */
  bool has_selection;

  /**
   * Selected range.
   *
   * The start position must always be before the
   * end position.
   *
   * Start position is included in the range, end
   * position is excluded.
   *
   * @note These are global positions and must
   * be adjusted for the region's start position.
   */
  Position sel_start;
  Position sel_end;

  /**
   * Audio pool ID of the associated audio file,
   * used during serialization.
   *
   * Set to -1 if unused.
   */
  int pool_id;

  /**
   * Identifier of the current region.
   *
   * Other types of selections don't need this
   * since their objects refer to it.
   */
  RegionIdentifier region_id;

} AudioSelections;

/**
 * Sets whether a range selection exists and sends
 * events to update the UI.
 *
 * @memberof AudioSelections
 */
void
audio_selections_set_has_range (AudioSelections * self, bool has_range);

/**
 * Returns if the selections can be pasted.
 *
 * @param pos Position to paste to.
 * @param region ZRegion to paste to.
 */
bool
audio_selections_can_be_pasted (AudioSelections * ts, Position * pos, ZRegion * r);

/**
 * @}
 */

#endif
