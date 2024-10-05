// SPDX-FileCopyrightText: © 2021, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __GUI_WIDGETS_ADD_TRACKS_TO_GROUP_DIALOG_H__
#define __GUI_WIDGETS_ADD_TRACKS_TO_GROUP_DIALOG_H__

#include "common/utils/types.h"
#include "gui/cpp/gtk_widgets/gtk_wrapper.h"

#define ADD_TRACKS_TO_GROUP_DIALOG_WIDGET_TYPE \
  (add_tracks_to_group_dialog_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  AddTracksToGroupDialogWidget,
  add_tracks_to_group_dialog_widget,
  Z,
  ADD_TRACKS_TO_GROUP_DIALOG_WIDGET,
  GtkDialog)

class TracklistSelections;
class GroupTargetTrack;

/**
 * @addtogroup widgets
 *
 * @{
 */

/**
 * Dialog for routing track selections to a group.
 */
using AddTracksToGroupDialogWidget = struct _AddTracksToGroupDialogWidget
{
  GtkDialog parent_instance;
};

/**
 * Creates an add_tracks_to_group dialog widget and displays it.
 *
 * @return The new group track, after a create tracks action
 *   has been executed, or NULL if failure.
 */
GroupTargetTrack *
add_tracks_to_group_dialog_widget_get_track (TracklistSelections * sel);

/**
 * @}
 */

#endif
