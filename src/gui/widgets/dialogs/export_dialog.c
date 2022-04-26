// SPDX-FileCopyrightText: © 2018-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "audio/engine.h"
#include "audio/exporter.h"
#include "audio/master_track.h"
#include "audio/router.h"
#include "gui/widgets/dialogs/export_dialog.h"
#include "gui/widgets/dialogs/export_progress_dialog.h"
#include "gui/widgets/main_window.h"
#include "project.h"
#include "settings/settings.h"
#include "utils/arrays.h"
#include "utils/color.h"
#include "utils/datetime.h"
#include "utils/flags.h"
#include "utils/gtk.h"
#include "utils/io.h"
#include "utils/mem.h"
#include "utils/objects.h"
#include "utils/resources.h"
#include "utils/string.h"
#include "utils/ui.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>
#include <gtk/gtk.h>

G_DEFINE_TYPE (
  ExportDialogWidget,
  export_dialog_widget,
  GTK_TYPE_DIALOG)

enum
{
  COLUMN_EXPORT_FORMAT_LABEL,
  COLUMN_EXPORT_FORMAT,
  NUM_EXPORT_FORMAT_COLUMNS
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

#define AUDIO_STACK_VISIBLE(self) \
  (string_is_equal ( \
    adw_view_stack_get_visible_child_name ( \
      self->stack), \
    "audio"))

/**
 * Returns either the audio or MIDI settings
 * depending on the current stack page.
 */
static GSettings *
get_current_settings (ExportDialogWidget * self)
{
  if (AUDIO_STACK_VISIBLE (self))
    {
      return S_EXPORT_AUDIO;
    }
  else
    {
      return S_EXPORT_MIDI;
    }
}

static void
add_enabled_recursively (
  ExportDialogWidget * self,
  GtkTreeView *        tree_view,
  GtkTreeIter *        iter,
  Track ***            tracks,
  size_t *             size,
  int *                count)
{
  GtkTreeModelFilter * parent_model =
    GTK_TREE_MODEL_FILTER (
      gtk_tree_view_get_model (tree_view));
  GtkTreeModel * model =
    gtk_tree_model_filter_get_model (parent_model);

  Track *  track;
  gboolean checked;
  gtk_tree_model_get (
    model, iter, TRACK_COLUMN_CHECKBOX, &checked,
    TRACK_COLUMN_TRACK, &track, -1);
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
        gtk_tree_model_iter_n_children (model, iter);

      for (int i = 0; i < num_children; i++)
        {
          GtkTreeIter child_iter;
          gtk_tree_model_iter_nth_child (
            model, &child_iter, iter, i);

          /* recurse */
          add_enabled_recursively (
            self, tree_view, &child_iter, tracks,
            size, count);
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
  GtkTreeView *        tree_view,
  int *                num_tracks)
{
  size_t   size = 1;
  int      count = 0;
  Track ** tracks = object_new_n (size, Track *);

  GtkTreeModelFilter * parent_model =
    GTK_TREE_MODEL_FILTER (
      gtk_tree_view_get_model (tree_view));
  GtkTreeModel * model =
    gtk_tree_model_filter_get_model (parent_model);
  GtkTreeIter iter;
  gtk_tree_model_get_iter_first (model, &iter);

  add_enabled_recursively (
    self, tree_view, &iter, &tracks, &size, &count);

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
  ExportDialogWidget * self,
  bool                 audio)
{
  const char * mixdown_str = "mixdown";
  const char * format_ext = NULL;
  if (audio)
    {
      GtkStringObject * so = GTK_STRING_OBJECT (
        adw_combo_row_get_selected_item (
          self->audio_format));
      ExportFormat format =
        export_format_from_pretty_str (
          gtk_string_object_get_string (so));
      format_ext = export_format_to_ext (format);
    }
  else
    {
      format_ext =
        export_format_to_ext (EXPORT_FORMAT_MIDI0);
    }

  GSettings * s = get_current_settings (self);
  char *      base = NULL;
  switch (
    g_settings_get_enum (s, "filename-pattern"))
    {
    case EFP_APPEND_FORMAT:
      base = g_strdup_printf (
        "%s.%s", mixdown_str, format_ext);
      break;
    case EFP_PREPEND_DATE_APPEND_FORMAT:
      {
        char * datetime_str =
          datetime_get_for_filename ();
        base = g_strdup_printf (
          "%s_%s.%s", datetime_str, mixdown_str,
          format_ext);
        g_free (datetime_str);
      }
      break;
    default:
      g_return_val_if_reached (NULL);
    }
  g_return_val_if_fail (base, NULL);

  char * exports_dir = project_get_path (
    PROJECT, PROJECT_PATH_EXPORTS, false);
  char * tmp =
    g_build_filename (exports_dir, base, NULL);
  char * full_path =
    io_get_next_available_filepath (tmp);
  g_free (base);
  g_free (tmp);
  g_free (exports_dir);

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
  Track *              in_track,
  bool                 audio)
{
  GtkTreeView * tree_view =
    audio ? self->audio_tracks_treeview
          : self->midi_tracks_treeview;
  int      num_tracks;
  Track ** tracks = get_enabled_tracks (
    self, tree_view, &num_tracks);

  if (!tracks)
    {
      return g_strdup (_ ("none"));
    }

  const char * format_ext = NULL;
  if (audio)
    {
      GtkStringObject * so = GTK_STRING_OBJECT (
        adw_combo_row_get_selected_item (
          self->audio_format));
      ExportFormat format =
        export_format_from_pretty_str (
          gtk_string_object_get_string (so));
      format_ext = export_format_to_ext (format);
    }
  else
    {
      format_ext =
        export_format_to_ext (EXPORT_FORMAT_MIDI0);
    }
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

      GSettings * s =
        audio ? S_EXPORT_AUDIO : S_EXPORT_MIDI;
      char * base = NULL;
      switch (g_settings_get_enum (
        s, "filename-pattern"))
        {
        case EFP_APPEND_FORMAT:
          base = g_strdup_printf (
            "%s.%s", track->name, format_ext);
          break;
        case EFP_PREPEND_DATE_APPEND_FORMAT:
          base = g_strdup_printf (
            "%s_%s.%s", datetime_str, track->name,
            format_ext);
          break;
        default:
          g_return_val_if_reached (NULL);
        }
      g_return_val_if_fail (base, NULL);

      char * exports_dir = project_get_path (
        PROJECT, PROJECT_PATH_EXPORTS, false);
      char * tmp =
        g_build_filename (exports_dir, base, NULL);
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
      else if (
        i == (new_max_files - 1)
        && new_max_files < num_tracks)
        {
          if (num_tracks - new_max_files == 1)
            {
              g_string_append (
                gstr, _ ("\n1 more file..."));
            }
          else
            {
              g_string_append_printf (
                gstr, _ ("\n%d more files..."),
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
get_exports_dir (ExportDialogWidget * self)
{
  GSettings * s = get_current_settings (self);
  bool        export_stems =
    g_settings_get_boolean (s, "export-stems");
  return project_get_path (
    PROJECT,
    export_stems
      ? PROJECT_PATH_EXPORTS_STEMS
      : PROJECT_PATH_EXPORTS,
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
  GSettings * s = get_current_settings (self);
  bool is_audio = AUDIO_STACK_VISIBLE (self);
  bool export_stems =
    g_settings_get_boolean (s, "export-stems");
  char * filename = NULL;
  if (export_stems)
    {
      filename = get_stem_export_filenames (
        self, 4, track, is_audio);
      if (absolute)
        {
          char * exports_dir =
            get_exports_dir (self);
          char * abs_path = g_build_filename (
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
      filename = get_mixdown_export_filename (
        self, is_audio);

      if (absolute)
        {
          char * exports_dir =
            get_exports_dir (self);
          char * abs_path = g_build_filename (
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

  char * exports_dir = get_exports_dir (self);
  char * str = g_strdup_printf (
    "%s\n"
    "<span foreground=\"%s\">%s</span>"
    "\n\n"
    "%s\n"
    "<a href=\"%s\">%s</a>",
    _ ("The following files will be created:"),
    matcha, filename, _ ("in the directory:"),
    exports_dir, exports_dir);
  bool       is_audio = AUDIO_STACK_VISIBLE (self);
  GtkLabel * lbl =
    is_audio
      ? self->audio_output_label
      : self->midi_output_label;
  gtk_label_set_markup (lbl, str);
  g_free (filename);
  g_free (str);
  g_free (exports_dir);

  g_signal_connect (
    G_OBJECT (lbl), "activate-link",
    G_CALLBACK (z_gtk_activate_dir_link_func),
    self);

#undef ORANGIZE
}

static void
setup_bit_depth_drop_down (AdwComboRow * combo_row)
{
  GtkStringList * string_list =
    gtk_string_list_new (NULL);

  for (BitDepth i = BIT_DEPTH_16;
       i <= BIT_DEPTH_32; i++)
    {
      const char * str =
        audio_bit_depth_to_pretty_str (i);
      gtk_string_list_append (string_list, str);
    }

  adw_combo_row_set_model (
    combo_row, G_LIST_MODEL (string_list));

  int selected_bit_depth = g_settings_get_int (
    S_EXPORT_AUDIO, "bit-depth");
  BitDepth depth = audio_bit_depth_int_to_enum (
    selected_bit_depth);
  adw_combo_row_set_selected (
    combo_row, (guint) depth);
}

static void
on_filename_pattern_changed (
  AdwComboRow *        combo_row,
  GParamSpec *         pspec,
  ExportDialogWidget * self)
{
  GSettings * s = get_current_settings (self);

  g_settings_set_enum (
    s, "filename-pattern",
    (int) adw_combo_row_get_selected (combo_row));

  update_text (self);
}

static void
setup_audio_formats_dropdown (
  AdwComboRow * combo_row)
{
  GtkStringList * string_list =
    gtk_string_list_new (NULL);

  for (ExportFormat i = 0; i < NUM_EXPORT_FORMATS;
       i++)
    {
      if (
        i == EXPORT_FORMAT_MIDI0
        || i == EXPORT_FORMAT_MIDI1)
        continue;

      const char * str =
        export_format_to_pretty_str (i);
      gtk_string_list_append (string_list, str);
    }

  adw_combo_row_set_model (
    combo_row, G_LIST_MODEL (string_list));

  const char * selected_str = g_settings_get_string (
    S_EXPORT_AUDIO, "format");
  ExportFormat format =
    export_format_from_pretty_str (selected_str);
  adw_combo_row_set_selected (
    combo_row, (guint) format);
}

static void
on_visible_child_changed (
  AdwViewStack *       stack,
  GParamSpec *         pspec,
  ExportDialogWidget * self)
{
  update_text (self);
}

static void
on_progress_dialog_closed (
  GtkDialog *          dialog,
  int                  response_id,
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
  bool is_audio = AUDIO_STACK_VISIBLE (self);
  GSettings * s = get_current_settings (self);

  memset (info, 0, sizeof (ExportSettings));

  if (is_audio)
    {
      GtkStringObject * so = GTK_STRING_OBJECT (
        adw_combo_row_get_selected_item (
          self->audio_format));
      const char * pretty_str =
        gtk_string_object_get_string (so);
      info->format =
        export_format_from_pretty_str (pretty_str);
      g_settings_set_string (
        s, "format", pretty_str);
    }
  else
    {
      guint idx = adw_combo_row_get_selected (
        self->midi_format);
      if (idx == 0)
        info->format = EXPORT_FORMAT_MIDI0;
      else
        info->format = EXPORT_FORMAT_MIDI1;
      g_settings_set_int (s, "format", (int) idx);
    }

  if (is_audio)
    {
      BitDepth depth =
        (BitDepth) adw_combo_row_get_selected (
          self->audio_bit_depth);
      info->depth = depth;
      g_settings_set_int (
        S_EXPORT_AUDIO, "bit-depth",
        audio_bit_depth_enum_to_int (depth));

      info->dither = gtk_switch_get_active (
        self->audio_dither_switch);
      g_settings_set_boolean (
        s, "dither", info->dither);
    }

  if (!is_audio)
    {
      info->lanes_as_tracks = gtk_switch_get_active (
        self->midi_export_lanes_as_tracks_switch);
      g_settings_set_boolean (
        s, "lanes-as-tracks",
        info->lanes_as_tracks);
    }

  AdwEntryRow * title;
  AdwEntryRow * artist;
  AdwEntryRow * genre;
  if (is_audio)
    {
      title = self->audio_title;
      artist = self->audio_artist;
      genre = self->audio_genre;
    }
  else
    {
      title = self->midi_title;
      artist = self->midi_artist;
      genre = self->midi_genre;
    }

  info->artist = g_strdup (
    gtk_editable_get_text (GTK_EDITABLE (artist)));
  info->title = g_strdup (
    gtk_editable_get_text (GTK_EDITABLE (title)));
  info->genre = g_strdup (
    gtk_editable_get_text (GTK_EDITABLE (genre)));
  g_settings_set_string (s, "artist", info->artist);
  g_settings_set_string (s, "title", info->title);
  g_settings_set_string (s, "genre", info->genre);

  GtkDropDown * time_range_drop_down =
    is_audio
      ? self->audio_time_range_drop_down
      : self->midi_time_range_drop_down;
  info->time_range =
    (ExportTimeRange) gtk_drop_down_get_selected (
      time_range_drop_down);
  g_settings_set_enum (
    s, "time-range", info->time_range);

  info->file_uri =
    get_export_filename (self, true, track);

  info->bounce_with_parents = true;

  info->mode = EXPORT_MODE_TRACKS;
  info->progress_info.has_error = false;
  info->progress_info.cancelled = false;
  strcpy (info->progress_info.error_str, "");
}

/**
 * Export.
 *
 * @param audio Whether exporting audio, otherwise
 *   MIDI.
 */
static void
on_export (ExportDialogWidget * self, bool audio)
{
  GSettings * s =
    audio ? S_EXPORT_AUDIO : S_EXPORT_MIDI;
  GtkTreeView * tree_view =
    audio ? self->audio_tracks_treeview
          : self->midi_tracks_treeview;
  bool export_stems =
    g_settings_get_boolean (s, "export-stems");

  int      num_tracks;
  Track ** tracks = get_enabled_tracks (
    self, tree_view, &num_tracks);
  if (!tracks)
    {
      ui_show_error_message (
        MAIN_WINDOW, false,
        _ ("No tracks to export"));
      return;
    }

  /* make exports dir if not there yet */
  char * exports_dir = get_exports_dir (self);
  io_mkdir (exports_dir);
  g_free (exports_dir);

  if (export_stems)
    {
      /* export each track individually */
      for (int i = 0; i < num_tracks; i++)
        {
          Track * track = tracks[i];
          g_debug (
            "~ bouncing stem for %s ~",
            track->name);

          /* unmark all tracks for bounce */
          tracklist_mark_all_tracks_for_bounce (
            TRACKLIST, false);

          track_mark_for_bounce (
            track, F_BOUNCE, F_MARK_REGIONS,
            F_MARK_CHILDREN, F_MARK_PARENTS);

          ExportSettings info;
          init_export_info (self, &info, track);

          GPtrArray * conns =
            exporter_prepare_tracks_for_export (
              &info);

          g_message ("exporting %s", info.file_uri);

          /* start exporting in a new thread */
          GThread * thread = g_thread_new (
            "export_thread",
            (GThreadFunc)
              exporter_generic_export_thread,
            &info);

          /* create a progress dialog and block */
          ExportProgressDialogWidget *
            progress_dialog =
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

          /* re-connect disconnected connections */
          exporter_return_connections_post_export (
            &info, conns);

          g_free (info.file_uri);

          track->bounce = false;

          g_debug (
            "~ finished bouncing stem for %s ~",
            track->name);
        }
    }
  else /* if exporting mixdown */
    {
      g_debug ("~ bouncing mixdown ~");

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
      GThread * thread = g_thread_new (
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
        G_CALLBACK (on_progress_dialog_closed),
        self);
      z_gtk_dialog_run (
        GTK_DIALOG (progress_dialog), true);

      g_thread_join (thread);

      g_free (info.file_uri);

      g_debug ("~ finished bouncing mixdown ~");
    }

  free (tracks);
}

static void
on_response (
  GtkDialog * dialog,
  gint        response_id,
  gpointer    user_data)
{
  ExportDialogWidget * self =
    Z_EXPORT_DIALOG_WIDGET (user_data);

  if (response_id == GTK_RESPONSE_ACCEPT)
    {
      on_export (self, AUDIO_STACK_VISIBLE (self));
      return;
    }

  gtk_window_destroy (GTK_WINDOW (dialog));
}

/**
 * Visible function for tracks tree model.
 *
 * Used for filtering based on selected options.
 */
static gboolean
tracks_tree_model_visible_func (
  GtkTreeModel *       model,
  GtkTreeIter *        iter,
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
    tree_store, &group_iter, TRACK_COLUMN_CHECKBOX,
    true, TRACK_COLUMN_ICON, track->icon_name,
    TRACK_COLUMN_BG_RGBA, &track->color,
    TRACK_COLUMN_DUMMY_TEXT, dummy_text,
    TRACK_COLUMN_NAME, track->name,
    TRACK_COLUMN_TRACK, track, -1);

  g_debug ("%s: track '%s'", __func__, track->name);

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
            TRACK_COLUMN_TRACK, child, -1);
        }
    }
}

/**
 * This toggles on all parents recursively if
 * something is toggled.
 */
static void
set_track_toggle_on_parent_recursively (
  ExportDialogWidget * self,
  GtkTreeView *        tree_view,
  GtkTreeIter *        iter,
  bool                 toggled)
{
  if (!toggled)
    {
      return;
    }

  GtkTreeModelFilter * parent_model =
    GTK_TREE_MODEL_FILTER (
      gtk_tree_view_get_model (tree_view));
  GtkTreeModel * model =
    gtk_tree_model_filter_get_model (parent_model);

  /* enable the parent if toggled */
  GtkTreeIter parent_iter;
  bool has_parent = gtk_tree_model_iter_parent (
    model, &parent_iter, iter);
  if (has_parent)
    {
      /* set new value on widget */
      gtk_tree_store_set (
        GTK_TREE_STORE (model), &parent_iter,
        TRACK_COLUMN_CHECKBOX, toggled, -1);

      set_track_toggle_on_parent_recursively (
        self, tree_view, &parent_iter, toggled);
    }
}

static void
set_track_toggle_recursively (
  ExportDialogWidget * self,
  GtkTreeView *        tree_view,
  GtkTreeIter *        iter,
  bool                 toggled)
{
  GtkTreeModelFilter * parent_model =
    GTK_TREE_MODEL_FILTER (
      gtk_tree_view_get_model (tree_view));
  GtkTreeModel * model =
    gtk_tree_model_filter_get_model (parent_model);

  /* if group track, also enable/disable children
   * recursively */
  bool has_children =
    gtk_tree_model_iter_has_child (model, iter);
  if (has_children)
    {
      int num_children =
        gtk_tree_model_iter_n_children (model, iter);

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
            self, tree_view, &child_iter, toggled);
        }
    }
}

static void
on_track_toggled (
  GtkCellRendererToggle * cell,
  gchar *                 path_str,
  ExportDialogWidget *    self)
{
  bool is_audio = AUDIO_STACK_VISIBLE (self);
  GSettings * s = get_current_settings (self);

  GtkTreeView * tree_view =
    is_audio
      ? self->audio_tracks_treeview
      : self->midi_tracks_treeview;
  GtkTreeModelFilter * parent_model =
    GTK_TREE_MODEL_FILTER (
      gtk_tree_view_get_model (tree_view));
  GtkTreeModel * model =
    gtk_tree_model_filter_get_model (parent_model);
  g_debug ("path str: %s", path_str);

  /* get tree path and iter */
  GtkTreePath * path =
    gtk_tree_path_new_from_string (path_str);
  GtkTreeIter iter;
  bool        ret =
    gtk_tree_model_get_iter (model, &iter, path);
  g_return_if_fail (ret);
  g_return_if_fail (gtk_tree_store_iter_is_valid (
    GTK_TREE_STORE (model), &iter));

  /* get toggled */
  gboolean toggled;
  gtk_tree_model_get (
    model, &iter, TRACK_COLUMN_CHECKBOX, &toggled,
    -1);
  g_return_if_fail (gtk_tree_store_iter_is_valid (
    GTK_TREE_STORE (model), &iter));

  /* get new value */
  toggled ^= 1;

  /* set new value on widget */
  gtk_tree_store_set (
    GTK_TREE_STORE (model), &iter,
    TRACK_COLUMN_CHECKBOX, toggled, -1);

  /* if exporting mixdown (single file) */
  if (!g_settings_get_boolean (s, "export-stems"))
    {
      /* toggle parents if toggled */
      set_track_toggle_on_parent_recursively (
        self, tree_view, &iter, toggled);

      /* propagate value to children recursively */
      set_track_toggle_recursively (
        self, tree_view, &iter, toggled);
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
show_tracks_context_menu (ExportDialogWidget * self)
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
  GtkGestureClick *    gesture,
  gint                 n_press,
  gdouble              x_dbl,
  gdouble              y_dbl,
  ExportDialogWidget * self)
{
  if (n_press != 1)
    return;

  show_tracks_context_menu (self);
}

static GtkTreeModel *
create_model_for_tracks (ExportDialogWidget * self)
{
  /* checkbox, icon, foreground rgba,
   * background rgba, name, track */
  GtkTreeStore * tree_store = gtk_tree_store_new (
    NUM_TRACK_COLUMNS, G_TYPE_BOOLEAN,
    G_TYPE_STRING, GDK_TYPE_RGBA, G_TYPE_STRING,
    G_TYPE_STRING, G_TYPE_POINTER);

  /*GtkTreeIter iter;*/
  /*gtk_tree_store_append (tree_store, &iter, NULL);*/
  add_group_track_children (
    self, tree_store, NULL, P_MASTER_TRACK);

  GtkTreeModel * model = gtk_tree_model_filter_new (
    GTK_TREE_MODEL (tree_store), NULL);
  gtk_tree_model_filter_set_visible_func (
    GTK_TREE_MODEL_FILTER (model),
    (GtkTreeModelFilterVisibleFunc)
      tracks_tree_model_visible_func,
    self, NULL);

  return model;
}

static void
setup_tracks_treeview (
  ExportDialogWidget * self,
  bool                 is_audio)
{
  GtkTreeView * tree_view =
    is_audio
      ? self->audio_tracks_treeview
      : self->midi_tracks_treeview;

  GtkTreeModel * model =
    create_model_for_tracks (self);
  gtk_tree_view_set_model (tree_view, model);

  /* init tree view */
  GtkCellRenderer *   renderer;
  GtkTreeViewColumn * column;

  /* column for checkbox */
  renderer = gtk_cell_renderer_toggle_new ();
  column = gtk_tree_view_column_new_with_attributes (
    "icon", renderer, "active",
    TRACK_COLUMN_CHECKBOX, NULL);
  gtk_tree_view_append_column (tree_view, column);
  g_signal_connect (
    renderer, "toggled",
    G_CALLBACK (on_track_toggled), self);

  /* column for color */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (
    "name", renderer, "text",
    TRACK_COLUMN_DUMMY_TEXT, "background-rgba",
    TRACK_COLUMN_BG_RGBA, NULL);
  gtk_tree_view_append_column (tree_view, column);

  /* column for icon */
  renderer = gtk_cell_renderer_pixbuf_new ();
  column = gtk_tree_view_column_new_with_attributes (
    "icon", renderer, "icon-name",
    TRACK_COLUMN_ICON, NULL);
  gtk_tree_view_append_column (tree_view, column);

  /* column for name */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (
    "name", renderer, "text", TRACK_COLUMN_NAME,
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
    GTK_GESTURE_CLICK (gtk_gesture_click_new ());
  gtk_gesture_single_set_button (
    GTK_GESTURE_SINGLE (mp), GDK_BUTTON_SECONDARY);
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

static void
setup_filename_pattern_combo_row (
  ExportDialogWidget * self,
  AdwComboRow *        combo_row,
  bool                 is_audio)
{
  const char * strings[] = {
    _ ("<name>.<format>"),
    _ ("<date>_<name>.<format>"),
    NULL,
  };
  GtkStringList * string_list =
    gtk_string_list_new (strings);

  adw_combo_row_set_model (
    combo_row, G_LIST_MODEL (string_list));

  GSettings * s =
    is_audio ? S_EXPORT_AUDIO : S_EXPORT_MIDI;
  adw_combo_row_set_selected (
    combo_row,
    (guint) g_settings_get_enum (
      s, "filename-pattern"));

  g_signal_connect (
    G_OBJECT (combo_row), "notify::selected-item",
    G_CALLBACK (on_filename_pattern_changed), self);
}

static void
on_mixdown_stem_selection_changed (
  AdwComboRow *        combo_row,
  GParamSpec *         pspec,
  ExportDialogWidget * self)
{
  update_text (self);
}

static void
setup_mixdown_or_stems_combo_row (
  ExportDialogWidget * self,
  AdwComboRow *        combo_row,
  bool                 is_audio)
{
  const char * strings[] = {
    _ ("Mixdown"),
    _ ("Stems"),
    NULL,
  };
  GtkStringList * string_list =
    gtk_string_list_new (strings);

  adw_combo_row_set_model (
    combo_row, G_LIST_MODEL (string_list));

  adw_combo_row_set_selected (
    combo_row,
    (guint) g_settings_get_boolean (
      is_audio ? S_EXPORT_AUDIO : S_EXPORT_MIDI,
      "export-stems"));

  g_signal_connect (
    G_OBJECT (combo_row), "notify::selected-item",
    G_CALLBACK (on_mixdown_stem_selection_changed),
    self);
}

static void
setup_time_range_drop_down (
  ExportDialogWidget * self,
  GtkDropDown *        drop_down,
  bool                 is_audio)
{
  const char * strings[] = {
    C_ ("Export time range", "Loop"),
    C_ ("Export time range", "Song"),
    C_ ("Export time range", "Custom"),
    NULL,
  };
  GtkStringList * string_list =
    gtk_string_list_new (strings);

  gtk_drop_down_set_model (
    drop_down, G_LIST_MODEL (string_list));

  gtk_drop_down_set_selected (
    drop_down,
    (guint) g_settings_get_enum (
      is_audio ? S_EXPORT_AUDIO : S_EXPORT_MIDI,
      "time-range"));
}

static void
setup_dither (
  AdwActionRow * dither_row,
  GtkSwitch *    dither_switch)
{
  gtk_switch_set_active (
    dither_switch,
    g_settings_get_boolean (
      S_EXPORT_AUDIO, "dither"));

  char * descr = settings_get_description (
    S_EXPORT_AUDIO, "dither");
  adw_action_row_set_subtitle (dither_row, descr);
  g_free (descr);
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

  return self;
}

static void
export_dialog_widget_class_init (
  ExportDialogWidgetClass * _klass)
{
  GtkWidgetClass * klass =
    GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (
    klass, "export_dialog.ui");

#define BIND_CHILD(x) \
  gtk_widget_class_bind_template_child ( \
    klass, ExportDialogWidget, x)

  BIND_CHILD (title);
  BIND_CHILD (stack);
  BIND_CHILD (audio_title);
  BIND_CHILD (audio_artist);
  BIND_CHILD (audio_genre);
  BIND_CHILD (audio_format);
  BIND_CHILD (audio_bit_depth);
  BIND_CHILD (audio_dither);
  BIND_CHILD (audio_dither_switch);
  BIND_CHILD (audio_filename_pattern);
  BIND_CHILD (audio_mixdown_or_stems);
  BIND_CHILD (audio_time_range_drop_down);
  BIND_CHILD (audio_tracks_treeview);
  BIND_CHILD (audio_output_label);
  BIND_CHILD (midi_title);
  BIND_CHILD (midi_artist);
  BIND_CHILD (midi_genre);
  BIND_CHILD (midi_format);
  BIND_CHILD (midi_export_lanes_as_tracks_switch);
  BIND_CHILD (midi_filename_pattern);
  BIND_CHILD (midi_mixdown_or_stems);
  BIND_CHILD (midi_time_range_drop_down);
  BIND_CHILD (midi_tracks_treeview);
  BIND_CHILD (midi_output_label);

#undef BIND_CHILD
}

static void
export_dialog_widget_init (
  ExportDialogWidget * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  gtk_dialog_add_button (
    GTK_DIALOG (self), _ ("_Cancel"),
    GTK_RESPONSE_REJECT);
  gtk_dialog_add_button (
    GTK_DIALOG (self), _ ("_Export"),
    GTK_RESPONSE_ACCEPT);
  gtk_dialog_set_default_response (
    GTK_DIALOG (self), GTK_RESPONSE_ACCEPT);

  /* --- audio --- */

  /* metadata */
  gtk_editable_set_text (
    GTK_EDITABLE (self->audio_title),
    g_settings_get_string (S_EXPORT_AUDIO, "title"));
  gtk_editable_set_text (
    GTK_EDITABLE (self->audio_artist),
    g_settings_get_string (
      S_EXPORT_AUDIO, "artist"));
  gtk_editable_set_text (
    GTK_EDITABLE (self->audio_genre),
    g_settings_get_string (S_EXPORT_AUDIO, "genre"));

  /* options */
  setup_audio_formats_dropdown (self->audio_format);
  setup_dither (
    self->audio_dither, self->audio_dither_switch);
  setup_bit_depth_drop_down (self->audio_bit_depth);
  setup_filename_pattern_combo_row (
    self, self->audio_filename_pattern, true);
  setup_mixdown_or_stems_combo_row (
    self, self->audio_mixdown_or_stems, true);

  /* selections */
  setup_time_range_drop_down (
    self, self->audio_time_range_drop_down, true);
  setup_tracks_treeview (self, true);

  /* --- end audio --- */

  /* --- MIDI --- */

  /* metadata */
  gtk_editable_set_text (
    GTK_EDITABLE (self->midi_title),
    g_settings_get_string (S_EXPORT_MIDI, "title"));
  gtk_editable_set_text (
    GTK_EDITABLE (self->midi_artist),
    g_settings_get_string (S_EXPORT_MIDI, "artist"));
  gtk_editable_set_text (
    GTK_EDITABLE (self->midi_genre),
    g_settings_get_string (S_EXPORT_MIDI, "genre"));

  /* options */
  adw_combo_row_set_selected (
    self->midi_format,
    (guint) g_settings_get_int (
      S_EXPORT_MIDI, "format"));
  gtk_switch_set_active (
    self->midi_export_lanes_as_tracks_switch,
    g_settings_get_boolean (
      S_EXPORT_MIDI, "lanes-as-tracks"));
  setup_filename_pattern_combo_row (
    self, self->midi_filename_pattern, false);
  setup_mixdown_or_stems_combo_row (
    self, self->midi_mixdown_or_stems, false);

  /* selections */
  setup_time_range_drop_down (
    self, self->midi_time_range_drop_down, false);
  setup_tracks_treeview (self, false);

  /* --- end MIDI --- */

  g_signal_connect (
    G_OBJECT (self->stack), "notify::visible-child",
    G_CALLBACK (on_visible_child_changed), self);

  g_signal_connect (
    G_OBJECT (self), "response",
    G_CALLBACK (on_response), self);
}
