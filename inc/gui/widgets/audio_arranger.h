/*
 * Copyright (C) 2020-2022 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __GUI_WIDGETS_AUDIO_ARRANGER_H__
#define __GUI_WIDGETS_AUDIO_ARRANGER_H__

#include "audio/position.h"
#include "gui/backend/tool.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/main_window.h"

#include <gtk/gtk.h>

/**
 * @addtogroup widgets
 *
 * @{
 */

#define MW_AUDIO_ARRANGER \
  MW_AUDIO_EDITOR_SPACE->arranger

void
audio_arranger_widget_snap_range_r (
  ArrangerWidget * self,
  Position *       pos);

/**
 * Returns whether the cursor is inside a fade
 * area.
 *
 * @param fade_in True to check for fade in, false
 *   to check for fade out.
 * @param resize True to check for whether resizing
 *   the fade (left <=> right), false to check
 *   for whether changing the fade curviness up/down.
 */
bool
audio_arranger_widget_is_cursor_in_fade (
  ArrangerWidget * self,
  double           x,
  double           y,
  bool             fade_in,
  bool             resize);

/**
 * Returns whether the cursor touches the gain line.
 */
bool
audio_arranger_widget_is_cursor_gain (
  ArrangerWidget * self,
  double           x,
  double           y);

UiOverlayAction
audio_arranger_widget_get_action_on_drag_begin (
  ArrangerWidget * self);

/**
 * Handle fade in/out curviness drag.
 */
void
audio_arranger_widget_fade_up (
  ArrangerWidget * self,
  double           offset_y,
  bool             fade_in);

void
audio_arranger_widget_update_gain (
  ArrangerWidget * self,
  double           offset_y);

/**
 * Updates the fade position during drag update.
 *
 * @param pos Absolute position in the editor.
 * @param fade_in Whether we are resizing the fade in
 *   or fade out position.
 * @parram dry_run Don't resize; just check
 *   if the resize is allowed.
 *
 * @return 0 if the operation was successful,
 *   nonzero otherwise.
 */
int
audio_arranger_widget_snap_fade (
  ArrangerWidget * self,
  Position *       pos,
  bool             fade_in,
  bool             dry_run);

/**
 * @}
 */

#endif
