// SPDX-FileCopyrightText: © 2020, 2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Timeline backend.
 */

#ifndef __GUI_BACKEND_TIMELINE_H__
#define __GUI_BACKEND_TIMELINE_H__

#include "gui/backend/editor_settings.h"

/**
 * @addtogroup gui_backend
 *
 * @{
 */

#define PRJ_TIMELINE (PROJECT->timeline)

/**
 * Clip editor serializable backend.
 *
 * The actual widgets should reflect the information here.
 */
struct Timeline
{
  /** Settings for the timeline. */
  EditorSettings editor_settings;

  /** Width of the left side of the timeline panel. */
  int tracks_width;
};

/**
 * Inits the Timeline after a Project is loaded.
 */
void
timeline_init_loaded (Timeline * self);

/**
 * Inits the Timeline instance.
 */
void
timeline_init (Timeline * self);

Timeline *
timeline_clone (Timeline * src);

/**
 * Creates a new Timeline instance.
 */
Timeline *
timeline_new (void);

void
timeline_free (Timeline * self);

/**
 * @}
 */

#endif
