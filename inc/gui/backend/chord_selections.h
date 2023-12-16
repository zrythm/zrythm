// SPDX-FileCopyrightText: © 2019, 2021 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * API for selections in the piano roll.
 */

#ifndef __GUI_BACKEND_CHORD_SELECTIONS_H__
#define __GUI_BACKEND_CHORD_SELECTIONS_H__

#include "dsp/chord_object.h"
#include "gui/backend/arranger_selections.h"
#include "utils/yaml.h"

/**
 * @addtogroup gui_backend
 *
 * @{
 */

#define CHORD_SELECTIONS_SCHEMA_VERSION 1

#define CHORD_SELECTIONS (PROJECT->chord_selections)

/**
 * Selections to be used for the ChordArrangerWidget's
 * current selections, copying, undoing, etc.
 */
typedef struct ChordSelections
{
  /** Base struct. */
  ArrangerSelections base;

  /** Selected ChordObject's.
   *
   * These are used for
   * serialization/deserialization only. */
  ChordObject ** chord_objects;
  int            num_chord_objects;
  size_t         chord_objects_size;

} ChordSelections;

/**
 * Returns if the selections can be pasted.
 *
 * @param pos Position to paste to.
 * @param region ZRegion to paste to.
 */
int
chord_selections_can_be_pasted (
  ChordSelections * ts,
  Position *        pos,
  ZRegion *         region);

/**
 * @}
 */

#endif
