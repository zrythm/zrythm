/*
 * Copyright (C) 2019-2021 Alexandros Theodotou <alex at zrythm dot org>
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

/**
 * \file
 *
 */

#include "audio/engine.h"
#include "audio/engine_alsa.h"
#include "audio/engine_jack.h"
#include "audio/engine_pa.h"
#include "gui/widgets/dialogs/export_midi_file_dialog.h"
#include "gui/widgets/active_hardware_mb.h"
#include "utils/arrays.h"
#include "utils/flags.h"
#include "utils/gtk.h"
#include "utils/localization.h"
#include "utils/io.h"
#include "utils/resources.h"
#include "utils/string.h"
#include "utils/strv_builder.h"
#include "utils/ui.h"
#include "zrythm.h"

#include <glib/gi18n.h>

/**
 * Runs a new dialog and returns the filepath if
 * selected or NULL if canceled.
 */
char *
export_midi_file_dialog_widget_run_for_region (
  GtkWindow * parent,
  ZRegion *   region)
{
  GtkFileChooserDialog * dialog =
    GTK_FILE_CHOOSER_DIALOG (
      gtk_file_chooser_dialog_new (
        _("Select MIDI file"), parent,
        GTK_FILE_CHOOSER_ACTION_SAVE,
        _("_Cancel"), GTK_RESPONSE_CANCEL,
        _("_Export"), GTK_RESPONSE_ACCEPT,
        NULL));

  /* add region content choice */
  StrvBuilder * region_content_ids_builder =
    strv_builder_new ();
  StrvBuilder * region_content_labels_builder =
    strv_builder_new ();
  strv_builder_add (
    region_content_ids_builder, "base");
  strv_builder_add (
    region_content_labels_builder, _("Base region"));
  strv_builder_add (
    region_content_ids_builder, "full");
  strv_builder_add (
    region_content_labels_builder, _("Full region"));
  char ** region_content_ids =
    strv_builder_end (region_content_ids_builder);
  char ** region_content_labels =
    strv_builder_end (region_content_labels_builder);
  gtk_file_chooser_add_choice (
    GTK_FILE_CHOOSER (dialog), "region-content",
    _("Region Content"),
    (const char **) region_content_ids,
    (const char **) region_content_labels);
  g_strfreev (region_content_ids);
  g_strfreev (region_content_labels);

  /* add MIDI format choice */
  StrvBuilder * midi_format_ids_builder =
    strv_builder_new ();
  StrvBuilder * midi_format_labels_builder =
    strv_builder_new ();
  strv_builder_add (
    midi_format_ids_builder, "zero");
  strv_builder_add (
    midi_format_labels_builder, _("Format 0"));
  strv_builder_add (
    midi_format_ids_builder, "one");
  strv_builder_add (
    midi_format_labels_builder, _("Format 1"));
  char ** midi_format_ids =
    strv_builder_end (midi_format_ids_builder);
  char ** midi_format_labels =
    strv_builder_end (midi_format_labels_builder);
  gtk_file_chooser_add_choice (
    GTK_FILE_CHOOSER (dialog), "midi-format",
    _("MIDI Format"),
    (const char **) midi_format_ids,
    (const char **) midi_format_labels);
  g_strfreev (midi_format_ids);
  g_strfreev (midi_format_labels);

  /* add MIDI file filter */
  GtkFileFilter * filter = gtk_file_filter_new ();
  gtk_file_filter_add_mime_type (
    filter, "audio/midi");
  gtk_file_filter_add_suffix (
    filter, "mid");
  gtk_file_filter_add_suffix (
    filter, "midi");
  gtk_file_chooser_add_filter (
    GTK_FILE_CHOOSER (dialog), filter);

  char * tmp =
    g_strdup_printf (
      "%s.mid", region->name);
  char * file =
    string_convert_to_filename (tmp);
  g_free (tmp);
  gtk_file_chooser_set_current_name (
    GTK_FILE_CHOOSER (dialog), file);
  g_free (file);

  gtk_window_set_transient_for (
    GTK_WINDOW (dialog), parent);

  int res =
    z_gtk_dialog_run (GTK_DIALOG (dialog), false);
  char * filename  = NULL;
  switch (res)
    {
    case GTK_RESPONSE_ACCEPT:
      {
        GFile * gfile = NULL;
        gfile =
          gtk_file_chooser_get_file (
            GTK_FILE_CHOOSER (dialog));
        filename = g_file_get_path (gfile);
        g_object_unref (gfile);
      }
      break;
    default:
      break;
    }
  gtk_window_destroy (GTK_WINDOW (dialog));

  return filename;
}
