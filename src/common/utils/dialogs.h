// SPDX-FileCopyrightText: © 2018, 2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __UTILS_DIALOGS_H__
#define __UTILS_DIALOGS_H__

#include "gui/cpp/gtk_widgets/gtk_wrapper.h"

/**
 * Creates and returns the overwrite plugin dialog.
 */
AdwMessageDialog *
dialogs_get_overwrite_plugin_dialog (GtkWindow * parent);

AdwMessageDialog *
dialogs_get_basic_ok_message_dialog (GtkWindow * parent);

#endif
