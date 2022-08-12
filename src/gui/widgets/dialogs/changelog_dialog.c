// SPDX-FileCopyrightText: © 2021 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-config.h"

/* auto-generated */
#include "gui/widgets/dialogs/changelog_dialog.h"
#include "utils/gtk.h"
#include "utils/resources.h"
#include "zrythm.h"

#include <glib/gi18n.h>

#include "src/translators.h"

#ifdef HAVE_CHANGELOG

/**
 * Creates and displays the about dialog.
 */
void
changelog_dialog_widget_run (GtkWindow * parent)
{
  GtkWidget * dialog = gtk_message_dialog_new_with_markup (
    parent, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_INFO,
    GTK_BUTTONS_CLOSE, _ ("Running %s version <b>%s</b>%s%s"),
    PROGRAM_NAME, PACKAGE_VERSION, "\n\n", CHANGELOG_TXT);

  z_gtk_dialog_run (GTK_DIALOG (dialog), true);
}

#endif /* HAVE_CHANGELOG */
