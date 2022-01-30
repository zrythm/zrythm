/*
 * Copyright (C) 2021 Alexandros Theodotou <alex at zrythm dot org>
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
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * @file
 *
 * Folder channel widget.
 */

#ifndef __GUI_WIDGETS_FOLDER_CHANNEL_H__
#define __GUI_WIDGETS_FOLDER_CHANNEL_H__

#include <gtk/gtk.h>

#define FOLDER_CHANNEL_WIDGET_TYPE \
  (folder_channel_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  FolderChannelWidget, folder_channel_widget,
  Z, FOLDER_CHANNEL_WIDGET,
  GtkBox)

typedef struct _ColorAreaWidget ColorAreaWidget;
typedef struct Track Track;
typedef struct _FolderChannelSlotWidget FolderChannelSlotWidget;
typedef struct _EditableLabelWidget
  EditableLabelWidget;
typedef struct _RotatedLabelWidget
  RotatedLabelWidget;
typedef struct _FaderButtonsWidget
  FaderButtonsWidget;

/**
 * @addtogroup widgets
 *
 * @{
 */

typedef struct _FolderChannelWidget
{
  GtkBox         parent_instance;
  GtkGrid *           grid;
  ColorAreaWidget *   color_top;
  ColorAreaWidget *   color_left;
  GtkBox *       icon_and_name_event_box;
  RotatedLabelWidget * name;

  GtkImage *          icon;

  GtkToggleButton *   fold_toggle;

  /** Used for highlighting. */
  GtkBox *            highlight_left_box;
  GtkBox *            highlight_right_box;

  /** Fader buttons. */
  FaderButtonsWidget *  fader_buttons;

  /** Number of clicks, used when selecting/moving/
   * dragging channels. */
  int                 n_press;

  /** Control held down on drag begin. */
  int                 ctrl_held_at_start;

  /** If drag update was called at least once. */
  int                 dragged;

  /** The track selection processing was done in
   * the dnd callbacks, so no need to do it in
   * drag_end. */
  int                 selected_in_dnd;

  /** Pointer to owner Track. */
  Track *             track;

  /** Whole folder_channel press. */
  GtkGestureClick * mp;

  GtkGestureClick * right_mouse_mp;

  /** Drag on the icon and name event box. */
  GtkGestureDrag       * drag;

  gulong              fold_toggled_handler_id;

  bool                   setup;

  /** Popover to be reused for context menus. */
  GtkPopoverMenu * popover_menu;
} FolderChannelWidget;

/**
 * Creates a folder_channel widget using the given folder_channel data.
 */
FolderChannelWidget *
folder_channel_widget_new (
  Track * track);

void
folder_channel_widget_tear_down (
  FolderChannelWidget * self);

/**
 * Updates everything on the widget.
 *
 * It is redundant but keeps code organized. Should
 * fix if it causes lags.
 */
void
folder_channel_widget_refresh (
  FolderChannelWidget * self);

/**
 * Displays the widget.
 */
void
folder_channel_widget_show (
  FolderChannelWidget * self);

/**
 * @}
 */

#endif
