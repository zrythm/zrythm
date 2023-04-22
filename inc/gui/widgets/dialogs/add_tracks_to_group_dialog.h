// SPDX-FileCopyrightText: © 2021 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

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
 * Creates an add_tracks_to_group dialog widget and displays it.
 *
 * @return The new group track, after a create tracks action
 *   has been executed, or NULL if failure.
 */
Track *
add_tracks_to_group_dialog_widget_get_track (
  TracklistSelections * sel);

/**
 * @}
 */

#endif
