/*
 * Copyright (C) 2018-2019 Alexandros Theodotou
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * \file
 *
 * Export dialog.
 */

#include "audio/engine.h"
#include "audio/exporter.h"
#include "gui/widgets/export_dialog.h"
#include "project.h"
#include "utils/io.h"
#include "utils/resources.h"

#include <gtk/gtk.h>

G_DEFINE_TYPE (ExportDialogWidget, export_dialog_widget, GTK_TYPE_DIALOG)

static void
on_arrangement_clicked (ExportDialogWidget * self,
                        gpointer user_data)
{
}

static void
on_loop_clicked (ExportDialogWidget * self,
                 gpointer user_data)
{
}

static void
on_cancel_clicked (GtkButton * btn,
                   ExportDialogWidget * self)
{
  gtk_widget_destroy (GTK_WIDGET (self));
}

static void
on_export_clicked (GtkButton * btn,
                   ExportDialogWidget * self)
{
  ExportSettings info;
  info.format = AUDIO_FORMAT_FLAC;
  info.depth = BIT_DEPTH_24;
  g_message ("projects dir %s, project file %s",
             PROJECT->dir,
             PROJECT->project_file_path);
  info.file_uri =
    g_build_filename (PROJECT->dir,
                      "exports",
                      "Master.FLAC",
                      NULL);
  g_message ("exporting");
  AUDIO_ENGINE->run = 0;
  exporter_export (&info);
  AUDIO_ENGINE->run = 1;
  g_message ("exported");
  g_free (info.file_uri);
}

/**
 * Creates a new export dialog.
 */
ExportDialogWidget *
export_dialog_widget_new ()
{
  ExportDialogWidget * self =
    g_object_new (EXPORT_DIALOG_WIDGET_TYPE, NULL);

  g_signal_connect (
    G_OBJECT (self->export_button), "clicked",
    G_CALLBACK (on_export_clicked), self);

  return self;
}

static void
export_dialog_widget_class_init (ExportDialogWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (klass,
                                "export_dialog.ui");

  gtk_widget_class_bind_template_child (
    klass,
    ExportDialogWidget,
    cancel_button);
  gtk_widget_class_bind_template_child (
    klass,
    ExportDialogWidget,
    export_button);
  gtk_widget_class_bind_template_child (
    klass,
    ExportDialogWidget,
    export_artist);
  gtk_widget_class_bind_template_child (
    klass,
    ExportDialogWidget,
    export_genre);
  gtk_widget_class_bind_template_child (
    klass,
    ExportDialogWidget,
    filename_pattern);
  gtk_widget_class_bind_template_child (
    klass,
    ExportDialogWidget,
    tracks);
  gtk_widget_class_bind_template_child (
    klass,
    ExportDialogWidget,
    time_range_song);
  gtk_widget_class_bind_template_child (
    klass,
    ExportDialogWidget,
    time_range_loop);
  gtk_widget_class_bind_template_child (
    klass,
    ExportDialogWidget,
    time_range_custom);
  gtk_widget_class_bind_template_child (
    klass,
    ExportDialogWidget,
    format);
  gtk_widget_class_bind_template_child (
    klass,
    ExportDialogWidget,
    dither);
  gtk_widget_class_bind_template_child (
    klass,
    ExportDialogWidget,
    output_label);
  gtk_widget_class_bind_template_callback (
    klass,
    on_arrangement_clicked);
  gtk_widget_class_bind_template_callback (
    klass,
    on_loop_clicked);
  gtk_widget_class_bind_template_callback (
    klass,
    on_cancel_clicked);
}

static void
export_dialog_widget_init (ExportDialogWidget * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  gtk_entry_set_text (
    GTK_ENTRY (self->export_artist),
    g_settings_get_string (
      S_PREFERENCES,
      "export-artist"));
  gtk_entry_set_text (
    GTK_ENTRY (self->export_genre),
    g_settings_get_string (
      S_PREFERENCES,
      "export-genre"));

  gtk_toggle_button_set_active (
    self->time_range_song,
    0);
  gtk_toggle_button_set_active (
    self->time_range_loop,
    0);
  gtk_toggle_button_set_active (
    self->time_range_custom,
    0);
  switch (g_settings_get_enum (
            S_PREFERENCES,
            "export-time-range"))
    {
    case 0: // song
      gtk_toggle_button_set_active (
        self->time_range_song,
        1);
      break;
    case 1: // loop
      gtk_toggle_button_set_active (
        self->time_range_loop,
        1);
      break;
    case 2: // custom
      gtk_toggle_button_set_active (
        self->time_range_custom,
        1);
      break;
    }
}
