// clang-format off
// SPDX-FileCopyrightText: © 2019, 2021, 2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense
// clang-format on

/**
 * \file
 *
 * Arranger minimap.
 */

#ifndef __GUI_WIDGETS_ARRANGER_MINIMAP_H__
#define __GUI_WIDGETS_ARRANGER_MINIMAP_H__

#include "dsp/position.h"

#include <gtk/gtk.h>

#define ARRANGER_MINIMAP_WIDGET_TYPE (arranger_minimap_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  ArrangerMinimapWidget,
  arranger_minimap_widget,
  Z,
  ARRANGER_MINIMAP_WIDGET,
  GtkWidget)

typedef struct _ArrangerMinimapBgWidget        ArrangerMinimapBgWidget;
typedef struct _ArrangerMinimapSelectionWidget ArrangerMinimapSelectionWidget;
typedef struct ArrangerMinimap                 ArrangerMinimap;

/**
 * @addtogroup widgets
 *
 * @{
 */

#define MW_TIMELINE_MINIMAP (MW_TIMELINE_PANEL->timeline_wrapper->minimap)

typedef enum ArrangerMinimapAction
{
  ARRANGER_MINIMAP_ACTION_NONE,
  ARRANGER_MINIMAP_ACTION_RESIZING_L,
  ARRANGER_MINIMAP_ACTION_RESIZING_R,
  ARRANGER_MINIMAP_ACTION_STARTING_MOVING, ///< in drag_start
  ARRANGER_MINIMAP_ACTION_MOVING,          ///< in drag start,
                                  ///< also for dragging up/down bitwig style
} ArrangerMinimapAction;

typedef enum ArrangerMinimapType
{
  ARRANGER_MINIMAP_TYPE_TIMELINE,
  ARRANGER_MINIMAP_TYPE_CLIP_EDITOR,
} ArrangerMinimapType;

typedef struct _ArrangerMinimapWidget
{
  GtkWidget parent_instance;

  GtkOverlay * overlay;

  ArrangerMinimapType type;

  ArrangerMinimapBgWidget *        bg;
  ArrangerMinimapSelectionWidget * selection;
  ArrangerMinimapAction            action;

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
} ArrangerMinimapWidget;

/**
 * Taken from arranger.c
 */
void
arranger_minimap_widget_px_to_pos (
  ArrangerMinimapWidget * self,
  Position *              pos,
  int                     px);

/**
 * Causes reallocation.
 */
void
arranger_minimap_widget_refresh (ArrangerMinimapWidget * self);

/**
 * @}
 */

#endif
