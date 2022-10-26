// SPDX-FileCopyrightText: © 2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Welcome message dialog.
 */

#ifndef __GUI_WIDGETS_DIALOGS_WELCOME_MESSAGE_DIALOG_H__
#define __GUI_WIDGETS_DIALOGS_WELCOME_MESSAGE_DIALOG_H__

#include <gtk/gtk.h>

/**
 * @addtogroup widgets
 *
 * @{
 */

/**
 * Creates and returns the welcome dialog.
 */
GtkDialog *
welcome_message_dialog_new (GtkWindow * parent);

/**
 * @}
 */

#endif /* __GUI_WIDGETS_DIALOGS_WELCOME_MESSAGE_DIALOG_H__ */
