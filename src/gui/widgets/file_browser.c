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

#include "zrythm-config.h"

#include "actions/tracklist_selections.h"
#include "audio/supported_file.h"
#include "gui/backend/file_manager.h"
#include "gui/backend/wrapped_object_with_change_signal.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/file_auditioner_controls.h"
#include "gui/widgets/file_browser.h"
#include "gui/widgets/file_browser_filters.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/mixer.h"
#include "gui/widgets/right_dock_edge.h"
#include "gui/widgets/timeline_arranger.h"
#include "gui/widgets/tracklist.h"
#include "plugins/plugin.h"
#include "plugins/plugin_manager.h"
#include "project.h"
#include "settings/settings.h"
#include "utils/error.h"
#include "utils/gtk.h"
#include "utils/io.h"
#include "utils/objects.h"
#include "utils/resources.h"
#include "zrythm.h"
#include "zrythm_app.h"

#include <audec/audec.h>

#include <gtk/gtk.h>
#include <glib/gi18n.h>

G_DEFINE_TYPE (
  FileBrowserWidget, file_browser_widget,
  GTK_TYPE_BOX)

enum
{
  COLUMN_ICON,
  COLUMN_NAME,
  COLUMN_DESCR,
  NUM_COLUMNS
};

static int
update_file_info_label (
  FileBrowserWidget * self,
  const char *        label)
{
  gtk_label_set_markup (
    self->file_info,
    label ? label : _("No file selected"));

  return G_SOURCE_REMOVE;
}

/**
 * Called when a file is double clicked or enter is
 * pressed.
 */
static void
on_file_chooser_file_activated (
  GtkFileChooser *chooser,
  FileBrowserWidget * self)
{
  GFile * gfile =
    gtk_file_chooser_get_file (chooser);
  char * abs_path = g_file_get_path (gfile);
  g_object_unref (gfile);

  SupportedFile * file =
    supported_file_new_from_path (abs_path);

  g_message (
    "activated file type %d, abs path %s",
    file->type, abs_path);

  GError * err = NULL;
  bool ret = true;
  switch (file->type)
    {
    case FILE_TYPE_WAV:
    case FILE_TYPE_OGG:
    case FILE_TYPE_FLAC:
    case FILE_TYPE_MP3:
      ret =
        tracklist_selections_action_perform_create (
          TRACK_TYPE_AUDIO, NULL, file,
          TRACKLIST->num_tracks, PLAYHEAD, 1, -1,
          &err);
      break;
    case FILE_TYPE_MIDI:
      ret =
        tracklist_selections_action_perform_create (
          TRACK_TYPE_MIDI, NULL, file,
          TRACKLIST->num_tracks,
          PLAYHEAD,
          /* the number of tracks
           * to create depends on the MIDI file */
          -1, -1, &err);
      break;
    default:
      break;
    }

  if (!ret)
    {
      HANDLE_ERROR (
        err,
        _("Failed to create track for file '%s'"),
        abs_path);
    }

  g_free (abs_path);
  supported_file_free (file);
}

/**
 * Called when the file selection changes.
 */
static void
on_file_chooser_selection_changed (
  GtkFileChooser *chooser,
  FileBrowserWidget * self)
{
  if (self->selected_file)
    {
      if (self->selected_file->obj)
        {
          supported_file_free (
            (SupportedFile *)
            self->selected_file->obj);
          self->selected_file->obj = NULL;
        }
      object_free_w_func_and_null (
        g_object_unref, self->selected_file);
    }

  sample_processor_stop_file_playback (
    SAMPLE_PROCESSOR);

  GFile * gfile =
    gtk_file_chooser_get_file (chooser);
  char * abs_path = g_file_get_path (gfile);
  g_object_unref (gfile);

  if (!abs_path)
    {
      update_file_info_label (self, NULL);
      return;
    }

  SupportedFile * file =
    supported_file_new_from_path (abs_path);
  WrappedObjectWithChangeSignal * wrapped_obj =
    wrapped_object_with_change_signal_new (
      file,
      WRAPPED_OBJECT_TYPE_SUPPORTED_FILE);

  char * label =
    supported_file_get_info_text_for_label (file);
  update_file_info_label (self, label);

  g_free (abs_path);

  if (g_settings_get_boolean (
        S_UI_FILE_BROWSER, "autoplay") &&
      supported_file_should_autoplay (file))
    {
      sample_processor_queue_file (
        SAMPLE_PROCESSOR, file);
    }

  self->selected_file = wrapped_obj;
}

static WrappedObjectWithChangeSignal *
get_selected_file (
  GtkWidget * widget)
{
  FileBrowserWidget * self =
    Z_FILE_BROWSER_WIDGET (widget);

  return self->selected_file;
}

#if 0
static bool
file_filter_func (
  const GtkFileFilterInfo * filter_info,
  FileBrowserWidget *       self)
{
  SupportedFile * descr =
    supported_file_new_from_uri (filter_info->uri);
  if (!descr)
    return false;

  bool show_audio =
    gtk_toggle_button_get_active (
      self->filters_toolbar->toggle_audio);
  bool show_midi =
    gtk_toggle_button_get_active (
      self->filters_toolbar->toggle_midi);
  bool show_presets =
    gtk_toggle_button_get_active (
      self->filters_toolbar->toggle_presets);
  bool all_toggles_off =
    !show_audio && !show_midi && !show_presets;

  bool visible = false;
  switch (descr->type)
    {
    case FILE_TYPE_MIDI:
      visible = show_midi || all_toggles_off;
      break;
    case FILE_TYPE_DIR:
    case FILE_TYPE_PARENT_DIR:
      supported_file_free (descr);
      return true;
    case FILE_TYPE_MP3:
    case FILE_TYPE_FLAC:
    case FILE_TYPE_OGG:
    case FILE_TYPE_WAV:
      visible = show_audio || all_toggles_off;
      break;
    case FILE_TYPE_OTHER:
      visible =
        all_toggles_off &&
        g_settings_get_boolean (
          S_UI_FILE_BROWSER,
          "show-unsupported-files");
      break;
    default:
      break;
    }

  if (!visible)
    {
      supported_file_free (descr);
      return false;
    }

  if (!g_settings_get_boolean (
         S_UI_FILE_BROWSER, "show-hidden-files") &&
      descr->hidden)
    {
      supported_file_free (descr);
      return false;
    }

  supported_file_free (descr);
  return visible;
}
#endif

static void
refilter_files (
  FileBrowserWidget * self)
{
  /* TODO use MIME-based filtering */
#if 0
  GtkFileFilter * filter =
    gtk_file_filter_new ();
  gtk_file_filter_add_custom (
    filter, GTK_FILE_FILTER_URI,
    (GtkFileFilterFunc) file_filter_func,
    self, NULL);
  gtk_file_chooser_set_filter (
    GTK_FILE_CHOOSER (self->file_chooser), filter);
#endif
}

static int
on_map_event (
  GtkStack * stack,
  GdkEvent * event,
  FileBrowserWidget * self)
{
  /*g_message ("FILE MAP EVENT");*/
  if (gtk_widget_get_mapped (
        GTK_WIDGET (self)))
    {
      self->start_saving_pos = 1;
      self->first_time_position_set_time =
        g_get_monotonic_time ();

      /* set divider position */
      int divider_pos =
        g_settings_get_int (
          S_UI,
          "browser-divider-position");
      gtk_paned_set_position (
        GTK_PANED (self), divider_pos);
      self->first_time_position_set = 1;
      g_message (
        "setting file browser divider pos to %d",
        divider_pos);
    }

  return FALSE;
}

FileBrowserWidget *
file_browser_widget_new ()
{
  FileBrowserWidget * self =
    g_object_new (FILE_BROWSER_WIDGET_TYPE, NULL);

  g_message ("Instantiating file_browser widget...");

  gtk_label_set_xalign (self->file_info, 0);

  /* create file chooser */
  self->file_chooser =
    GTK_FILE_CHOOSER_WIDGET (
      gtk_file_chooser_widget_new (
        GTK_FILE_CHOOSER_ACTION_OPEN));
  gtk_widget_set_visible (
    GTK_WIDGET (self->file_chooser), true);
  refilter_files (self);

  /* add bookmarks */
  for (size_t i = 0;
       i < FILE_MANAGER->locations->len; i++)
    {
      FileBrowserLocation * loc =
        (FileBrowserLocation *)
        g_ptr_array_index (
          FILE_MANAGER->locations, i);

      if (loc->special_location ==
            FILE_MANAGER_NONE)
        {
          GFile * gfile =
            g_file_new_for_path (loc->path);
          GError * err = NULL;
          bool ret =
            gtk_file_chooser_add_shortcut_folder (
              GTK_FILE_CHOOSER (self->file_chooser),
              gfile, &err);
          g_object_unref (gfile);
          if (!ret)
            {
              g_warning (
                "Failed to add shortcut folder "
                "'%s': %s",
                loc->path, err->message);
            }
        }
    }

  /* choose current location from settings */
  GFile * gfile =
    g_file_new_for_path (
      FILE_MANAGER->selection->path);
  gtk_file_chooser_set_file (
    GTK_FILE_CHOOSER (self->file_chooser), gfile,
    NULL);
  g_object_unref (gfile);

  gtk_box_append (
    GTK_BOX (self->file_chooser_box),
    GTK_WIDGET (self->file_chooser));
  g_signal_connect (
    G_OBJECT (self->file_chooser),
    "file-activated",
    G_CALLBACK (on_file_chooser_file_activated),
    self);
  g_signal_connect (
    G_OBJECT (self->file_chooser),
    "selection-changed",
    G_CALLBACK (on_file_chooser_selection_changed),
    self);

  file_auditioner_controls_widget_setup (
    self->auditioner_controls,
    GTK_WIDGET (self), true,
    get_selected_file,
    (GenericCallback) refilter_files);
  file_browser_filters_widget_setup (
    self->filters_toolbar,
    GTK_WIDGET (self),
    (GenericCallback) refilter_files);

  g_signal_connect (
    G_OBJECT (self), "map-event",
    G_CALLBACK (on_map_event), self);

  return self;
}

static void
file_browser_widget_class_init (
  FileBrowserWidgetClass * _klass)
{
  GtkWidgetClass * klass =
    GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (
    klass, "file_browser.ui");

#define BIND_CHILD(x) \
  gtk_widget_class_bind_template_child ( \
    klass, \
    FileBrowserWidget, \
    x)

  BIND_CHILD (browser_bot);
  BIND_CHILD (file_chooser_box);
  BIND_CHILD (auditioner_controls);
  BIND_CHILD (filters_toolbar);
  BIND_CHILD (file_info);

#undef BIND_CHILD
}

static void
file_browser_widget_init (FileBrowserWidget * self)
{
  g_type_ensure (
    FILE_AUDITIONER_CONTROLS_WIDGET_TYPE);
  g_type_ensure (
    FILE_BROWSER_FILTERS_WIDGET_TYPE);

  gtk_widget_init_template (GTK_WIDGET (self));

  gtk_widget_add_css_class (
    GTK_WIDGET (self), "file-browser");
}
