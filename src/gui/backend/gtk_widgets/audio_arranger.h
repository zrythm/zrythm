// SPDX-FileCopyrightText: © 2020-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __GUI_WIDGETS_AUDIO_ARRANGER_H__
#define __GUI_WIDGETS_AUDIO_ARRANGER_H__

#include "common/dsp/position.h"
#include "gui/backend/backend/tool.h"
#include "gui/backend/gtk_widgets/arranger.h"
#include "gui/backend/gtk_widgets/gtk_wrapper.h"
#include "gui/backend/gtk_widgets/main_window.h"

/**
 * @addtogroup widgets
 *
 * @{
 */

#define MW_AUDIO_ARRANGER MW_AUDIO_EDITOR_SPACE->arranger

ArrangerCursor
audio_arranger_widget_get_cursor (ArrangerWidget * self, Tool tool);

void
audio_arranger_widget_snap_range_r (ArrangerWidget * self, Position * pos);

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
  const ArrangerWidget &self,
  double                x,
  double                y,
  bool                  fade_in,
  bool                  resize);

/**
 * Returns whether the cursor touches the gain line.
 */
bool
audio_arranger_widget_is_cursor_gain (
  const ArrangerWidget * self,
  double                 x,
  double                 y);

UiOverlayAction
audio_arranger_widget_get_action_on_drag_begin (ArrangerWidget * self);

/**
 * Handle fade in/out curviness drag.
 */
void
audio_arranger_widget_fade_up (
  ArrangerWidget * self,
  double           offset_y,
  bool             fade_in);

void
audio_arranger_widget_update_gain (ArrangerWidget * self, double offset_y);

/**
 * Updates the fade position during drag update.
 *
 * @param pos Absolute position in the editor.
 * @param fade_in Whether we are resizing the fade in
 *   or fade out position.
 * @parram dry_run Don't resize; just check
 *   if the resize is allowed.
 *
 * @return Whether successful.
 */
bool
audio_arranger_widget_snap_fade (
  ArrangerWidget * self,
  Position        &pos,
  bool             fade_in,
  bool             dry_run);

void
audio_arranger_on_drag_end (ArrangerWidget * self);

/**
 * @}
 */

#endif
