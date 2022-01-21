/*
 * Copyright (C) 2018-2022 Alexandros Theodotou <alex at zrythm dot org>
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

#include "audio/engine.h"
#include "audio/exporter.h"
#include "audio/master_track.h"
#include "gui/widgets/dialogs/export_dialog.h"
#include "gui/widgets/dialogs/export_progress_dialog.h"
#include "gui/widgets/main_window.h"
#include "project.h"
#include "utils/arrays.h"
#include "utils/color.h"
#include "utils/datetime.h"
#include "utils/flags.h"
#include "utils/gtk.h"
#include "utils/io.h"
#include "utils/mem.h"
#include "utils/objects.h"
#include "utils/resources.h"
#include "utils/ui.h"
#include "settings/settings.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>
#include <gtk/gtk.h>

G_DEFINE_TYPE (
  ExportDialogWidget,
  export_dialog_widget,
  GTK_TYPE_DIALOG)

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

enum
{
  TRACK_COLUMN_CHECKBOX,
  TRACK_COLUMN_ICON,
  TRACK_COLUMN_BG_RGBA,
  TRACK_COLUMN_DUMMY_TEXT,
  TRACK_COLUMN_NAME,
  TRACK_COLUMN_TRACK,
  NUM_TRACK_COLUMNS,
};

static const char * dummy_text = "";

static void
add_enabled_recursively (
  ExportDialogWidget * self,
  GtkTreeIter *        iter,
  Track ***            tracks,
  size_t *             size,
  int *                count)
{
  GtkTreeModel * model =
    GTK_TREE_MODEL (self->tracks_store);

  Track * track;
  gboolean checked;
  gtk_tree_model_get (
    model, iter,
    TRACK_COLUMN_CHECKBOX, &checked,
    TRACK_COLUMN_TRACK, &track,
    -1);
  if (checked)
    {
      array_double_size_if_full (
        *tracks, *count, *size, Track *);
      array_append (*tracks, *count, track);
      g_debug ("added %s", track->name);
    }

  /* if group track, also check children
   * recursively */
  bool has_children =
    gtk_tree_model_iter_has_child (model, iter);
  if (has_children)
    {
      int num_children =
        gtk_tree_model_iter_n_children (
          model, iter);

      for (int i = 0; i < num_children; i++)
        {
          GtkTreeIter child_iter;
          gtk_tree_model_iter_nth_child (
            model, &child_iter, iter, i);

          /* recurse */
          add_enabled_recursively (
            self, &child_iter, tracks, size, count);
        }
    }
}

/**
 * Returns the currently checked tracks.
 *
 * Must be free'd with free().
 *
 * @param[out] num_tracks Number of tracks returned.
 *
 * @return Newly allocated track array, or NULL if
 *   no tracks selected.
 */
static Track **
get_enabled_tracks (
  ExportDialogWidget * self,
  int *                num_tracks)
{
  size_t size = 1;
  int count = 0;
  Track ** tracks = object_new_n (size, Track *);

  GtkTreeModel * model =
    GTK_TREE_MODEL (self->tracks_store);
  GtkTreeIter iter;
  gtk_tree_model_get_iter_first (
    model, &iter);

  add_enabled_recursively (
    self, &iter, &tracks, &size, &count);

  if (count == 0)
    {
      *num_tracks = 0;
      free (tracks);
      return NULL;
    }
  else
    {
      *num_tracks = count;
      return tracks;
    }
}

static char *
get_mixdown_export_filename (
  ExportDialogWidget * self)
{
  const char * mixdown_str = "mixdown";
  const char * format =
    exporter_stringize_audio_format (
      gtk_combo_box_get_active (self->format),
      true);
  char * datetime_str =
    datetime_get_for_filename ();
  char * base = NULL;
  switch (g_settings_get_enum (
            S_EXPORT, "filename-pattern"))
    {
    case EFP_APPEND_FORMAT:
      base =
        g_strdup_printf (
          "%s.%s", mixdown_str, format);
      break;
    case EFP_PREPEND_DATE_APPEND_FORMAT:
      base =
        g_strdup_printf (
          "%s_%s.%s", datetime_str,
          mixdown_str, format);
      break;
    default:
      g_return_val_if_reached (NULL);
    }
  g_return_val_if_fail (base, NULL);

  char * exports_dir =
    project_get_path (
      PROJECT, PROJECT_PATH_EXPORTS, false);
  char * tmp =
    g_build_filename (
      exports_dir, base, NULL);
  char * full_path =
    io_get_next_available_filepath (tmp);
  g_free (base);
  g_free (tmp);
  g_free (exports_dir);
  g_free (datetime_str);

  /* we now have the full path, get only the
   * basename */
  base = g_path_get_basename (full_path);
  g_free (full_path);

  return base;
}

/**
 * Gets the filename string for stems.
 *
 * @param max_files Max files to show, then append
 *   "..." at the end.
 * @param track If non-NULL, assumed to be the
 *   stem for this track.
 */
static char *
get_stem_export_filenames (
  ExportDialogWidget * self,
  int                  max_files,
  Track *              in_track)
{
  int num_tracks;
  Track ** tracks =
    get_enabled_tracks (self, &num_tracks);

  if (!tracks)
    {
      return g_strdup (_("none"));
    }

  const char * format =
    exporter_stringize_audio_format (
      gtk_combo_box_get_active (self->format),
      true);
  char * datetime_str =
    datetime_get_for_filename ();

  GString * gstr = g_string_new (NULL);

  int new_max_files = MIN (num_tracks, max_files);

  for (int i = 0; i < new_max_files; i++)
    {
      Track * track = tracks[i];

      if (in_track)
        {
          track = in_track;
        }

      char * base = NULL;
      switch (g_settings_get_enum (
                S_EXPORT, "filename-pattern"))
        {
        case EFP_APPEND_FORMAT:
          base =
            g_strdup_printf (
              "%s.%s", track->name, format);
          break;
        case EFP_PREPEND_DATE_APPEND_FORMAT:
          base =
            g_strdup_printf (
              "%s_%s.%s", datetime_str,
              track->name, format);
          break;
        default:
          g_return_val_if_reached (NULL);
        }
      g_return_val_if_fail (base, NULL);

      char * exports_dir =
        project_get_path (
          PROJECT, PROJECT_PATH_EXPORTS, false);
      char * tmp =
        g_build_filename (
          exports_dir, base, NULL);
      char * full_path =
        io_get_next_available_filepath (tmp);
      g_free (base);
      g_free (tmp);
      g_free (exports_dir);

      /* we now have the full path, get only the
       * basename */
      base = g_path_get_basename (full_path);
      g_free (full_path);

      g_string_append (gstr, base);

      if (in_track)
        {
          g_free (base);
          goto return_result;
        }

      if (i < (new_max_files - 1))
        {
          g_string_append (gstr, "\n");
        }
      else if (i == (new_max_files - 1) &&
               new_max_files < num_tracks)
        {
          if (num_tracks - new_max_files == 1)
            {
              g_string_append (
                gstr, _("\n1 more file..."));
            }
          else
            {
              g_string_append_printf (
                gstr, _("\n%d more files..."),
                num_tracks - new_max_files);
            }
        }
      g_free (base);
    }
return_result:
  g_free (datetime_str);

  return g_string_free (gstr, false);
}

static char *
get_exports_dir (void)
{
  bool export_stems =
    g_settings_get_boolean (
      S_EXPORT, "export-stems");
  return
    project_get_path (
      PROJECT,
      export_stems ?
        PROJECT_PATH_EXPORTS_STEMS :
        PROJECT_PATH_EXPORTS,
      false);
}

/**
 * Gets the export filename only, or absolute path
 * if @ref absolute is true.
 *
 * @param track If non-NULL, assumed to be the
 *   stem for this track.
 */
static char *
get_export_filename (
  ExportDialogWidget * self,
  bool                 absolute,
  Track *              track)
{
  bool export_stems =
    g_settings_get_boolean (
      S_EXPORT, "export-stems");
  char * filename = NULL;
  if (export_stems)
    {
      filename =
        get_stem_export_filenames (self, 4, track);
      if (absolute)
        {
          char * exports_dir = get_exports_dir ();
          char * abs_path =
            g_build_filename (
              exports_dir, filename, NULL);
          g_free (exports_dir);
          g_free (filename);
          return abs_path;
        }
      else
        {
          return filename;
        }
    }
  else
    {
      filename =
        get_mixdown_export_filename (self);

      if (absolute)
        {
          char * exports_dir = get_exports_dir ();
          char * abs_path =
            g_build_filename (
              exports_dir, filename, NULL);
          g_free (exports_dir);
          g_free (filename);
          return abs_path;
        }
      else
        {
          return filename;
        }
    }
}

static void
update_text (ExportDialogWidget * self)
{
  char * filename =
    get_export_filename (self, false, NULL);
  g_return_if_fail (filename);

  char matcha[10];
  ui_gdk_rgba_to_hex (&UI_COLORS->matcha, matcha);

#define ORANGIZE(x) \
  "<span " \
  "foreground=\"" matcha "\">" x "</span>"

  char * exports_dir = get_exports_dir ();
  char * str =
    g_strdup_printf (
      "%s\n"
      "<span foreground=\"%s\">%s</span>"
      "\n\n"
      "%s\n"
      "<a href=\"%s\">%s</a>",
      _("The following files will be created:"),
      matcha,
      filename,
      _("in the directory:"),
      exports_dir,
      exports_dir);
  gtk_label_set_markup (
    self->output_label, str);
  g_free (filename);
  g_free (str);
  g_free (exports_dir);

  g_signal_connect (
    G_OBJECT (self->output_label), "activate-link",
    G_CALLBACK (z_gtk_activate_dir_link_func), self);

#undef ORANGIZE
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

static void
on_mixdown_toggled (
  GtkToggleButton * toggle,
  ExportDialogWidget * self)
{
  bool export_stems =
    !gtk_toggle_button_get_active (toggle);
  g_settings_set_boolean (
    S_EXPORT, "export-stems", export_stems);
  gtk_toggle_button_set_active (
    self->stems_toggle, export_stems);
  update_text (self);
}

static void
on_stems_toggled (
  GtkToggleButton * toggle,
  ExportDialogWidget * self)
{
  bool export_stems =
    gtk_toggle_button_get_active (toggle);
  g_settings_set_boolean (
    S_EXPORT, "export-stems", export_stems);
  gtk_toggle_button_set_active (
    self->mixdown_toggle, !export_stems);
  update_text (self);
}

/**
 * Creates the combo box model for bit depth.
 */
static GtkTreeModel *
create_bit_depth_store (void)
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
create_filename_pattern_store (void)
{
  GtkTreeIter iter;
  GtkTreeStore *store;

  store = gtk_tree_store_new (1, G_TYPE_STRING);

  gtk_tree_store_append (store, &iter, NULL);
  gtk_tree_store_set (
    store, &iter,
    0, _("<name>.<format>"),
    -1);
  gtk_tree_store_append (store, &iter, NULL);
  gtk_tree_store_set (
    store, &iter,
    0, _("<date>_<name>.<format>"),
    -1);

  return GTK_TREE_MODEL (store);
}

static void
on_filename_pattern_changed (
  GtkComboBox *        widget,
  ExportDialogWidget * self)
{
  g_settings_set_enum (
    S_EXPORT, "filename-pattern",
    gtk_combo_box_get_active (widget));

  update_text (self);
}

static void
setup_filename_pattern_combo_box (
  ExportDialogWidget * self)
{
  GtkTreeModel * model =
    create_filename_pattern_store ();
  gtk_combo_box_set_model (
    self->filename_pattern, model);
  gtk_cell_layout_clear (
    GTK_CELL_LAYOUT (self->filename_pattern));
  GtkCellRenderer* renderer =
    gtk_cell_renderer_text_new ();
  gtk_cell_layout_pack_start (
    GTK_CELL_LAYOUT (self->filename_pattern),
    renderer, TRUE);
  gtk_cell_layout_set_attributes (
    GTK_CELL_LAYOUT (self->filename_pattern),
    renderer, "text", 0, NULL);

  gtk_combo_box_set_active (
    self->filename_pattern,
    g_settings_get_enum (
      S_EXPORT, "filename-pattern"));

  g_signal_connect (
    G_OBJECT (self->filename_pattern), "changed",
    G_CALLBACK (on_filename_pattern_changed), self);
}

/**
 * Creates the combo box model for the audio
 * formats.
 */
static GtkTreeModel *
create_formats_store (void)
{
  GtkTreeIter iter;
  GtkTreeStore *store;

  store =
    gtk_tree_store_new (NUM_AUDIO_FORMAT_COLUMNS,
                        G_TYPE_STRING,
                        G_TYPE_INT);

  for (int i = 0; i < NUM_AUDIO_FORMATS; i++)
    {
      gtk_tree_store_append (store, &iter, NULL);
      const char * str =
        exporter_stringize_audio_format (i, false);
      gtk_tree_store_set (
        store, &iter,
        COLUMN_AUDIO_FORMAT_LABEL, str,
        COLUMN_AUDIO_FORMAT, i,
        -1);
    }

  return GTK_TREE_MODEL (store);
}

static void
on_format_changed (
  GtkComboBox *        widget,
  ExportDialogWidget * self)
{
  update_text (self);
  AudioFormat format =
    gtk_combo_box_get_active (widget);

  g_settings_set_enum (
    S_EXPORT, "format", format);

#define SET_SENSITIVE(x) \
  gtk_widget_set_sensitive ( \
    GTK_WIDGET (self->x), 1)

#define SET_UNSENSITIVE(x) \
  gtk_widget_set_sensitive ( \
    GTK_WIDGET (self->x), 0)

  SET_UNSENSITIVE (export_genre);
  SET_UNSENSITIVE (export_artist);
  SET_UNSENSITIVE (export_title);
  SET_UNSENSITIVE (bit_depth);
  SET_UNSENSITIVE (dither);

  switch (format)
    {
    case AUDIO_FORMAT_MIDI:
      break;
    case AUDIO_FORMAT_OGG_VORBIS:
    case AUDIO_FORMAT_OGG_OPUS:
      SET_SENSITIVE (export_genre);
      SET_SENSITIVE (export_artist);
      SET_SENSITIVE (export_title);
      SET_SENSITIVE (dither);
      break;
    default:
      SET_SENSITIVE (export_genre);
      SET_SENSITIVE (export_artist);
      SET_SENSITIVE (export_title);
      SET_SENSITIVE (bit_depth);
      SET_SENSITIVE (dither);
      break;
    }

#undef SET_SENSITIVE
#undef SET_UNSENSITIVE
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
    g_settings_get_enum (S_EXPORT, "format"));
}

static void
on_cancel_clicked (GtkButton * btn,
                   ExportDialogWidget * self)
{
  gtk_window_close (GTK_WINDOW (self));
  /*gtk_widget_destroy (GTK_WIDGET (self));*/
}

static void
on_progress_dialog_closed (
  GtkDialog * dialog,
  int         response_id,
  ExportDialogWidget * self)
{
  update_text (self);
}

/**
 * @param track If non-NULL, assumed to be a stem
 *   for this track.
 */
static void
init_export_info (
  ExportDialogWidget * self,
  ExportSettings *     info,
  Track *              track)
{
  memset (info, 0, sizeof (ExportSettings));
  info->format =
    gtk_combo_box_get_active (self->format);
  g_settings_set_enum (
    S_EXPORT, "format", info->format);
  info->depth =
    gtk_combo_box_get_active (self->bit_depth);
  g_settings_set_enum (
    S_EXPORT, "bit-depth", info->depth);
  info->dither =
    gtk_toggle_button_get_active (self->dither);
  g_settings_set_boolean (
    S_EXPORT, "dither", info->dither);
  info->artist =
    g_strdup (
      gtk_editable_get_text (
        GTK_EDITABLE (self->export_artist)));
  info->title =
    g_strdup (
      gtk_editable_get_text (
        GTK_EDITABLE (self->export_title)));
  info->genre =
    g_strdup (
      gtk_editable_get_text (
        GTK_EDITABLE (self->export_genre)));
  g_settings_set_string (
    S_EXPORT, "artist", info->artist);
  g_settings_set_string (
    S_EXPORT, "title", info->title);
  g_settings_set_string (
    S_EXPORT, "genre", info->genre);

#define SET_TIME_RANGE(x) \
g_settings_set_enum ( \
S_EXPORT, "time-range", TIME_RANGE_##x); \
info->time_range = TIME_RANGE_##x

  if (gtk_toggle_button_get_active (
        self->time_range_song))
    {
      SET_TIME_RANGE (SONG);
    }
  else if (gtk_toggle_button_get_active (
             self->time_range_loop))
    {
      SET_TIME_RANGE (LOOP);
    }
  else if (gtk_toggle_button_get_active (
             self->time_range_custom))
    {
      SET_TIME_RANGE (CUSTOM);
    }

  info->file_uri =
    get_export_filename (self, true, track);

  info->bounce_with_parents = true;

  info->mode = EXPORT_MODE_TRACKS;
  info->progress_info.has_error = false;
  info->progress_info.cancelled = false;
  strcpy (info->progress_info.error_str, "");
}

static void
on_export_clicked (
  GtkButton * btn,
  ExportDialogWidget * self)
{
  bool export_stems =
    g_settings_get_boolean (
      S_EXPORT, "export-stems");

  int num_tracks;
  Track ** tracks =
    get_enabled_tracks (self, &num_tracks);
  if (!tracks)
    {
      ui_show_error_message (
        MAIN_WINDOW, false,
        _("No tracks to export"));
      return;
    }

  /* make exports dir if not there yet */
  char * exports_dir = get_exports_dir ();
  io_mkdir (exports_dir);
  g_free (exports_dir);

  if (export_stems)
    {
      /* export each track individually */
      for (int i = 0; i < num_tracks; i++)
        {
          /* unmark all tracks for bounce */
          tracklist_mark_all_tracks_for_bounce (
            TRACKLIST, false);

          Track * track = tracks[i];
          track_mark_for_bounce (
            track, F_BOUNCE, F_MARK_REGIONS,
            F_MARK_CHILDREN, F_MARK_PARENTS);

          ExportSettings info;
          init_export_info (self, &info, track);

          g_message ("exporting %s", info.file_uri);

          /* start exporting in a new thread */
          GThread * thread =
            g_thread_new (
              "export_thread",
              (GThreadFunc)
              exporter_generic_export_thread,
              &info);

          /* create a progress dialog and block */
          ExportProgressDialogWidget * progress_dialog =
            export_progress_dialog_widget_new (
              &info, true, true, F_CANCELABLE);
          gtk_window_set_transient_for (
            GTK_WINDOW (progress_dialog),
            GTK_WINDOW (self));
          g_signal_connect (
            G_OBJECT (progress_dialog), "response",
            G_CALLBACK (on_progress_dialog_closed),
            self);
          z_gtk_dialog_run (
            GTK_DIALOG (progress_dialog), true);

          g_thread_join (thread);

          g_free (info.file_uri);

          track->bounce = false;
        }
    }
  else /* if exporting mixdown */
    {
      ExportSettings info;
      init_export_info (self, &info, NULL);

      /* unmark all tracks for bounce */
      tracklist_mark_all_tracks_for_bounce (
        TRACKLIST, false);

      /* mark all checked tracks for bounce */
      for (int i = 0; i < num_tracks; i++)
        {
          Track * track = tracks[i];
          track_mark_for_bounce (
            track, F_BOUNCE, F_MARK_REGIONS,
            F_NO_MARK_CHILDREN, F_MARK_PARENTS);
        }

      g_message ("exporting %s", info.file_uri);

      /* start exporting in a new thread */
      GThread * thread =
        g_thread_new (
          "export_thread",
          (GThreadFunc) exporter_generic_export_thread,
          &info);

      /* create a progress dialog and block */
      ExportProgressDialogWidget * progress_dialog =
        export_progress_dialog_widget_new (
          &info, true, true, F_CANCELABLE);
      gtk_window_set_transient_for (
        GTK_WINDOW (progress_dialog),
        GTK_WINDOW (self));
      g_signal_connect (
        G_OBJECT (progress_dialog), "response",
        G_CALLBACK (on_progress_dialog_closed), self);
      z_gtk_dialog_run (
        GTK_DIALOG (progress_dialog), true);

      g_thread_join (thread);

      g_free (info.file_uri);
    }
}

/**
 * Visible function for tracks tree model.
 *
 * Used for filtering based on selected options.
 */
static gboolean
tracks_tree_model_visible_func (
  GtkTreeModel *       model,
  GtkTreeIter  *       iter,
  ExportDialogWidget * self)
{
  bool visible = true;

  return visible;
}

static void
add_group_track_children (
  ExportDialogWidget * self,
  GtkTreeStore *       tree_store,
  GtkTreeIter *        iter,
  Track *              track)
{
  /* add the group */
  GtkTreeIter group_iter;
  gtk_tree_store_append (
    tree_store, &group_iter, iter);
  gtk_tree_store_set (
    tree_store, &group_iter,
    TRACK_COLUMN_CHECKBOX, true,
    TRACK_COLUMN_ICON, track->icon_name,
    TRACK_COLUMN_BG_RGBA, &track->color,
    TRACK_COLUMN_DUMMY_TEXT, dummy_text,
    TRACK_COLUMN_NAME, track->name,
    TRACK_COLUMN_TRACK, track,
    -1);

  g_debug (
    "%s: track '%s'", __func__, track->name);

  /* add the children */
  for (int i = 0; i < track->num_children; i++)
    {
      Track * child =
        tracklist_find_track_by_name_hash (
          TRACKLIST, track->children[i]);
      g_return_if_fail (
        IS_TRACK_AND_NONNULL (child));

      g_debug ("child: '%s'", child->name);

      if (child->num_children > 0)
        {
          add_group_track_children (
            self, tree_store, &group_iter, child);
        }
      else
        {
          GtkTreeIter child_iter;
          gtk_tree_store_append (
            tree_store, &child_iter, &group_iter);
          gtk_tree_store_set (
            tree_store, &child_iter,
            TRACK_COLUMN_CHECKBOX, true,
            TRACK_COLUMN_ICON, child->icon_name,
            TRACK_COLUMN_BG_RGBA, &child->color,
            TRACK_COLUMN_DUMMY_TEXT, dummy_text,
            TRACK_COLUMN_NAME, child->name,
            TRACK_COLUMN_TRACK, child,
            -1);
        }
    }
}

static GtkTreeModel *
create_model_for_tracks (
  ExportDialogWidget * self)
{
  /* checkbox, icon, foreground rgba,
   * background rgba, name, track */
  GtkTreeStore * tree_store =
    gtk_tree_store_new (
      NUM_TRACK_COLUMNS,
      G_TYPE_BOOLEAN,
      G_TYPE_STRING,
      GDK_TYPE_RGBA,
      G_TYPE_STRING,
      G_TYPE_STRING,
      G_TYPE_POINTER);

  /*GtkTreeIter iter;*/
  /*gtk_tree_store_append (tree_store, &iter, NULL);*/
  add_group_track_children (
    self, tree_store, NULL, P_MASTER_TRACK);

  self->tracks_store = tree_store;

  GtkTreeModel * model =
    gtk_tree_model_filter_new (
      GTK_TREE_MODEL (tree_store), NULL);
  gtk_tree_model_filter_set_visible_func (
    GTK_TREE_MODEL_FILTER (model),
    (GtkTreeModelFilterVisibleFunc)
    tracks_tree_model_visible_func,
    self, NULL);

  return model;
}

/**
 * This toggles on all parents recursively if
 * something is toggled.
 */
static void
set_track_toggle_on_parent_recursively (
  ExportDialogWidget * self,
  GtkTreeIter *        iter,
  bool                 toggled)
{
  if (!toggled)
    {
      return;
    }

  GtkTreeModel * model =
    GTK_TREE_MODEL (self->tracks_store);

  /* enable the parent if toggled */
  GtkTreeIter parent_iter;
  bool has_parent =
    gtk_tree_model_iter_parent (
      model, &parent_iter, iter);
  if (has_parent)
    {
      /* set new value on widget */
      gtk_tree_store_set (
        GTK_TREE_STORE (model), &parent_iter,
        TRACK_COLUMN_CHECKBOX, toggled, -1);

      set_track_toggle_on_parent_recursively (
        self, &parent_iter, toggled);
    }
}

static void
set_track_toggle_recursively (
  ExportDialogWidget * self,
  GtkTreeIter *        iter,
  bool                 toggled)
{
  GtkTreeModel * model =
    GTK_TREE_MODEL (self->tracks_store);

  /* if group track, also enable/disable children
   * recursively */
  bool has_children =
    gtk_tree_model_iter_has_child (model, iter);
  if (has_children)
    {
      int num_children =
        gtk_tree_model_iter_n_children (
          model, iter);

      for (int i = 0; i < num_children; i++)
        {
          GtkTreeIter child_iter;
          gtk_tree_model_iter_nth_child (
            model, &child_iter, iter, i);

          /* set new value on widget */
          gtk_tree_store_set (
            GTK_TREE_STORE (model), &child_iter,
            TRACK_COLUMN_CHECKBOX, toggled, -1);

          /* recurse */
          set_track_toggle_recursively (
            self, &child_iter, toggled);
        }
    }
}

static void
on_track_toggled (
  GtkCellRendererToggle * cell,
  gchar *                 path_str,
  ExportDialogWidget *    self)
{
  GtkTreeModel * model =
    GTK_TREE_MODEL (self->tracks_store);
  g_debug ("path str: %s", path_str);

  /* get tree path and iter */
  GtkTreePath * path =
    gtk_tree_path_new_from_string (path_str);
  GtkTreeIter  iter;
  bool ret =
    gtk_tree_model_get_iter (model, &iter, path);
  g_return_if_fail (ret);
  g_return_if_fail (
    gtk_tree_store_iter_is_valid (
      GTK_TREE_STORE (model), &iter));

  /* get toggled */
  gboolean toggled;
  gtk_tree_model_get (
    model, &iter,
    TRACK_COLUMN_CHECKBOX, &toggled, -1);
  g_return_if_fail (
    gtk_tree_store_iter_is_valid (
      GTK_TREE_STORE (model), &iter));

  /* get new value */
  toggled ^= 1;

  /* set new value on widget */
  gtk_tree_store_set (
    GTK_TREE_STORE (model), &iter,
    TRACK_COLUMN_CHECKBOX, toggled, -1);

  /* if exporting mixdown (single file) */
  if (!g_settings_get_boolean (
        S_EXPORT, "export-stems"))
    {
      /* toggle parents if toggled */
      set_track_toggle_on_parent_recursively (
        self, &iter, toggled);

      /* propagate value to children recursively */
      set_track_toggle_recursively (
        self, &iter, toggled);
    }

  update_text (self);

  /* clean up */
  gtk_tree_path_free (path);
}

/* TODO */
#if 0
/**
 * Returns the currently selected tracks.
 *
 * Must be free'd with free().
 *
 * @param[out] num_tracks Number of tracks returned.
 *
 * @return Newly allocated track array, or NULL if
 *   no tracks selected.
 */
static Track **
get_selected_tracks (
  ExportDialogWidget * self,
  int *                num_tracks)
{
  GtkTreeSelection * selection =
    gtk_tree_view_get_selection (
      (self->tracks_treeview));

  size_t size = 1;
  int count = 0;
  Track ** tracks =
    calloc (size, sizeof (Track *));

  GList * selected_rows =
    gtk_tree_selection_get_selected_rows (
      selection, NULL);
  GList * list_iter =
    g_list_first (selected_rows);
  while (list_iter)
    {
      GtkTreePath * tp = list_iter->data;
      gtk_tree_selection_select_path (
        selection, tp);
      GtkTreeIter iter;
      gtk_tree_model_get_iter (
        GTK_TREE_MODEL (self->tracks_store),
        &iter, tp);
      Track * track;
      gtk_tree_model_get (
        GTK_TREE_MODEL (self->tracks_store),
        &iter, TRACK_COLUMN_TRACK, &track, -1);

      array_double_size_if_full (
        tracks, count, size, Track *);
      array_append (tracks, size, track);
      g_debug ("track %s selected", track->name);

      list_iter = g_list_next (list_iter);
    }

  g_list_free_full (
    selected_rows,
    (GDestroyNotify) gtk_tree_path_free);

  if (count == 0)
    {
      *num_tracks = 0;
      free (tracks);
      return NULL;
    }
  else
    {
      *num_tracks = count;
      return tracks;
    }
}

static void
enable_or_disable_tracks (
  ExportDialogWidget * self,
  bool                 enable)
{
  int num_tracks;
  Track ** tracks =
    get_selected_tracks (self, &num_tracks);

  if (tracks)
    {
      /* TODO enable/disable */
    }
}

static void
on_tracks_disable (
  GtkMenuItem *        menuitem,
  ExportDialogWidget * self)
{
  g_message ("tracks disable");
  enable_or_disable_tracks (self, false);
}

static void
on_tracks_enable (
  GtkMenuItem *        menuitem,
  ExportDialogWidget * self)
{
  g_message ("tracks enable");
  enable_or_disable_tracks (self, true);
}
#endif

static void
show_tracks_context_menu (
  ExportDialogWidget * self)
{
#if 0
  GtkWidget *menuitem;
  GtkWidget * menu = gtk_menu_new();

  /* FIXME this is allocating memory every time */

  menuitem =
    gtk_menu_item_new_with_label (_("Enable"));
  gtk_widget_set_visible (menuitem, true);
  gtk_menu_shell_append (
    GTK_MENU_SHELL (menu),
    GTK_WIDGET (menuitem));
  g_signal_connect (
    G_OBJECT (menuitem), "activate",
    G_CALLBACK (on_tracks_enable), self);

  menuitem =
    gtk_menu_item_new_with_label (_("Disable"));
  gtk_widget_set_visible (menuitem, true);
  gtk_menu_shell_append (
    GTK_MENU_SHELL (menu),
    GTK_WIDGET (menuitem));
  g_signal_connect (
    G_OBJECT (menuitem), "activate",
    G_CALLBACK (on_tracks_disable), self);

  gtk_menu_attach_to_widget (
    GTK_MENU (menu),
    GTK_WIDGET (self), NULL);
  gtk_menu_popup_at_pointer (GTK_MENU (menu), NULL);
#endif
}

static void
on_track_right_click (
  GtkGestureClick * gesture,
  gint                   n_press,
  gdouble                x_dbl,
  gdouble                y_dbl,
  ExportDialogWidget *   self)
{
  if (n_press != 1)
    return;

  show_tracks_context_menu (self);
}

static void
setup_treeview (
  ExportDialogWidget * self)
{
  self->tracks_model =
    create_model_for_tracks (self);
  gtk_tree_view_set_model (
    self->tracks_treeview, self->tracks_model);

  GtkTreeView * tree_view = self->tracks_treeview;

  /* init tree view */
  GtkCellRenderer * renderer;
  GtkTreeViewColumn * column;

  /* column for checkbox */
  renderer = gtk_cell_renderer_toggle_new ();
  column =
    gtk_tree_view_column_new_with_attributes (
      "icon", renderer,
      "active", TRACK_COLUMN_CHECKBOX,
      NULL);
  gtk_tree_view_append_column (tree_view, column);
  g_signal_connect (
    renderer, "toggled",
    G_CALLBACK (on_track_toggled), self);

  /* column for color */
  renderer = gtk_cell_renderer_text_new ();
  column =
    gtk_tree_view_column_new_with_attributes (
      "name", renderer,
      "text", TRACK_COLUMN_DUMMY_TEXT,
      "background-rgba", TRACK_COLUMN_BG_RGBA,
      NULL);
  gtk_tree_view_append_column (tree_view, column);

  /* column for icon */
  renderer = gtk_cell_renderer_pixbuf_new ();
  column =
    gtk_tree_view_column_new_with_attributes (
      "icon", renderer,
      "icon-name", TRACK_COLUMN_ICON,
      NULL);
  gtk_tree_view_append_column (tree_view, column);

  /* column for name */
  renderer = gtk_cell_renderer_text_new ();
  column =
    gtk_tree_view_column_new_with_attributes (
      "name", renderer,
      "text", TRACK_COLUMN_NAME,
      NULL);
  gtk_tree_view_append_column (tree_view, column);

  /* set search column */
  gtk_tree_view_set_search_column (
    tree_view, TRACK_COLUMN_NAME);

  /* hide headers */
  gtk_tree_view_set_headers_visible (
    GTK_TREE_VIEW (tree_view), false);

  gtk_tree_selection_set_mode (
    gtk_tree_view_get_selection (tree_view),
    GTK_SELECTION_MULTIPLE);

  /* connect right click handler */
  GtkGestureClick * mp =
    GTK_GESTURE_CLICK (
      gtk_gesture_click_new ());
  gtk_gesture_single_set_button (
    GTK_GESTURE_SINGLE (mp),
    GDK_BUTTON_SECONDARY);
  g_signal_connect (
    G_OBJECT (mp), "pressed",
    G_CALLBACK (on_track_right_click), self);
  gtk_widget_add_controller (
    GTK_WIDGET (tree_view),
    GTK_EVENT_CONTROLLER (mp));

#if 0
  /* set search func */
  gtk_tree_view_set_search_equal_func (
    GTK_TREE_VIEW (tree_view),
    (GtkTreeViewSearchEqualFunc)
      plugin_search_equal_func,
    self, NULL);

  GtkTreeSelection * sel =
    gtk_tree_view_get_selection (
      GTK_TREE_VIEW (tree_view));
  g_signal_connect (
    G_OBJECT (sel), "changed",
    G_CALLBACK (on_selection_changed), self);
#endif
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
  g_signal_connect (
    G_OBJECT (self->mixdown_toggle), "toggled",
    G_CALLBACK (on_mixdown_toggled), self);
  g_signal_connect (
    G_OBJECT (self->stems_toggle), "toggled",
    G_CALLBACK (on_stems_toggled), self);

  return self;
}

static void
export_dialog_widget_class_init (
  ExportDialogWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (
    klass, "export_dialog.ui");

#define BIND_CHILD(x) \
  gtk_widget_class_bind_template_child ( \
    klass, ExportDialogWidget, x)

  BIND_CHILD (cancel_button);
  BIND_CHILD (export_button);
  BIND_CHILD (export_artist);
  BIND_CHILD (export_title);
  BIND_CHILD (export_genre);
  BIND_CHILD (filename_pattern);
  BIND_CHILD (bit_depth);
  BIND_CHILD (time_range_song);
  BIND_CHILD (time_range_loop);
  BIND_CHILD (time_range_custom);
  BIND_CHILD (format);
  BIND_CHILD (dither);
  BIND_CHILD (output_label);
  BIND_CHILD (tracks_treeview);
  BIND_CHILD (mixdown_toggle);
  BIND_CHILD (stems_toggle);

#undef BIND_CHILD

  gtk_widget_class_bind_template_callback (
    klass, on_cancel_clicked);
}

static void
export_dialog_widget_init (
  ExportDialogWidget * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  gtk_editable_set_text (
    GTK_EDITABLE (self->export_artist),
    g_settings_get_string (S_EXPORT, "artist"));
  gtk_editable_set_text (
    GTK_EDITABLE (self->export_title),
    g_settings_get_string (S_EXPORT, "title"));
  gtk_editable_set_text (
    GTK_EDITABLE (self->export_genre),
    g_settings_get_string (S_EXPORT, "genre"));

  gtk_toggle_button_set_active (
    self->time_range_song, false);
  gtk_toggle_button_set_active (
    self->time_range_loop, false);
  gtk_toggle_button_set_active (
    self->time_range_custom, false);
  switch (g_settings_get_enum (S_EXPORT, "time-range"))
    {
    case 0: // loop
      gtk_toggle_button_set_active (
        self->time_range_loop, true);
      break;
    case 1: // song
      gtk_toggle_button_set_active (
        self->time_range_song, true);
      break;
    case 2: // custom
      gtk_toggle_button_set_active (
        self->time_range_custom, true);
      break;
    }
  bool export_stems =
    g_settings_get_boolean (
      S_EXPORT, "export-stems");
  gtk_toggle_button_set_active (
    self->mixdown_toggle, !export_stems);
  gtk_toggle_button_set_active (
    self->stems_toggle, export_stems);

  gtk_toggle_button_set_active (
    GTK_TOGGLE_BUTTON (self->dither),
    g_settings_get_boolean (
      S_EXPORT, "dither"));

  setup_bit_depth_combo_box (self);
  setup_formats_combo_box (self);
  setup_filename_pattern_combo_box (self);

  setup_treeview (self);

  on_format_changed (self->format, self);
}
