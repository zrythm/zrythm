/*
 * Copyright (C) 2018-2019 Alexandros Theodotou <alex at zrythm dot org>
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

enum
{
  COLUMN_AUDIO_FORMAT_LABEL,
  COLUMN_AUDIO_FORMAT,
  NUM_AUDIO_FORMAT_COLUMNS
};
enum
{
  FILENAME_PATTERN_MIXDOWN_FORMAT,
  FILENAME_PATTERN_DATE_MIXDOWN_FORMAT,
  NUM_FILENAME_PATTERNS,
};

static char *
get_export_filename (ExportDialogWidget * self)
{
  char * format =
    exporter_stringize_audio_format (
      gtk_combo_box_get_active (
        self->format));
  char * str =
    g_strdup_printf ("%s.%s",
                     "mixdown", // TODO replace
                     format);
  g_free (format);

  return str;
}

static void
update_text (ExportDialogWidget * self)
{
  char * filename =
    get_export_filename (self);
  char * str =
    g_strdup_printf (
      "The following files will be created:\n\n"
      "%s\n\n"
      "in the directory:\n\n"
      "%s",
      filename,
      PROJECT->exports_dir);
  gtk_label_set_text (
    self->output_label,
    str);
  g_free (filename);
  g_free (str);
}

static void
on_song_toggled (GtkToggleButton * toggle,
                 ExportDialogWidget * self)
{
  if (gtk_toggle_button_get_active (toggle))
    {
      gtk_toggle_button_set_active (
        self->time_range_loop, 0);
      gtk_toggle_button_set_active (
        self->time_range_custom, 0);
    }
}

static void
on_loop_toggled (GtkToggleButton * toggle,
                 ExportDialogWidget * self)
{
  if (gtk_toggle_button_get_active (toggle))
    {
      gtk_toggle_button_set_active (
        self->time_range_song, 0);
      gtk_toggle_button_set_active (
        self->time_range_custom, 0);
    }
}

static void
on_custom_toggled (GtkToggleButton * toggle,
                   ExportDialogWidget * self)
{
  if (gtk_toggle_button_get_active (toggle))
    {
      gtk_toggle_button_set_active (
        self->time_range_song, 0);
      gtk_toggle_button_set_active (
        self->time_range_loop, 0);
    }
}

/**
 * Creates the combo box model for bit depth.
 */
static GtkTreeModel *
create_bit_depth_store ()
{
  GtkTreeIter iter;
  GtkTreeStore *store;

  store =
    gtk_tree_store_new (1,
                        G_TYPE_STRING);

  gtk_tree_store_append (store, &iter, NULL);
  gtk_tree_store_set (
    store, &iter,
    0, "16 bit",
    -1);
  gtk_tree_store_append (store, &iter, NULL);
  gtk_tree_store_set (
    store, &iter,
    0, "24 bit",
    -1);
  gtk_tree_store_append (store, &iter, NULL);
  gtk_tree_store_set (
    store, &iter,
    0, "32 bit",
    -1);

  return GTK_TREE_MODEL (store);
}

static void
setup_bit_depth_combo_box (
  ExportDialogWidget * self)
{
  GtkTreeModel * model =
    create_bit_depth_store ();
  gtk_combo_box_set_model (
    self->bit_depth,
    model);
  gtk_cell_layout_clear (
    GTK_CELL_LAYOUT (self->bit_depth));
  GtkCellRenderer* renderer =
    gtk_cell_renderer_text_new ();
  gtk_cell_layout_pack_start (
    GTK_CELL_LAYOUT (self->bit_depth),
    renderer,
    TRUE);
  gtk_cell_layout_set_attributes (
    GTK_CELL_LAYOUT (self->bit_depth),
    renderer,
    "text", 0,
    NULL);

  gtk_combo_box_set_active (
    self->bit_depth,
    0);
}

/**
 * Creates the combo box model for the pattern.
 */
static GtkTreeModel *
create_filename_pattern_store ()
{
  GtkTreeIter iter;
  GtkTreeStore *store;

  store =
    gtk_tree_store_new (1,
                        G_TYPE_STRING);

  gtk_tree_store_append (store, &iter, NULL);
  gtk_tree_store_set (
    store, &iter,
    0, "mixdown.<format>",
    -1);
  gtk_tree_store_append (store, &iter, NULL);
  gtk_tree_store_set (
    store, &iter,
    0, "<date>_mixdown.<format>",
    -1);

  return GTK_TREE_MODEL (store);
}

static void
setup_filename_pattern_combo_box (
  ExportDialogWidget * self)
{
  GtkTreeModel * model =
    create_filename_pattern_store ();
  gtk_combo_box_set_model (
    self->filename_pattern,
    model);
  gtk_cell_layout_clear (
    GTK_CELL_LAYOUT (self->filename_pattern));
  GtkCellRenderer* renderer =
    gtk_cell_renderer_text_new ();
  gtk_cell_layout_pack_start (
    GTK_CELL_LAYOUT (self->filename_pattern),
    renderer,
    TRUE);
  gtk_cell_layout_set_attributes (
    GTK_CELL_LAYOUT (self->filename_pattern),
    renderer,
    "text", 0,
    NULL);

  gtk_combo_box_set_active (
    self->filename_pattern,
    0);
}

/**
 * Creates the combo box model for the audio
 * formats.
 */
static GtkTreeModel *
create_formats_store ()
{
  GtkTreeIter iter;
  GtkTreeStore *store;

  store =
    gtk_tree_store_new (NUM_AUDIO_FORMAT_COLUMNS,
                        G_TYPE_STRING,
                        G_TYPE_INT);

  gtk_tree_store_append (store, &iter, NULL);
  gtk_tree_store_set (
    store, &iter,
    COLUMN_AUDIO_FORMAT_LABEL, "FLAC",
    COLUMN_AUDIO_FORMAT, AUDIO_FORMAT_FLAC,
    -1);
  gtk_tree_store_append (store, &iter, NULL);
  gtk_tree_store_set (
    store, &iter,
    COLUMN_AUDIO_FORMAT_LABEL, "OGG",
    COLUMN_AUDIO_FORMAT, AUDIO_FORMAT_OGG,
    -1);
  gtk_tree_store_append (store, &iter, NULL);
  gtk_tree_store_set (
    store, &iter,
    COLUMN_AUDIO_FORMAT_LABEL, "WAV",
    COLUMN_AUDIO_FORMAT, AUDIO_FORMAT_WAV,
    -1);
  gtk_tree_store_append (store, &iter, NULL);
  gtk_tree_store_set (
    store, &iter,
    COLUMN_AUDIO_FORMAT_LABEL, "MP3",
    COLUMN_AUDIO_FORMAT, AUDIO_FORMAT_MP3,
    -1);

  return GTK_TREE_MODEL (store);
}

static void
on_format_changed (GtkComboBox *widget,
               ExportDialogWidget * self)
{
  update_text (self);
  if (gtk_combo_box_get_active (widget) ==
        AUDIO_FORMAT_OGG)
    {
      gtk_widget_set_sensitive (
        GTK_WIDGET (self->bit_depth), 0);
    }
  else
    {
      gtk_widget_set_sensitive (
        GTK_WIDGET (self->bit_depth), 1);
    }
}

static void
setup_formats_combo_box (
  ExportDialogWidget * self)
{
  GtkTreeModel * model =
    create_formats_store ();
  gtk_combo_box_set_model (
    self->format,
    model);
  gtk_cell_layout_clear (
    GTK_CELL_LAYOUT (self->format));
  GtkCellRenderer* renderer =
    gtk_cell_renderer_text_new ();
  gtk_cell_layout_pack_start (
    GTK_CELL_LAYOUT (self->format),
    renderer,
    TRUE);
  gtk_cell_layout_set_attributes (
    GTK_CELL_LAYOUT (self->format),
    renderer,
    "text", COLUMN_AUDIO_FORMAT_LABEL,
    NULL);

  gtk_combo_box_set_active (
    self->format,
    g_settings_get_enum (
      S_PREFERENCES,
      "export-format"));
}

static void
on_cancel_clicked (GtkButton * btn,
                   ExportDialogWidget * self)
{
  gtk_window_close (GTK_WINDOW (self));
  /*gtk_widget_destroy (GTK_WIDGET (self));*/
}

static void
on_export_clicked (GtkButton * btn,
                   ExportDialogWidget * self)
{
  ExportSettings info;
  info.format =
    gtk_combo_box_get_active (self->format);
  g_settings_set_enum (
    S_PREFERENCES,
    "export-format",
    info.format);
  info.depth =
    gtk_combo_box_get_active (self->bit_depth);
  g_settings_set_enum (
    S_PREFERENCES,
    "export-bit-depth",
    info.depth);
  info.dither =
    gtk_toggle_button_get_active (
      GTK_TOGGLE_BUTTON (self->dither));
  g_settings_set_int (
    S_PREFERENCES,
    "export-dither",
    info.dither);
  info.artist =
    gtk_entry_get_text (self->export_artist);
  info.genre =
    gtk_entry_get_text (self->export_genre);
  g_settings_set_string (
    S_PREFERENCES,
    "export-artist",
    info.artist);
  g_settings_set_string (
    S_PREFERENCES,
    "export-genre",
    info.genre);
  if (gtk_toggle_button_get_active (
        self->time_range_song))
    {
      g_settings_set_enum (
        S_PREFERENCES,
        "export-time-range",
        0);
      info.time_range = TIME_RANGE_SONG;
    }
  else if (gtk_toggle_button_get_active (
             self->time_range_loop))
    {
      g_settings_set_enum (
        S_PREFERENCES,
        "export-time-range",
        1);
      info.time_range = TIME_RANGE_LOOP;
    }
  else if (gtk_toggle_button_get_active (
             self->time_range_custom))
    {
      g_settings_set_enum (
        S_PREFERENCES,
        "export-time-range",
        2);
      info.time_range = TIME_RANGE_CUSTOM;
    }

  g_message ("projects dir %s, project file %s",
             PROJECT->dir,
             PROJECT->project_file_path);
  char * filename =
    get_export_filename (self);
  info.file_uri =
    g_build_filename (PROJECT->exports_dir,
                      filename,
                      NULL);
  g_free (filename);

  /* make exports dir if not there yet */
  io_mkdir (PROJECT->exports_dir);
  g_message ("exporting %s",
             info.file_uri);

  /* stop engine and give it some time to stop
   * running */
  g_atomic_int_set (&AUDIO_ENGINE->run, 0);
  g_usleep (1000);
  AUDIO_ENGINE->exporting = 1;
  int prev_loop = TRANSPORT->loop;
  TRANSPORT->loop = 0;

  /* export */
  exporter_export (&info);

  /* restart engine */
  AUDIO_ENGINE->exporting = 0;
  TRANSPORT->loop = prev_loop;
  g_atomic_int_set (&AUDIO_ENGINE->run, 1);
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

  update_text (self);

  g_signal_connect (
    G_OBJECT (self->export_button), "clicked",
    G_CALLBACK (on_export_clicked), self);
  g_signal_connect (
    G_OBJECT (self->format), "changed",
    G_CALLBACK (on_format_changed), self);
  g_signal_connect (
    G_OBJECT (self->time_range_song), "toggled",
    G_CALLBACK (on_song_toggled), self);
  g_signal_connect (
    G_OBJECT (self->time_range_loop), "toggled",
    G_CALLBACK (on_loop_toggled), self);
  g_signal_connect (
    G_OBJECT (self->time_range_custom), "toggled",
    G_CALLBACK (on_custom_toggled), self);

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
    bit_depth);
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

  gtk_toggle_button_set_active (
    GTK_TOGGLE_BUTTON (self->dither),
    g_settings_get_int (
      S_PREFERENCES,
      "export-dither"));

  setup_bit_depth_combo_box (self);
  setup_formats_combo_box (self);
  setup_filename_pattern_combo_box (self);

  if (gtk_combo_box_get_active (self->format) ==
        AUDIO_FORMAT_OGG)
    {
      gtk_widget_set_sensitive (
        GTK_WIDGET (self->bit_depth), 0);
    }
  else
    {
      gtk_widget_set_sensitive (
        GTK_WIDGET (self->bit_depth), 1);
    }
}
