// SPDX-FileCopyrightText: © 2019, 2021 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Timeline minimap.
 */

#ifndef __GUI_WIDGETS_TIMELINE_MINIMAP_H__
#define __GUI_WIDGETS_TIMELINE_MINIMAP_H__

#include "dsp/position.h"

#include <gtk/gtk.h>

#define TIMELINE_MINIMAP_WIDGET_TYPE (timeline_minimap_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  TimelineMinimapWidget,
  timeline_minimap_widget,
  Z,
  TIMELINE_MINIMAP_WIDGET,
  GtkWidget)

typedef struct _TimelineMinimapBgWidget        TimelineMinimapBgWidget;
typedef struct _TimelineMinimapSelectionWidget TimelineMinimapSelectionWidget;
typedef struct TimelineMinimap                 TimelineMinimap;

/**
 * @addtogroup widgets
 *
 * @{
 */

#define MW_TIMELINE_MINIMAP MW_TIMELINE_BOT_BOX->timeline_minimap

typedef enum TimelineMinimapAction
{
  TIMELINE_MINIMAP_ACTION_NONE,
  TIMELINE_MINIMAP_ACTION_RESIZING_L,
  TIMELINE_MINIMAP_ACTION_RESIZING_R,
  TIMELINE_MINIMAP_ACTION_STARTING_MOVING, ///< in drag_start
  TIMELINE_MINIMAP_ACTION_MOVING,          ///< in drag start,
                                  ///< also for dragging up/down bitwig style
} TimelineMinimapAction;

typedef struct _TimelineMinimapWidget
{
  GtkWidget parent_instance;

  GtkOverlay * overlay;

  TimelineMinimapBgWidget *        bg;
  TimelineMinimapSelectionWidget * selection;
  TimelineMinimapAction            action;
  // GtkGestureDrag *    drag;
  // GtkGestureClick *   multipress;
  // GtkGestureClick *   right_mouse_mp;

  /** Last drag offsets during a drag. */
  double last_offset_x;
  double last_offset_y;

  /** Coordinates at the start of a drag action. */
  double start_x;
  double start_y;

  /** To be set in drag_begin(). */
  double start_zoom_level;
  double selection_start_pos;
  double selection_end_pos;

  /** Number of presses, for click controller. */
  int n_press;
} TimelineMinimapWidget;

/**
 * Taken from arranger.c
 */
void
timeline_minimap_widget_px_to_pos (
  TimelineMinimapWidget * self,
  Position *              pos,
  int                     px);

/**
 * Causes reallocation.
 */
void
timeline_minimap_widget_refresh (TimelineMinimapWidget * self);

/**
 * @}
 */

#endif
