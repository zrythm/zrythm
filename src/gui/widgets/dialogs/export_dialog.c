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

#include "audio/engine.h"
#include "audio/exporter.h"
#include "audio/master_track.h"
#include "gui/widgets/dialogs/export_dialog.h"
#include "gui/widgets/dialogs/export_progress_dialog.h"
#include "project.h"
#include "utils/color.h"
#include "utils/gtk.h"
#include "utils/io.h"
#include "utils/resources.h"
#include "utils/ui.h"
#include "settings/settings.h"
#include "zrythm_app.h"

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

static char *
get_export_filename (ExportDialogWidget * self)
{
  char * format =
    exporter_stringize_audio_format (
      gtk_combo_box_get_active (
        self->format));
  char * base =
    g_strdup_printf ("%s.%s",
                     "mixdown", // TODO replace
                     format);
  char * exports_dir =
    project_get_path (
      PROJECT, PROJECT_PATH_EXPORTS, false);
  char * tmp =
    g_build_filename (
      exports_dir,
      base,
      NULL);
  char * full_path =
    io_get_next_available_filepath (tmp);
  g_free (base);
  g_free (tmp);
  g_free (format);
  g_free (exports_dir);

  /* we now have the full path, get only the
   * basename */
  base =
    g_path_get_basename (full_path);
  g_free (full_path);

  return base;
}

static void
update_text (ExportDialogWidget * self)
{
  char * filename =
    get_export_filename (self);

  char matcha[10];
  ui_gdk_rgba_to_hex (&UI_COLORS->matcha, matcha);

#define ORANGIZE(x) \
  "<span " \
  "foreground=\"" matcha "\">" x "</span>"

  char * exports_dir =
    project_get_path (
      PROJECT, PROJECT_PATH_EXPORTS, false);
  char * str =
    g_strdup_printf (
      "The following files will be created:\n"
      "<span foreground=\"%s\">%s</span>"
      "\n\n"
      "in the directory:\n"
      "<a href=\"%s\">%s</a>",
      matcha,
      filename,
      exports_dir,
      exports_dir);
  gtk_label_set_markup (
    self->output_label,
    str);
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
  gtk_toggle_button_set_active (
    self->stems_toggle,
    !gtk_toggle_button_get_active (toggle));
}

static void
on_stems_toggled (
  GtkToggleButton * toggle,
  ExportDialogWidget * self)
{
  gtk_toggle_button_set_active (
    self->mixdown_toggle,
    !gtk_toggle_button_get_active (toggle));
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

  for (int i = 0; i < NUM_AUDIO_FORMATS; i++)
    {
      gtk_tree_store_append (store, &iter, NULL);
      char * str =
        exporter_stringize_audio_format (i);
      gtk_tree_store_set (
        store, &iter,
        COLUMN_AUDIO_FORMAT_LABEL, str,
        COLUMN_AUDIO_FORMAT, i,
        -1);
      g_free (str);
    }

  return GTK_TREE_MODEL (store);
}

static void
on_format_changed (GtkComboBox *widget,
               ExportDialogWidget * self)
{
  update_text (self);
  AudioFormat format =
    gtk_combo_box_get_active (widget);

#define SET_SENSITIVE(x) \
  gtk_widget_set_sensitive ( \
    GTK_WIDGET (self->x), 1)

#define SET_UNSENSITIVE(x) \
  gtk_widget_set_sensitive ( \
    GTK_WIDGET (self->x), 0)

  SET_UNSENSITIVE (export_genre);
  SET_UNSENSITIVE (export_artist);
  SET_UNSENSITIVE (bit_depth);
  SET_UNSENSITIVE (dither);

  switch (format)
    {
    case AUDIO_FORMAT_MIDI:
      break;
    case AUDIO_FORMAT_OGG:
      SET_SENSITIVE (export_genre);
      SET_SENSITIVE (export_artist);
      SET_SENSITIVE (dither);
      break;
    default:
      SET_SENSITIVE (export_genre);
      SET_SENSITIVE (export_artist);
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
    g_settings_get_enum (
      S_EXPORT,
      "format"));
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

static void
on_export_clicked (
  GtkButton * btn,
  ExportDialogWidget * self)
{
  ExportSettings info;
  info.format =
    gtk_combo_box_get_active (self->format);
  g_settings_set_enum (
    S_EXPORT,
    "format",
    info.format);
  info.depth =
    gtk_combo_box_get_active (self->bit_depth);
  g_settings_set_enum (
    S_EXPORT,
    "bit-depth",
    info.depth);
  info.dither =
    gtk_toggle_button_get_active (
      GTK_TOGGLE_BUTTON (self->dither));
  g_settings_set_boolean (
    S_EXPORT, "dither", info.dither);
  info.artist =
    g_strdup (
      gtk_entry_get_text (self->export_artist));
  info.genre =
    g_strdup (
      gtk_entry_get_text (self->export_genre));
  g_settings_set_string (
    S_EXPORT,
    "artist",
    info.artist);
  g_settings_set_string (
    S_EXPORT,
    "genre",
    info.genre);

#define SET_TIME_RANGE(x) \
  g_settings_set_enum ( \
    S_EXPORT, \
    "time-range", TIME_RANGE_##x); \
  info.time_range = TIME_RANGE_##x

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

  char * exports_dir =
    project_get_path (
      PROJECT, PROJECT_PATH_EXPORTS, false);
  char * filename =
    get_export_filename (self);
  info.file_uri =
    g_build_filename (exports_dir,
                      filename,
                      NULL);
  g_free (filename);

  /* make exports dir if not there yet */
  io_mkdir (exports_dir);
  g_free (exports_dir);
  g_message ("exporting %s",
             info.file_uri);

  /* start exporting in a new thread */
  GThread * thread =
    g_thread_new (
      "export_thread",
      (GThreadFunc) exporter_generic_export_thread,
      &info);

  /* create a progress dialog and block */
  ExportProgressDialogWidget * progress_dialog =
    export_progress_dialog_widget_new (
      &info, 0, 1);
  gtk_window_set_transient_for (
    GTK_WINDOW (progress_dialog),
    GTK_WINDOW (self));
  g_signal_connect (
    G_OBJECT (progress_dialog), "response",
    G_CALLBACK (on_progress_dialog_closed), self);
  gtk_dialog_run (GTK_DIALOG (progress_dialog));
  gtk_widget_destroy (GTK_WIDGET (progress_dialog));

  g_thread_join (thread);

  /* restart engine */
  AUDIO_ENGINE->exporting = 0;
  TRANSPORT->loop = info.prev_loop;
  g_atomic_int_set (&AUDIO_ENGINE->run, 1);
  g_free (info.file_uri);
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

  /* add the children */
  for (int i = 0; i < track->num_children; i++)
    {
      Track * child =
        TRACKLIST->tracks[track->children[i]];
      g_return_if_fail (IS_TRACK (child));

      if (track_type_is_audio_group (child->type))
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

  /* clean up */
  gtk_tree_path_free (path);
}

static void
setup_treeview (
  ExportDialogWidget * self)
{
  self->tracks_model = create_model_for_tracks (self);
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

#if 0
  /* set search func */
  gtk_tree_view_set_search_equal_func (
    GTK_TREE_VIEW (tree_view),
    (GtkTreeViewSearchEqualFunc)
      plugin_search_equal_func,
    self, NULL);

  /* connect right click handler */
  GtkGestureMultiPress * mp =
    GTK_GESTURE_MULTI_PRESS (
      gtk_gesture_multi_press_new (
        GTK_WIDGET (tree_view)));
  gtk_gesture_single_set_button (
    GTK_GESTURE_SINGLE (mp),
    GDK_BUTTON_SECONDARY);
  g_signal_connect (
    G_OBJECT (mp), "pressed",
    G_CALLBACK (on_plugin_right_click), self);

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

  setup_treeview (self);

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

  gtk_entry_set_text (
    GTK_ENTRY (self->export_artist),
    g_settings_get_string (S_EXPORT, "artist"));
  gtk_entry_set_text (
    GTK_ENTRY (self->export_genre),
    g_settings_get_string (S_EXPORT, "genre"));

  gtk_toggle_button_set_active (
    self->time_range_song, false);
  gtk_toggle_button_set_active (
    self->time_range_loop, false);
  gtk_toggle_button_set_active (
    self->time_range_custom, false);
  switch (g_settings_get_enum (S_EXPORT, "time-range"))
    {
    case 0: // song
      gtk_toggle_button_set_active (
        self->time_range_song, true);
      break;
    case 1: // loop
      gtk_toggle_button_set_active (
        self->time_range_loop, true);
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
