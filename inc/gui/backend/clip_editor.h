// SPDX-FileCopyrightText: © 2019-2021 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * The clip/region editor backend.
 */

#ifndef __GUI_BACKEND_CLIP_EDITOR_H__
#define __GUI_BACKEND_CLIP_EDITOR_H__

#include "dsp/region_identifier.h"
#include "gui/backend/audio_clip_editor.h"
#include "gui/backend/automation_editor.h"
#include "gui/backend/chord_editor.h"
#include "gui/backend/piano_roll.h"

typedef struct Region             Region;
typedef struct ArrangerSelections ArrangerSelections;

/**
 * @addtogroup gui_backend
 *
 * @{
 */

#define CLIP_EDITOR (PROJECT->clip_editor)

/**
 * Clip editor serializable backend.
 *
 * The actual widgets should reflect the
 * information here.
 */
typedef struct ClipEditor
{
  /** Region currently attached to the clip
   * editor. */
  RegionIdentifier region_id;

  /** Whether \ref region_id is a valid region. */
  bool has_region;

  PianoRoll *        piano_roll;
  AudioClipEditor *  audio_clip_editor;
  AutomationEditor * automation_editor;
  ChordEditor *      chord_editor;

  /** Flag used by rulers when region first
   * changes. */
  int region_changed;

  /* --- caches --- */
  Region * region;

  Track * track;
} ClipEditor;

/**
 * Inits the ClipEditor after a Project is loaded.
 */
void
clip_editor_init_loaded (ClipEditor * self);

/**
 * Inits the clip editor.
 */
void
clip_editor_init (ClipEditor * self);

/**
 * Creates a new clip editor.
 */
ClipEditor *
clip_editor_new (void);

/**
 * Sets the track and refreshes the piano roll
 * widgets.
 *
 * To be called only from GTK threads.
 */
void
clip_editor_set_region (ClipEditor * self, Region * region, bool fire_events);

Region *
clip_editor_get_region (ClipEditor * self);

ArrangerSelections *
clip_editor_get_arranger_selections (ClipEditor * self);

#if 0
/**
 * Returns the Region that widgets are expected
 * to use.
 */
Region *
clip_editor_get_region_for_widgets (
  ClipEditor * self);
#endif

Track *
clip_editor_get_track (ClipEditor * self);

/**
 * To be called when recalculating the graph.
 */
void
clip_editor_set_caches (ClipEditor * self);

ClipEditor *
clip_editor_clone (const ClipEditor * src);

void
clip_editor_free (ClipEditor * self);

/**
 * @}
 */

#endif
