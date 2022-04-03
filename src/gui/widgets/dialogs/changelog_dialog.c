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
  GtkWidget * dialog =
    gtk_message_dialog_new_with_markup (
      parent, GTK_DIALOG_DESTROY_WITH_PARENT,
      GTK_MESSAGE_INFO, GTK_BUTTONS_CLOSE,
      _ ("Running %s version <b>%s</b>%s%s"),
      PROGRAM_NAME, PACKAGE_VERSION, "\n\n",
      CHANGELOG_TXT);

  z_gtk_dialog_run (GTK_DIALOG (dialog), true);
}

#endif /* HAVE_CHANGELOG */
