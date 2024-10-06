// SPDX-FileCopyrightText: © 2019, 2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * @file
 *
 * About dialog.
 */

#ifndef __GUI_WIDGETS_ABOUT_DIALOG_H__
#define __GUI_WIDGETS_ABOUT_DIALOG_H__

#include "gui/backend/gtk_widgets/gtk_wrapper.h"

/**
 * @addtogroup widgets
 *
 * @{
 */

/**
 * Creates and displays the about dialog.
 */
GtkWindow *
about_dialog_widget_new (GtkWindow * parent);

/**
 * @}
 */

#endif
