/*
 * Copyright (C) 2019-2022 Alexandros Theodotou <alex at zrythm dot org>
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
#include "gui/backend/timeline_selections.h"
#include "gui/widgets/dialogs/export_midi_file_dialog.h"
#include "gui/widgets/main_window.h"
#include "utils/arrays.h"
#include "utils/flags.h"
#include "utils/gtk.h"
#include "utils/io.h"
#include "utils/localization.h"
#include "utils/resources.h"
#include "utils/string.h"
#include "utils/strv_builder.h"
#include "utils/ui.h"
#include "zrythm.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>

static void
on_response (GtkNativeDialog * native, int response)
{
  if (response == GTK_RESPONSE_ACCEPT)
    {
      GtkFileChooser * chooser =
        GTK_FILE_CHOOSER (native);
      GFile * gfile =
        gtk_file_chooser_get_file (chooser);
      char * filename = g_file_get_path (gfile);

      g_message ("exporting to %s", filename);

      int          midi_format = 0;
      const char * midi_format_str =
        gtk_file_chooser_get_choice (
          GTK_FILE_CHOOSER (native), "midi-format");
      if (string_is_equal (midi_format_str, "one"))
        {
          midi_format = 1;
        }

      bool         export_full = false;
      const char * region_content_str =
        gtk_file_chooser_get_choice (
          GTK_FILE_CHOOSER (native),
          "region-content");
      if (string_is_equal (
            region_content_str, "full"))
        {
          export_full = true;
        }

      bool lanes_as_separate_tracks = false;
      const char * lane_export_type_str =
        gtk_file_chooser_get_choice (
          GTK_FILE_CHOOSER (native),
          "lane-export-type");
      if (string_is_equal (
            lane_export_type_str, "separate-tracks"))
        {
          lanes_as_separate_tracks = true;
        }

      const TimelineSelections * sel =
        (const TimelineSelections *)
          g_object_get_data (
            G_OBJECT (native), "sel");
      if (sel->num_regions == 1)
        {
          midi_region_export_to_midi_file (
            sel->regions[0], filename, midi_format,
            export_full);

          ui_show_notification (
            _ ("MIDI region exported."));
        }
      else
        {
          bool ret =
            timeline_selections_export_to_midi_file (
              sel, filename, midi_format,
              export_full, lanes_as_separate_tracks);
          if (ret)
            {
              ui_show_notification (
                _ ("MIDI regions exported."));
            }
          else
            {
              ui_show_error_message (
                MAIN_WINDOW, false,
                _ ("Failed to export MIDI regions."));
            }
        }

      g_free (filename);

      g_object_unref (gfile);
    }

  g_object_unref (native);
}

/**
 * Runs the dialog asynchronously.
 *
 * @param region Region, if exporting region.
 */
void
export_midi_file_dialog_widget_run (
  GtkWindow *                parent,
  const TimelineSelections * sel)
{
  g_return_if_fail (
    timeline_selections_contains_only_region_types (
      sel, REGION_TYPE_MIDI));

  GtkFileChooserNative * fc_native =
    GTK_FILE_CHOOSER_NATIVE (
      gtk_file_chooser_native_new (
        _ ("Select MIDI file"), parent,
        GTK_FILE_CHOOSER_ACTION_SAVE,
        _ ("_Export"), _ ("_Cancel")));

  /* add region content choice */
  StrvBuilder * region_content_ids_builder =
    strv_builder_new ();
  StrvBuilder * region_content_labels_builder =
    strv_builder_new ();
  strv_builder_add (
    region_content_ids_builder, "base");
  strv_builder_add (
    region_content_labels_builder,
    _ ("Base region"));
  strv_builder_add (
    region_content_ids_builder, "full");
  strv_builder_add (
    region_content_labels_builder,
    _ ("Full region"));
  char ** region_content_ids =
    strv_builder_end (region_content_ids_builder);
  char ** region_content_labels = strv_builder_end (
    region_content_labels_builder);
  gtk_file_chooser_add_choice (
    GTK_FILE_CHOOSER (fc_native), "region-content",
    _ ("Region Content"),
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
    midi_format_labels_builder, _ ("Format 0"));
  strv_builder_add (midi_format_ids_builder, "one");
  strv_builder_add (
    midi_format_labels_builder, _ ("Format 1"));
  char ** midi_format_ids =
    strv_builder_end (midi_format_ids_builder);
  char ** midi_format_labels =
    strv_builder_end (midi_format_labels_builder);
  gtk_file_chooser_add_choice (
    GTK_FILE_CHOOSER (fc_native), "midi-format",
    _ ("MIDI Format"),
    (const char **) midi_format_ids,
    (const char **) midi_format_labels);
  g_strfreev (midi_format_ids);
  g_strfreev (midi_format_labels);

  /* add MIDI format choice */
  StrvBuilder * lane_export_type_ids_builder =
    strv_builder_new ();
  StrvBuilder * lane_export_type_labels_builder =
    strv_builder_new ();
  strv_builder_add (
    lane_export_type_ids_builder, "part-of-track");
  strv_builder_add (
    lane_export_type_labels_builder,
    _ ("Part of parent track"));
  strv_builder_add (
    lane_export_type_ids_builder,
    "separate-tracks");
  strv_builder_add (
    lane_export_type_labels_builder,
    _ ("Separate tracks"));
  char ** lane_export_type_ids = strv_builder_end (
    lane_export_type_ids_builder);
  char ** lane_export_type_labels =
    strv_builder_end (
      lane_export_type_labels_builder);
  gtk_file_chooser_add_choice (
    GTK_FILE_CHOOSER (fc_native),
    "lane-export-type", _ ("Export lanes as"),
    (const char **) lane_export_type_ids,
    (const char **) lane_export_type_labels);
  g_strfreev (lane_export_type_ids);
  g_strfreev (lane_export_type_labels);

  /* add MIDI file filter */
  GtkFileFilter * filter = gtk_file_filter_new ();
  gtk_file_filter_add_mime_type (
    filter, "audio/midi");
  gtk_file_filter_add_suffix (filter, "mid");
  gtk_file_filter_add_suffix (filter, "midi");
  gtk_file_chooser_add_filter (
    GTK_FILE_CHOOSER (fc_native), filter);

  ZRegion * r = sel->regions[0];
  char * tmp = g_strdup_printf ("%s.mid", r->name);
  char * file = string_convert_to_filename (tmp);
  g_free (tmp);
  gtk_file_chooser_set_current_name (
    GTK_FILE_CHOOSER (fc_native), file);
  g_free (file);

  g_signal_connect (
    fc_native, "response",
    G_CALLBACK (on_response), NULL);

  g_object_set_data (
    G_OBJECT (fc_native), "sel", (void *) sel);

  gtk_native_dialog_show (
    GTK_NATIVE_DIALOG (fc_native));
}
