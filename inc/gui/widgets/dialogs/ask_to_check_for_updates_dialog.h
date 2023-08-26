// SPDX-FileCopyrightText: © 2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * @file
 */

#ifndef __GUI_WIDGETS_DIALOGS_ASK_CHECK_FOR_UPDATES_DIALOG_H__
#define __GUI_WIDGETS_DIALOGS_ASK_CHECK_FOR_UPDATES_DIALOG_H__

#include <adwaita.h>

/**
 * @addtogroup dialogs
 *
 * @{
 */

/**
 * Creates and displays the dialog asynchronously.
 */
void
ask_to_check_for_updates_dialog_run_async (GtkWindow * parent);

/**
 * @}
 */

#endif
