// SPDX-FileCopyrightText: © 2020-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * @file
 *
 * Common editor settings.
 */

#ifndef __GUI_BACKEND_EDITOR_SETTINGS_H__
#define __GUI_BACKEND_EDITOR_SETTINGS_H__

#include <stdbool.h>

/**
 * @addtogroup gui_backend
 *
 * @{
 */

/**
 * Common editor settings.
 */
typedef struct EditorSettings
{
  /** Horizontal scroll start position. */
  int scroll_start_x;

  /** Vertical scroll start position. */
  int scroll_start_y;

  /** Horizontal zoom level. */
  double hzoom_level;
} EditorSettings;

void
editor_settings_init (EditorSettings * self);

void
editor_settings_set_scroll_start_x (EditorSettings * self, int x, bool validate);

void
editor_settings_set_scroll_start_y (EditorSettings * self, int y, bool validate);

/**
 * Appends the given deltas to the scroll x/y values.
 */
void
editor_settings_append_scroll (
  EditorSettings * self,
  int              dx,
  int              dy,
  bool             validate);

/**
 * @}
 */

#endif
