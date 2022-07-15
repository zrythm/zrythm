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
 * \file
 *
 * Dialog for routing track selections to a group.
 */

#ifndef __GUI_WIDGETS_ADD_TRACKS_TO_GROUP_DIALOG_H__
#define __GUI_WIDGETS_ADD_TRACKS_TO_GROUP_DIALOG_H__

#include "utils/types.h"

#include <gtk/gtk.h>

#define ADD_TRACKS_TO_GROUP_DIALOG_WIDGET_TYPE \
  (add_tracks_to_group_dialog_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  AddTracksToGroupDialogWidget,
  add_tracks_to_group_dialog_widget,
  Z,
  ADD_TRACKS_TO_GROUP_DIALOG_WIDGET,
  GtkDialog)

typedef struct TracklistSelections TracklistSelections;

/**
 * @addtogroup widgets
 *
 * @{
 */

/**
 * The add_tracks_to_group dialog.
 */
typedef struct _AddTracksToGroupDialogWidget
{
  GtkDialog parent_instance;
} AddTracksToGroupDialogWidget;

/**
 * Creates an add_tracks_to_group dialog widget and
 * displays it.
 *
 * @return The new group track, after a create tracks
 *   action has been executed, or NULL if failure.
 */
Track *
add_tracks_to_group_dialog_widget_get_track (
  TracklistSelections * sel);

/**
 * @}
 */

#endif
