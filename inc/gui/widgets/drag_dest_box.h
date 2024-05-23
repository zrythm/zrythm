// clang-format off
// SPDX-FileCopyrightText: © 2018-2019, 2021 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense
// clang-format on

/**
 * @file
 *
 * Drag dest box.
 */

#ifndef __GUI_WIDGETS_DRAG_DEST_BOX_H__
#define __GUI_WIDGETS_DRAG_DEST_BOX_H__

#include <gtk/gtk.h>

typedef struct Channel Channel;

/**
 * @addtogroup widgets
 *
 * @{
 */

#define DRAG_DEST_BOX_WIDGET_TYPE (drag_dest_box_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  DragDestBoxWidget,
  drag_dest_box_widget,
  Z,
  DRAG_DEST_BOX_WIDGET,
  GtkBox)
#define TRACKLIST_DRAG_DEST_BOX MW_TRACKLIST->ddbox
#define MIXER_DRAG_DEST_BOX MW_MIXER->ddbox

enum class DragDestBoxType
{
  DRAG_DEST_BOX_TYPE_MIXER,
  DRAG_DEST_BOX_TYPE_TRACKLIST,
  DRAG_DEST_BOX_TYPE_MODULATORS,
};

/**
 * DnD destination box used by mixer and tracklist
 * widgets.
 */
typedef struct _DragDestBoxWidget
{
  GtkBox            parent_instance;
  GtkGestureDrag *  drag;
  GtkGestureClick * click;
  GtkGestureClick * right_click;
  DragDestBoxType   type;

  /** Popover to be reused for context menus. */
  GtkPopoverMenu * popover_menu;
} DragDestBoxWidget;

/**
 * Creates a drag destination box widget.
 */
DragDestBoxWidget *
drag_dest_box_widget_new (
  GtkOrientation  orientation,
  int             spacing,
  DragDestBoxType type);

/**
 * @}
 */

#endif
