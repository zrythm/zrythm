// SPDX-FileCopyrightText: © 2018, 2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __UTILS_DIALOGS_H__
#define __UTILS_DIALOGS_H__

#include <gtk/gtk.h>

/**
 * Creates and returns the overwrite plugin dialog.
 */
GtkDialog *
dialogs_get_overwrite_plugin_dialog (GtkWindow * parent);

GtkDialog *
dialogs_get_error_instantiating_plugin_dialog (GtkWindow * parent);

#endif
