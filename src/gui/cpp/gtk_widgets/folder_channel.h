// SPDX-FileCopyrightText: © 2021-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * @file
 *
 * Folder channel widget.
 */

#ifndef __GUI_WIDGETS_FOLDER_CHANNEL_H__
#define __GUI_WIDGETS_FOLDER_CHANNEL_H__

#include "common/dsp/foldable_track.h"
#include "common/utils/types.h"
#include "gui/cpp/gtk_widgets/gtk_wrapper.h"

#define FOLDER_CHANNEL_WIDGET_TYPE (folder_channel_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  FolderChannelWidget,
  folder_channel_widget,
  Z,
  FOLDER_CHANNEL_WIDGET,
  GtkWidget)

class Track;
TYPEDEF_STRUCT_UNDERSCORED (ColorAreaWidget);
TYPEDEF_STRUCT_UNDERSCORED (FolderChannelSlotWidget);
TYPEDEF_STRUCT_UNDERSCORED (EditableLabelWidget);
TYPEDEF_STRUCT_UNDERSCORED (FaderButtonsWidget);

/**
 * @addtogroup widgets
 *
 * @{
 */

using FolderChannelWidget = struct _FolderChannelWidget
{
  GtkWidget         parent_instance;
  GtkGrid *         grid;
  ColorAreaWidget * color_top;
  ColorAreaWidget * color_left;
  GtkBox *          icon_and_name_event_box;
  GtkLabel *        name_lbl;

  GtkImage * icon;

  GtkToggleButton * fold_toggle;

  /** Used for highlighting. */
  GtkBox * highlight_left_box;
  GtkBox * highlight_right_box;

  /** Fader buttons. */
  FaderButtonsWidget * fader_buttons;

  /** Number of clicks, used when selecting/moving/
   * dragging channels. */
  int n_press;

  /** Control held down on drag begin. */
  bool ctrl_held_at_start;

  /** If drag update was called at least once. */
  bool dragged;

  /** The track selection processing was done in
   * the dnd callbacks, so no need to do it in
   * drag_end. */
  bool selected_in_dnd;

  /** Pointer to owner Track. */
  FoldableTrack * track;

  /** Whole folder_channel press. */
  GtkGestureClick * mp;

  GtkGestureClick * right_mouse_mp;

  /** Drag on the icon and name event box. */
  GtkGestureDrag * drag;

  gulong fold_toggled_handler_id;

  bool setup;

  /** Popover to be reused for context menus. */
  GtkPopoverMenu * popover_menu;
};

/**
 * Creates a folder_channel widget using the given folder_channel data.
 */
FolderChannelWidget *
folder_channel_widget_new (FoldableTrack * track);

void
folder_channel_widget_tear_down (FolderChannelWidget * self);

/**
 * Updates everything on the widget.
 *
 * It is redundant but keeps code organized. Should
 * fix if it causes lags.
 */
void
folder_channel_widget_refresh (FolderChannelWidget * self);

/**
 * Displays the widget.
 */
void
folder_channel_widget_show (FolderChannelWidget * self);

/**
 * @}
 */

#endif
