// SPDX-FileCopyrightText: © 2018-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/engine.h"
#include "dsp/exporter.h"
#include "dsp/master_track.h"
#include "dsp/router.h"
#include "dsp/tracklist.h"
#include "gui/backend/wrapped_object_with_change_signal.h"
#include "gui/widgets/dialogs/export_dialog.h"
#include "gui/widgets/dialogs/export_progress_dialog.h"
#include "gui/widgets/digital_meter.h"
#include "gui/widgets/item_factory.h"
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
#include "utils/progress_info.h"
#include "utils/resources.h"
#include "utils/string.h"
#include "utils/ui.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>

#include "gtk_wrapper.h"

G_DEFINE_TYPE (ExportDialogWidget, export_dialog_widget, GTK_TYPE_DIALOG)

#if 0
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
#endif

#define AUDIO_STACK_VISIBLE(self) \
  (string_is_equal ( \
    adw_view_stack_get_visible_child_name (self->stack), "audio"))

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
get_enabled_child_tracks_recursively (
  ExportDialogWidget *            self,
  WrappedObjectWithChangeSignal * wobj,
  GPtrArray *                     tracks)
{
  if (GPOINTER_TO_INT (g_object_get_data (G_OBJECT (wobj), "checked")))
    {
      Track * track = (Track *) wobj->obj;
      g_ptr_array_add (tracks, track);
    }
  if (!wobj->child_model)
    return;

  for (guint i = 0; i < g_list_model_get_n_items (wobj->child_model); i++)
    {
      WrappedObjectWithChangeSignal * child_wobj =
        Z_WRAPPED_OBJECT_WITH_CHANGE_SIGNAL (
          g_list_model_get_item (wobj->child_model, i));
      get_enabled_child_tracks_recursively (self, child_wobj, tracks);
    }
}

/**
 * Returns the currently checked tracks.
 *
 * @param[out] tracks Tracks array to fill.
 */
static void
get_enabled_tracks (
  ExportDialogWidget * self,
  GtkColumnView *      list_view,
  GPtrArray *          tracks)
{
  GtkNoSelection * nosel =
    GTK_NO_SELECTION (gtk_column_view_get_model (list_view));
  GtkTreeListModel * tree_model =
    GTK_TREE_LIST_MODEL (gtk_no_selection_get_model (nosel));
  GtkTreeListRow * row = gtk_tree_list_model_get_child_row (tree_model, 0);
  WrappedObjectWithChangeSignal * wobj =
    Z_WRAPPED_OBJECT_WITH_CHANGE_SIGNAL (gtk_tree_list_row_get_item (row));
  get_enabled_child_tracks_recursively (self, wobj, tracks);
}

static char *
get_mixdown_export_filename (ExportDialogWidget * self, bool audio)
{
  const char * mixdown_str = "mixdown";
  const char * format_ext = NULL;
  if (audio)
    {
      GtkStringObject * so = GTK_STRING_OBJECT (
        adw_combo_row_get_selected_item (self->audio_format));
      ExportFormat format =
        export_format_from_pretty_str (gtk_string_object_get_string (so));
      format_ext = export_format_to_ext (format);
    }
  else
    {
      format_ext = export_format_to_ext (ExportFormat::EXPORT_FORMAT_MIDI0);
    }

  GSettings * s = get_current_settings (self);
  char *      base = NULL;
  switch (ENUM_INT_TO_VALUE (
    ExportFilenamePattern, g_settings_get_enum (s, "filename-pattern")))
    {
    case ExportFilenamePattern::EFP_APPEND_FORMAT:
      base = g_strdup_printf ("%s.%s", mixdown_str, format_ext);
      break;
    case ExportFilenamePattern::EFP_PREPEND_DATE_APPEND_FORMAT:
      {
        char * datetime_str = datetime_get_for_filename ();
        base =
          g_strdup_printf ("%s_%s.%s", datetime_str, mixdown_str, format_ext);
        g_free (datetime_str);
      }
      break;
    default:
      g_return_val_if_reached (NULL);
    }
  g_return_val_if_fail (base, NULL);

  char * exports_dir =
    project_get_path (PROJECT, ProjectPath::PROJECT_PATH_EXPORTS, false);
  char * tmp = g_build_filename (exports_dir, base, NULL);
  char * full_path = io_get_next_available_filepath (tmp);
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
  size_t               max_files,
  Track *              in_track,
  bool                 audio)
{
  GtkColumnView * list_view =
    audio ? self->audio_tracks_view : self->midi_tracks_view;
  GPtrArray * tracks = g_ptr_array_new ();
  get_enabled_tracks (self, list_view, tracks);

  if (tracks->len == 0)
    {
      return g_strdup (_ ("none"));
    }

  const char * format_ext = NULL;
  if (audio)
    {
      GtkStringObject * so = GTK_STRING_OBJECT (
        adw_combo_row_get_selected_item (self->audio_format));
      ExportFormat format =
        export_format_from_pretty_str (gtk_string_object_get_string (so));
      format_ext = export_format_to_ext (format);
    }
  else
    {
      format_ext = export_format_to_ext (ExportFormat::EXPORT_FORMAT_MIDI0);
    }
  char * datetime_str = datetime_get_for_filename ();

  GString * gstr = g_string_new (NULL);

  size_t new_max_files = MIN (tracks->len, max_files);

  for (size_t i = 0; i < new_max_files; i++)
    {
      Track * track = static_cast<Track *> (g_ptr_array_index (tracks, i));

      if (in_track)
        {
          track = in_track;
        }

      GSettings * s = audio ? S_EXPORT_AUDIO : S_EXPORT_MIDI;
      char *      base = NULL;
      switch (ENUM_INT_TO_VALUE (
        ExportFilenamePattern, g_settings_get_enum (s, "filename-pattern")))
        {
        case ExportFilenamePattern::EFP_APPEND_FORMAT:
          base = g_strdup_printf ("%s.%s", track->name, format_ext);
          break;
        case ExportFilenamePattern::EFP_PREPEND_DATE_APPEND_FORMAT:
          base =
            g_strdup_printf ("%s_%s.%s", datetime_str, track->name, format_ext);
          break;
        default:
          g_return_val_if_reached (NULL);
        }
      g_return_val_if_fail (base, NULL);

      char * exports_dir =
        project_get_path (PROJECT, ProjectPath::PROJECT_PATH_EXPORTS, false);
      char * tmp = g_build_filename (exports_dir, base, NULL);
      char * full_path = io_get_next_available_filepath (tmp);
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
      else if (i == (new_max_files - 1) && new_max_files < tracks->len)
        {
          if (tracks->len - new_max_files == 1)
            {
              g_string_append (gstr, _ ("\n1 more file..."));
            }
          else
            {
              g_string_append_printf (
                gstr, _ ("\n%zu more files..."), tracks->len - new_max_files);
            }
        }
      g_free (base);
    }
return_result:
  g_free (datetime_str);

  g_ptr_array_unref (tracks);

  return g_string_free (gstr, false);
}

static char *
get_exports_dir (ExportDialogWidget * self)
{
  bool is_audio = AUDIO_STACK_VISIBLE (self);
  bool export_stems = (bool) adw_combo_row_get_selected (
    is_audio ? self->audio_mixdown_or_stems : self->midi_mixdown_or_stems);
  return project_get_path (
    PROJECT,
    export_stems
      ? ProjectPath::PROJECT_PATH_EXPORTS_STEMS
      : ProjectPath::PROJECT_PATH_EXPORTS,
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
get_export_filename (ExportDialogWidget * self, bool absolute, Track * track)
{
  bool is_audio = AUDIO_STACK_VISIBLE (self);
  bool export_stems = (bool) adw_combo_row_get_selected (
    is_audio ? self->audio_mixdown_or_stems : self->midi_mixdown_or_stems);
  char * filename = NULL;
  if (export_stems)
    {
      filename = get_stem_export_filenames (self, 4, track, is_audio);
      if (absolute)
        {
          char * exports_dir = get_exports_dir (self);
          char * abs_path = g_build_filename (exports_dir, filename, NULL);
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
      filename = get_mixdown_export_filename (self, is_audio);

      if (absolute)
        {
          char * exports_dir = get_exports_dir (self);
          char * abs_path = g_build_filename (exports_dir, filename, NULL);
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
  char * filename = get_export_filename (self, false, NULL);
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
    _ ("The following files will be created:"), matcha, filename,
    _ ("in the directory:"), exports_dir, exports_dir);
  bool is_audio = AUDIO_STACK_VISIBLE (self);
  GtkLabel * lbl = is_audio ? self->audio_output_label : self->midi_output_label;
  gtk_label_set_markup (lbl, str);
  g_free (filename);
  g_free (str);
  g_free (exports_dir);

  g_signal_connect (
    G_OBJECT (lbl), "activate-link", G_CALLBACK (z_gtk_activate_dir_link_func),
    self);

#undef ORANGIZE
}

static void
setup_bit_depth_drop_down (AdwComboRow * combo_row)
{
  GtkStringList * string_list = gtk_string_list_new (NULL);

  for (
    unsigned int i = ENUM_VALUE_TO_INT (BitDepth::BIT_DEPTH_16);
    i <= ENUM_VALUE_TO_INT (BitDepth::BIT_DEPTH_32); i++)
    {
      const char * str =
        audio_bit_depth_to_pretty_str (ENUM_INT_TO_VALUE (BitDepth, i));
      gtk_string_list_append (string_list, str);
    }

  adw_combo_row_set_model (combo_row, G_LIST_MODEL (string_list));

  int selected_bit_depth = g_settings_get_int (S_EXPORT_AUDIO, "bit-depth");
  BitDepth depth = audio_bit_depth_int_to_enum (selected_bit_depth);
  adw_combo_row_set_selected (combo_row, (guint) depth);
}

static void
on_filename_pattern_changed (
  AdwComboRow *        combo_row,
  GParamSpec *         pspec,
  ExportDialogWidget * self)
{
  GSettings * s = get_current_settings (self);

  g_settings_set_enum (
    s, "filename-pattern", (int) adw_combo_row_get_selected (combo_row));

  update_text (self);
}

static void
setup_audio_formats_dropdown (AdwComboRow * combo_row)
{
  GtkStringList * string_list = gtk_string_list_new (NULL);

  for (int i = 0; i < ENUM_COUNT (ExportFormat); i++)
    {
      ExportFormat cur = ENUM_INT_TO_VALUE (ExportFormat, i);
      if (
        cur == ExportFormat::EXPORT_FORMAT_MIDI0
        || cur == ExportFormat::EXPORT_FORMAT_MIDI1)
        continue;

      const char * str = export_format_to_pretty_str (cur);
      gtk_string_list_append (string_list, str);
    }

  adw_combo_row_set_model (combo_row, G_LIST_MODEL (string_list));

  const char * selected_str = g_settings_get_string (S_EXPORT_AUDIO, "format");
  ExportFormat format = export_format_from_pretty_str (selected_str);
  adw_combo_row_set_selected (combo_row, (guint) format);
}

static void
on_visible_child_changed (
  AdwViewStack *       stack,
  GParamSpec *         pspec,
  ExportDialogWidget * self)
{
  update_text (self);
}

/**
 * Initializes the export info struct.
 *
 * @param track If non-NULL, assumed to be a stem
 *   for this track.
 */
static ExportSettings *
init_export_info (ExportDialogWidget * self, Track * track)
{
  bool        is_audio = AUDIO_STACK_VISIBLE (self);
  GSettings * s = get_current_settings (self);

  ExportSettings * info = export_settings_new ();

  if (is_audio)
    {
      GtkStringObject * so = GTK_STRING_OBJECT (
        adw_combo_row_get_selected_item (self->audio_format));
      const char * pretty_str = gtk_string_object_get_string (so);
      info->format = export_format_from_pretty_str (pretty_str);
      g_settings_set_string (s, "format", pretty_str);
    }
  else
    {
      guint idx = adw_combo_row_get_selected (self->midi_format);
      if (idx == 0)
        info->format = ExportFormat::EXPORT_FORMAT_MIDI0;
      else
        info->format = ExportFormat::EXPORT_FORMAT_MIDI1;
      g_settings_set_int (s, "format", (int) idx);
    }

  if (is_audio)
    {
      BitDepth depth =
        (BitDepth) adw_combo_row_get_selected (self->audio_bit_depth);
      info->depth = depth;
      g_settings_set_int (
        S_EXPORT_AUDIO, "bit-depth", audio_bit_depth_enum_to_int (depth));

      info->dither = gtk_switch_get_active (self->audio_dither_switch);
      g_settings_set_boolean (s, "dither", info->dither);
    }

  if (!is_audio)
    {
      info->lanes_as_tracks =
        gtk_switch_get_active (self->midi_export_lanes_as_tracks_switch);
      g_settings_set_boolean (s, "lanes-as-tracks", info->lanes_as_tracks);
    }

  /* mixdown/stems */
  if (is_audio)
    {
      g_settings_set_boolean (
        S_EXPORT_AUDIO, "export-stems",
        (bool) adw_combo_row_get_selected (self->audio_mixdown_or_stems));
    }
  else
    {
      g_settings_set_boolean (
        S_EXPORT_MIDI, "export-stems",
        (bool) adw_combo_row_get_selected (self->midi_mixdown_or_stems));
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

  info->artist = g_strdup (gtk_editable_get_text (GTK_EDITABLE (artist)));
  info->title = g_strdup (gtk_editable_get_text (GTK_EDITABLE (title)));
  info->genre = g_strdup (gtk_editable_get_text (GTK_EDITABLE (genre)));
  g_settings_set_string (s, "artist", info->artist);
  g_settings_set_string (s, "title", info->title);
  g_settings_set_string (s, "genre", info->genre);

  AdwComboRow * time_range_combo =
    is_audio ? self->audio_time_range_combo : self->midi_time_range_combo;
  info->time_range =
    (ExportTimeRange) adw_combo_row_get_selected (time_range_combo);
  g_settings_set_enum (s, "time-range", ENUM_VALUE_TO_INT (info->time_range));

  info->custom_start =
    is_audio ? self->audio_custom_start_pos : self->midi_custom_start_pos;
  info->custom_end =
    is_audio ? self->audio_custom_end_pos : self->midi_custom_end_pos;

  info->file_uri = get_export_filename (self, true, track);

  info->bounce_with_parents = true;

  info->mode = ExportMode::EXPORT_MODE_TRACKS;

  return info;
}

static void
progress_close_cb (ExportData * data)
{
  g_debug ("export ready cb");

  ExportDialogWidget * self = Z_EXPORT_DIALOG_WIDGET (data->parent_owner);

  g_thread_join (data->thread);

  if (data->export_stems)
    {
      /* re-connect disconnected connections */
      exporter_post_export (data->info, data->conns, data->state);

      Track * track = static_cast<Track *> (
        g_ptr_array_index (data->tracks, data->cur_track));
      track->bounce = false;

      /* stop if last stem not successful */
      ExportSettings * prev_info = data->info;
      if (progress_info_get_status (prev_info->progress_info) == PROGRESS_STATUS_COMPLETED && (progress_info_get_completion_type (prev_info->progress_info) == PROGRESS_COMPLETED_SUCCESS || progress_info_get_completion_type (prev_info->progress_info) == PROGRESS_COMPLETED_HAS_WARNING))
        {
          g_debug ("~ finished bouncing stem for %s ~", track->name);

          data->cur_track++;
          if (data->cur_track < data->tracks->len)
            {
              /* bounce the next track */
              track = static_cast<Track *> (
                g_ptr_array_index (data->tracks, data->cur_track));
              g_debug ("~ bouncing stem for %s ~", track->name);

              /* unmark all tracks for bounce */
              tracklist_mark_all_tracks_for_bounce (TRACKLIST, false);

              track_mark_for_bounce (
                track, F_BOUNCE, F_MARK_REGIONS, F_MARK_CHILDREN,
                F_MARK_PARENTS);

              ExportSettings * new_info = init_export_info (self, track);
              ExportData *     new_data =
                export_data_new (GTK_WIDGET (self), new_info);
              new_data->export_stems = true;
              new_data->tracks = g_ptr_array_copy (data->tracks, NULL, NULL);
              new_data->cur_track = data->cur_track;

              new_data->conns = exporter_prepare_tracks_for_export (
                new_data->info, new_data->state);

              g_message ("exporting %s", new_data->info->file_uri);

              /* start exporting in a new thread */
              new_data->thread = g_thread_new (
                "stem_export_thread",
                (GThreadFunc) exporter_generic_export_thread, new_data->info);

              /* create a progress dialog and show */
              /* don't use autoclose - instead force close in the callback
               * (couldn't figure out how to get autoclose working - the
               * callback wasn't getting called otherwise so just autoclose from
               * the callback instead) */
              ExportProgressDialogWidget * progress_dialog =
                export_progress_dialog_widget_new (
                  new_data, true, progress_close_cb, true, F_CANCELABLE);
              adw_dialog_present (
                ADW_DIALOG (progress_dialog), GTK_WIDGET (self));
              return;
            }
        } /*endif prev export completed */
    }
  else
    {
      /* re-connect disconnected connections */
      exporter_post_export (data->info, data->conns, data->state);

      g_debug ("~ finished bouncing mixdown ~");
    }

  ui_show_notification (_ ("Exported"));

  update_text (self);
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
  GtkColumnView * list_view =
    audio ? self->audio_tracks_view : self->midi_tracks_view;
  bool export_stems = (bool) adw_combo_row_get_selected (
    audio ? self->audio_mixdown_or_stems : self->midi_mixdown_or_stems);

  GPtrArray * tracks = g_ptr_array_new ();
  get_enabled_tracks (self, list_view, tracks);

  if (tracks->len == 0)
    {
      ui_show_error_message (_ ("Export Failed"), _ ("No tracks to export"));
      return;
    }

  /* make exports dir if not there yet */
  char *   exports_dir = get_exports_dir (self);
  GError * err = NULL;
  bool     success = io_mkdir (exports_dir, &err);
  if (!success)
    {
      ui_show_error_message (
        _ ("Export Failed"), "Failed to create exports directory");
      return;
    }
  g_free (exports_dir);

  /* begin export */

  if (export_stems)
    {
      /* export the first track for now */
      Track * track = static_cast<Track *> (g_ptr_array_index (tracks, 0));
      ExportSettings * info = init_export_info (self, track);
      ExportData *     data = export_data_new (GTK_WIDGET (self), info);
      data->export_stems = export_stems;
      data->tracks = tracks;
      data->cur_track = 0;

      g_debug ("~ bouncing stem for %s ~", track->name);

      /* unmark all tracks for bounce */
      tracklist_mark_all_tracks_for_bounce (TRACKLIST, false);

      track_mark_for_bounce (
        track, F_BOUNCE, F_MARK_REGIONS, F_MARK_CHILDREN, F_MARK_PARENTS);

      data->conns = exporter_prepare_tracks_for_export (data->info, data->state);

      g_message ("exporting %s", data->info->file_uri);

      /* start exporting in a new thread */
      data->thread = g_thread_new (
        "export_thread", (GThreadFunc) exporter_generic_export_thread,
        data->info);

      /* create a progress dialog and show */
      /* don't use autoclose - instead force close in the callback (couldn't
       * figure out how to get autoclose working - the callback wasn't getting
       * called otherwise so just autoclose from the callback instead) */
      ExportProgressDialogWidget * progress_dialog =
        export_progress_dialog_widget_new (
          data, true, progress_close_cb, true, F_CANCELABLE);
      adw_dialog_present (ADW_DIALOG (progress_dialog), GTK_WIDGET (self));
    }
  else /* if exporting mixdown */
    {
      g_debug ("~ bouncing mixdown ~");

      ExportSettings * info = init_export_info (self, NULL);
      ExportData *     data = export_data_new (GTK_WIDGET (self), info);
      data->export_stems = false;
      data->tracks = tracks;

      /* unmark all tracks for bounce */
      tracklist_mark_all_tracks_for_bounce (TRACKLIST, false);

      /* mark all checked tracks for bounce */
      for (size_t i = 0; i < tracks->len; i++)
        {
          Track * track = static_cast<Track *> (g_ptr_array_index (tracks, i));
          track_mark_for_bounce (
            track, F_BOUNCE, F_MARK_REGIONS, F_NO_MARK_CHILDREN, F_MARK_PARENTS);
        }

      g_message ("exporting %s", data->info->file_uri);

      data->conns = exporter_prepare_tracks_for_export (data->info, data->state);

      /* start exporting in a new thread */
      data->thread = g_thread_new (
        "export_thread", (GThreadFunc) exporter_generic_export_thread,
        data->info);

      /* create a progress dialog and show */
      /* don't use autoclose - instead force close in the callback (couldn't
       * figure out how to get autoclose working - the callback wasn't getting
       * called otherwise so just autoclose from the callback instead) */
      ExportProgressDialogWidget * progress_dialog =
        export_progress_dialog_widget_new (
          data, false, progress_close_cb, true, F_CANCELABLE);
      adw_dialog_present (ADW_DIALOG (progress_dialog), GTK_WIDGET (self));
    }
}

static void
on_response (GtkDialog * dialog, gint response_id, gpointer user_data)
{
  ExportDialogWidget * self = Z_EXPORT_DIALOG_WIDGET (user_data);

  if (response_id == GTK_RESPONSE_ACCEPT)
    {
      on_export (self, AUDIO_STACK_VISIBLE (self));
      return;
    }

  gtk_window_destroy (GTK_WINDOW (dialog));
}

/**
 * This toggles on all parents recursively if
 * something is toggled.
 */
static void
set_track_toggle_on_parent_recursively (
  ExportDialogWidget *            self,
  WrappedObjectWithChangeSignal * wobj,
  bool                            toggled)
{
  if (!toggled)
    {
      return;
    }

  Track * track = (Track *) wobj->obj;

  /* enable the parent if toggled */
  Track * direct_out = track->channel->get_output_track ();
  if (!direct_out)
    return;

  g_debug (
    "%s: setting toggle %d on parent %s recursively", track->name, toggled,
    direct_out->name);

  for (guint i = 0; i < g_list_model_get_n_items (wobj->parent_model); i++)
    {
      WrappedObjectWithChangeSignal * parent_wobj =
        Z_WRAPPED_OBJECT_WITH_CHANGE_SIGNAL (
          g_list_model_get_item (wobj->parent_model, i));
      Track * parent_track = (Track *) parent_wobj->obj;
      if (parent_track == direct_out)
        {
          g_object_set_data (
            G_OBJECT (parent_wobj), "checked", GINT_TO_POINTER (toggled));
          wrapped_object_with_change_signal_fire (parent_wobj);

          set_track_toggle_on_parent_recursively (self, parent_wobj, toggled);
        }
    }
}

static void
set_track_toggle_recursively (
  ExportDialogWidget *            self,
  WrappedObjectWithChangeSignal * wobj,
  bool                            toggled)
{
  if (!wobj->child_model)
    return;

  Track * track = (Track *) wobj->obj;
  g_debug (
    "%s: setting toggle %d on children recursively", track->name, toggled);

  for (guint i = 0; i < g_list_model_get_n_items (wobj->child_model); i++)
    {
      WrappedObjectWithChangeSignal * child_wobj =
        Z_WRAPPED_OBJECT_WITH_CHANGE_SIGNAL (
          g_list_model_get_item (wobj->child_model, i));
      /*Track * child_track = (Track *) child_wobj->obj;*/
      g_object_set_data (
        G_OBJECT (child_wobj), "checked", GINT_TO_POINTER (toggled));
      wrapped_object_with_change_signal_fire (child_wobj);
      set_track_toggle_recursively (self, child_wobj, toggled);
    }
}

static void
on_track_toggled (GtkCheckButton * check_btn, ExportDialogWidget * self)
{
  WrappedObjectWithChangeSignal * wobj = Z_WRAPPED_OBJECT_WITH_CHANGE_SIGNAL (
    g_object_get_data (G_OBJECT (check_btn), "wobj"));
  Track * track = (Track *) wobj->obj;

  /* get toggled */
  gboolean toggled = gtk_check_button_get_active (check_btn);
  g_object_set_data (G_OBJECT (wobj), "checked", GINT_TO_POINTER (toggled));
  g_debug ("%s track %s", toggled ? "toggled" : "untoggled", track->name);

  /* if exporting mixdown (single file) */
  bool is_audio = AUDIO_STACK_VISIBLE (self);
  bool export_stems = (bool) adw_combo_row_get_selected (
    is_audio ? self->audio_mixdown_or_stems : self->midi_mixdown_or_stems);
  if (!export_stems)
    {
      /* toggle parents if toggled */
      set_track_toggle_on_parent_recursively (self, wobj, toggled);

      /* propagate value to children recursively */
      set_track_toggle_recursively (self, wobj, toggled);
    }

  update_text (self);
}

static void
add_group_track_children (
  ExportDialogWidget * self,
  GListModel *         parent_store,
  GListModel *         store,
  Track *              track)
{
  /* add the group */
  WrappedObjectWithChangeSignal * wobj = wrapped_object_with_change_signal_new (
    track, WrappedObjectType::WRAPPED_OBJECT_TYPE_TRACK);
  g_object_set_data (G_OBJECT (wobj), "checked", GINT_TO_POINTER (true));
  wobj->parent_model = G_LIST_MODEL (parent_store);
  g_list_store_append (G_LIST_STORE (store), wobj);

  g_debug ("%s: track '%s'", __func__, track->name);

  if (track->num_children == 0)
    return;

  /* add the children */
  wobj->child_model =
    G_LIST_MODEL (g_list_store_new (WRAPPED_OBJECT_WITH_CHANGE_SIGNAL_TYPE));
  for (int i = 0; i < track->num_children; i++)
    {
      Track * child =
        tracklist_find_track_by_name_hash (TRACKLIST, track->children[i]);
      g_return_if_fail (IS_TRACK_AND_NONNULL (child));

      g_debug ("child: '%s'", child->name);
      add_group_track_children (self, store, wobj->child_model, child);
    }
}

static GListModel *
get_child_model (GObject * item, gpointer user_data)
{
  WrappedObjectWithChangeSignal * wobj =
    Z_WRAPPED_OBJECT_WITH_CHANGE_SIGNAL (item);
  if (wobj->child_model)
    return g_object_ref (G_LIST_MODEL (wobj->child_model));

  return NULL;
}

static void
on_obj_changed (GObject * obj, GtkCheckButton * check_btn)
{
  g_signal_handlers_block_matched (
    check_btn, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_track_toggled,
    NULL);
  gtk_check_button_set_active (
    check_btn, GPOINTER_TO_INT (g_object_get_data (obj, "checked")));
  g_signal_handlers_unblock_matched (
    check_btn, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_track_toggled,
    NULL);
}

static void
checked_factory_setup_cb (
  GtkSignalListItemFactory * factory,
  GtkListItem *              listitem,
  ExportDialogWidget *       self)
{
  GtkWidget * check_btn = gtk_check_button_new ();
  gtk_check_button_set_active (GTK_CHECK_BUTTON (check_btn), true);
  GtkTreeExpander * expander = GTK_TREE_EXPANDER (gtk_tree_expander_new ());
  gtk_tree_expander_set_child (expander, GTK_WIDGET (check_btn));
  gtk_list_item_set_focusable (listitem, false);
  gtk_list_item_set_child (listitem, GTK_WIDGET (expander));
}

static void
checked_factory_bind_cb (
  GtkSignalListItemFactory * factory,
  GtkListItem *              listitem,
  ExportDialogWidget *       self)
{
  GtkTreeListRow *  row = GTK_TREE_LIST_ROW (gtk_list_item_get_item (listitem));
  GtkTreeExpander * expander =
    GTK_TREE_EXPANDER (gtk_list_item_get_child (listitem));
  gtk_tree_expander_set_list_row (expander, row);
  WrappedObjectWithChangeSignal * obj =
    Z_WRAPPED_OBJECT_WITH_CHANGE_SIGNAL (gtk_tree_list_row_get_item (row));
  GtkWidget * check_btn = gtk_tree_expander_get_child (expander);
  gtk_check_button_set_active (
    GTK_CHECK_BUTTON (check_btn),
    GPOINTER_TO_INT (g_object_get_data (G_OBJECT (obj), "checked")));
  g_object_set_data (G_OBJECT (check_btn), "wobj", obj);
  g_signal_connect (
    G_OBJECT (check_btn), "toggled", G_CALLBACK (on_track_toggled), self);
  g_signal_connect (
    G_OBJECT (obj), "changed", G_CALLBACK (on_obj_changed), check_btn);
}

static void
checked_factory_unbind_cb (
  GtkSignalListItemFactory * factory,
  GtkListItem *              listitem,
  ExportDialogWidget *       self)
{
  GtkTreeListRow *  row = GTK_TREE_LIST_ROW (gtk_list_item_get_item (listitem));
  GtkTreeExpander * expander =
    GTK_TREE_EXPANDER (gtk_list_item_get_child (listitem));
  gtk_tree_expander_set_list_row (expander, NULL);
  WrappedObjectWithChangeSignal * obj =
    Z_WRAPPED_OBJECT_WITH_CHANGE_SIGNAL (gtk_tree_list_row_get_item (row));
  GtkWidget * check_btn = gtk_tree_expander_get_child (expander);
  g_signal_handlers_disconnect_by_func (
    check_btn, (gpointer) on_track_toggled, self);
  g_signal_handlers_disconnect_by_func (
    obj, (gpointer) on_obj_changed, check_btn);
}

static void
checked_factory_teardown_cb (
  GtkSignalListItemFactory * factory,
  GtkListItem *              listitem,
  ExportDialogWidget *       self)
{
  gtk_list_item_set_child (listitem, NULL);
}

static void
setup_tracks_treeview (ExportDialogWidget * self, bool is_audio)
{
  GtkColumnView * view =
    is_audio ? self->audio_tracks_view : self->midi_tracks_view;
  GPtrArray * item_factories =
    is_audio ? self->audio_item_factories : self->midi_item_factories;

  GListStore * store = g_list_store_new (WRAPPED_OBJECT_WITH_CHANGE_SIGNAL_TYPE);
  add_group_track_children (self, NULL, G_LIST_MODEL (store), P_MASTER_TRACK);
  GtkTreeListModel * tree_model = gtk_tree_list_model_new (
    G_LIST_MODEL (store), false, true,
    (GtkTreeListModelCreateModelFunc) get_child_model, self, NULL);
  GtkSelectionModel * sel_model =
    GTK_SELECTION_MODEL (gtk_no_selection_new (G_LIST_MODEL (tree_model)));
  gtk_column_view_set_model (view, sel_model);

  GtkListItemFactory * checked_factory = gtk_signal_list_item_factory_new ();
  g_signal_connect (
    G_OBJECT (checked_factory), "setup", G_CALLBACK (checked_factory_setup_cb),
    self);
  g_signal_connect (
    G_OBJECT (checked_factory), "bind", G_CALLBACK (checked_factory_bind_cb),
    self);
  g_signal_connect (
    G_OBJECT (checked_factory), "unbind",
    G_CALLBACK (checked_factory_unbind_cb), self);
  g_signal_connect (
    G_OBJECT (checked_factory), "teardown",
    G_CALLBACK (checked_factory_teardown_cb), self);
  GtkColumnViewColumn * column =
    gtk_column_view_column_new (_ ("Export"), checked_factory);
  gtk_column_view_column_set_resizable (column, true);
  gtk_column_view_column_set_expand (column, true);
  gtk_column_view_append_column (view, column);

  item_factory_generate_and_append_column (
    view, item_factories, ItemFactoryType::ITEM_FACTORY_COLOR, Z_F_NOT_EDITABLE,
    Z_F_NOT_RESIZABLE, NULL, _ ("Color"));

  item_factory_generate_and_append_column (
    view, item_factories, ItemFactoryType::ITEM_FACTORY_ICON, Z_F_NOT_EDITABLE,
    Z_F_NOT_RESIZABLE, NULL, _ ("Icon"));

  item_factory_generate_and_append_column (
    view, item_factories, ItemFactoryType::ITEM_FACTORY_TEXT, Z_F_NOT_EDITABLE,
    Z_F_NOT_RESIZABLE, NULL, _ ("Name"));

  gtk_column_view_set_tab_behavior (view, GTK_LIST_TAB_CELL);

  /* TODO hide headers */
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
  GtkStringList * string_list = gtk_string_list_new (strings);

  adw_combo_row_set_model (combo_row, G_LIST_MODEL (string_list));

  GSettings * s = is_audio ? S_EXPORT_AUDIO : S_EXPORT_MIDI;
  adw_combo_row_set_selected (
    combo_row, (guint) g_settings_get_enum (s, "filename-pattern"));

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
  GtkStringList * string_list = gtk_string_list_new (strings);

  adw_combo_row_set_model (combo_row, G_LIST_MODEL (string_list));

  adw_combo_row_set_selected (
    combo_row,
    (guint) g_settings_get_boolean (
      is_audio ? S_EXPORT_AUDIO : S_EXPORT_MIDI, "export-stems"));

  g_signal_connect (
    G_OBJECT (combo_row), "notify::selected-item",
    G_CALLBACK (on_mixdown_stem_selection_changed), self);
}

static void
on_time_range_changed (GObject * gobject, GParamSpec * pspec, gpointer user_data)
{
  ExportDialogWidget * self = Z_EXPORT_DIALOG_WIDGET (user_data);
  AdwComboRow *        combo_row = ADW_COMBO_ROW (gobject);
  bool                 is_audio = (combo_row == self->audio_time_range_combo);
  ExportTimeRange      time_range =
    ENUM_INT_TO_VALUE (ExportTimeRange, adw_combo_row_get_selected (combo_row));
  g_debug (
    "time range selected changed (is audio? %d): %s", is_audio,
    export_time_range_to_str (time_range));
  gtk_widget_set_visible (
    GTK_WIDGET (is_audio ? self->audio_custom_tr_row : self->midi_custom_tr_row),
    time_range == ExportTimeRange::TIME_RANGE_CUSTOM);
}

static void
setup_time_range_combo_row (
  ExportDialogWidget * self,
  AdwComboRow *        combo_row,
  bool                 is_audio)
{
  const char * strings[] = {
    C_ ("Export time range", "Loop"),
    C_ ("Export time range", "Song"),
    C_ ("Export time range", "Custom"),
    NULL,
  };
  GtkStringList * string_list = gtk_string_list_new (strings);

  g_debug ("setting up time range combo row (is audio ? %d)", is_audio);

  adw_combo_row_set_model (combo_row, G_LIST_MODEL (string_list));

  g_signal_connect (
    G_OBJECT (combo_row), "notify::selected",
    G_CALLBACK (on_time_range_changed), self);

  ExportTimeRange tr = ENUM_INT_TO_VALUE (
    ExportTimeRange,
    g_settings_get_enum (
      is_audio ? S_EXPORT_AUDIO : S_EXPORT_MIDI, "time-range"));
  adw_combo_row_set_selected (combo_row, (guint) tr);
  on_time_range_changed (G_OBJECT (combo_row), NULL, self);
}

static void
get_pos (Position * own_pos_ptr, Position * pos)
{
  position_set_to_pos (pos, own_pos_ptr);
}

static void
set_pos (Position * own_pos_ptr, Position * pos)
{
  position_set_to_pos (own_pos_ptr, pos);
}

static void
setup_custom_time_range_row (ExportDialogWidget * self, bool is_audio)
{
  DigitalMeterWidget * startm = digital_meter_widget_new_for_position (
    is_audio ? &self->audio_custom_start_pos : &self->midi_custom_start_pos,
    NULL, get_pos, set_pos, NULL, _ ("Start"));
  gtk_box_append (
    is_audio
      ? self->audio_custom_tr_start_meter_box
      : self->midi_custom_tr_start_meter_box,
    GTK_WIDGET (startm));
  DigitalMeterWidget * endm = digital_meter_widget_new_for_position (
    is_audio ? &self->audio_custom_end_pos : &self->midi_custom_end_pos, NULL,
    get_pos, set_pos, NULL, _ ("End"));
  gtk_box_append (
    is_audio
      ? self->audio_custom_tr_end_meter_box
      : self->midi_custom_tr_end_meter_box,
    GTK_WIDGET (endm));
}

static void
setup_dither (AdwActionRow * dither_row, GtkSwitch * dither_switch)
{
  gtk_switch_set_active (
    dither_switch, g_settings_get_boolean (S_EXPORT_AUDIO, "dither"));

  char * descr = settings_get_description (S_EXPORT_AUDIO, "dither");
  adw_action_row_set_subtitle (dither_row, descr);
  g_free (descr);
}

/**
 * Creates a new export dialog.
 */
ExportDialogWidget *
export_dialog_widget_new (void)
{
  ExportDialogWidget * self = static_cast<ExportDialogWidget *> (
    g_object_new (EXPORT_DIALOG_WIDGET_TYPE, NULL));

  update_text (self);

  return self;
}

static void
export_dialog_finalize (ExportDialogWidget * self)
{
  g_ptr_array_unref (self->midi_item_factories);
  g_ptr_array_unref (self->audio_item_factories);

  G_OBJECT_CLASS (export_dialog_widget_parent_class)->finalize (G_OBJECT (self));
}

static void
export_dialog_widget_class_init (ExportDialogWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (klass, "export_dialog.ui");

#define BIND_CHILD(x) \
  gtk_widget_class_bind_template_child (klass, ExportDialogWidget, x)

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
  BIND_CHILD (audio_time_range_combo);
  BIND_CHILD (audio_custom_tr_row);
  BIND_CHILD (audio_custom_tr_start_meter_box);
  BIND_CHILD (audio_custom_tr_end_meter_box);
  BIND_CHILD (audio_tracks_view);
  BIND_CHILD (audio_output_label);
  BIND_CHILD (midi_title);
  BIND_CHILD (midi_artist);
  BIND_CHILD (midi_genre);
  BIND_CHILD (midi_format);
  BIND_CHILD (midi_export_lanes_as_tracks_switch);
  BIND_CHILD (midi_filename_pattern);
  BIND_CHILD (midi_mixdown_or_stems);
  BIND_CHILD (midi_time_range_combo);
  BIND_CHILD (midi_custom_tr_row);
  BIND_CHILD (midi_custom_tr_start_meter_box);
  BIND_CHILD (midi_custom_tr_end_meter_box);
  BIND_CHILD (midi_tracks_view);
  BIND_CHILD (midi_output_label);

#undef BIND_CHILD

  GObjectClass * oklass = G_OBJECT_CLASS (klass);
  oklass->finalize = (GObjectFinalizeFunc) export_dialog_finalize;
}

static void
export_dialog_widget_init (ExportDialogWidget * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  self->audio_item_factories =
    g_ptr_array_new_with_free_func (item_factory_free_func);
  self->midi_item_factories =
    g_ptr_array_new_with_free_func (item_factory_free_func);

  gtk_dialog_add_button (GTK_DIALOG (self), _ ("_Cancel"), GTK_RESPONSE_REJECT);
  gtk_dialog_add_button (GTK_DIALOG (self), _ ("_Export"), GTK_RESPONSE_ACCEPT);
  gtk_dialog_set_default_response (GTK_DIALOG (self), GTK_RESPONSE_ACCEPT);

  /* --- audio --- */

  /* metadata */
  gtk_editable_set_text (
    GTK_EDITABLE (self->audio_title),
    g_settings_get_string (S_EXPORT_AUDIO, "title"));
  gtk_editable_set_text (
    GTK_EDITABLE (self->audio_artist),
    g_settings_get_string (S_EXPORT_AUDIO, "artist"));
  gtk_editable_set_text (
    GTK_EDITABLE (self->audio_genre),
    g_settings_get_string (S_EXPORT_AUDIO, "genre"));

  /* options */
  setup_audio_formats_dropdown (self->audio_format);
  setup_dither (self->audio_dither, self->audio_dither_switch);
  setup_bit_depth_drop_down (self->audio_bit_depth);
  setup_filename_pattern_combo_row (self, self->audio_filename_pattern, true);
  setup_mixdown_or_stems_combo_row (self, self->audio_mixdown_or_stems, true);

  /* selections */
  setup_time_range_combo_row (self, self->audio_time_range_combo, true);
  setup_custom_time_range_row (self, true);
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
    self->midi_format, (guint) g_settings_get_int (S_EXPORT_MIDI, "format"));
  gtk_switch_set_active (
    self->midi_export_lanes_as_tracks_switch,
    g_settings_get_boolean (S_EXPORT_MIDI, "lanes-as-tracks"));
  setup_filename_pattern_combo_row (self, self->midi_filename_pattern, false);
  setup_mixdown_or_stems_combo_row (self, self->midi_mixdown_or_stems, false);

  /* selections */
  setup_time_range_combo_row (self, self->midi_time_range_combo, false);
  setup_custom_time_range_row (self, false);
  setup_tracks_treeview (self, false);

  /* --- end MIDI --- */

  g_signal_connect (
    G_OBJECT (self->stack), "notify::visible-child",
    G_CALLBACK (on_visible_child_changed), self);

  g_signal_connect (G_OBJECT (self), "response", G_CALLBACK (on_response), self);
}
