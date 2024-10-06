// SPDX-FileCopyrightText: © 2020, 2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * @file
 *
 * Dialog for viewing/editing port info.
 */

#ifndef __GUI_WIDGETS_DIALOGS_ARRANGER_OBJECT_INFO_H__
#define __GUI_WIDGETS_DIALOGS_ARRANGER_OBJECT_INFO_H__

#include "gui/backend/gtk_widgets/gtk_wrapper.h"

#define ARRANGER_OBJECT_INFO_DIALOG_WIDGET_TYPE \
  (arranger_object_info_dialog_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  ArrangerObjectInfoDialogWidget,
  arranger_object_info_dialog_widget,
  Z,
  ARRANGER_OBJECT_INFO_DIALOG_WIDGET,
  GtkWindow)

class ArrangerObject;

/**
 * @addtogroup widgets
 *
 * @{
 */

/**
 * The arranger_object_info dialog.
 */
typedef struct _ArrangerObjectInfoDialogWidget
{
  GtkWindow parent_instance;

  ArrangerObject * obj;
} ArrangerObjectInfoDialogWidget;

/**
 * Creates an arranger_object_info dialog widget and
 * displays it.
 */
ArrangerObjectInfoDialogWidget *
arranger_object_info_dialog_widget_new (ArrangerObject * object);

/**
 * @}
 */

#endif
