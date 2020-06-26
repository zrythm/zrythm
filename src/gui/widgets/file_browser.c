/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
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

#include "actions/create_tracks_action.h"
#include "audio/supported_file.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/file_browser.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/mixer.h"
#include "gui/widgets/right_dock_edge.h"
#include "gui/widgets/timeline_arranger.h"
#include "gui/widgets/tracklist.h"
#include "plugins/plugin.h"
#include "plugins/plugin_manager.h"
#include "project.h"
#include "settings/settings.h"
#include "utils/io.h"
#include "utils/resources.h"
#include "zrythm.h"
#include "zrythm_app.h"

#include <audec/audec.h>

#include <gtk/gtk.h>

#include <sndfile.h>
#include <samplerate.h>

G_DEFINE_TYPE (FileBrowserWidget,
               file_browser_widget,
               GTK_TYPE_PANED)

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
  gpointer user_data)
{
  char * label = (char *) user_data;

  gtk_label_set_text (self->file_info, label);

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
  char * abs_path =
    gtk_file_chooser_get_filename (chooser);

  SupportedFile * file =
    supported_file_new_from_path (abs_path);

  g_message ("activated file type %d, abs path %s",
             file->type,
             abs_path);

  UndoableAction * ua = NULL;
  switch (file->type)
    {
    case FILE_TYPE_WAV:
    case FILE_TYPE_OGG:
    case FILE_TYPE_FLAC:
    case FILE_TYPE_MP3:
      ua =
        create_tracks_action_new (
          TRACK_TYPE_AUDIO, NULL, file,
          TRACKLIST->num_tracks, PLAYHEAD, 1);
      undo_manager_perform (UNDO_MANAGER, ua);
      break;
    case FILE_TYPE_MIDI:
      ua =
        create_tracks_action_new (
          TRACK_TYPE_MIDI, NULL, file,
          TRACKLIST->num_tracks,
          PLAYHEAD,
          /* the number of tracks
           * to create depends on the MIDI file */
          -1);
      undo_manager_perform (UNDO_MANAGER, ua);
      break;
    default:
      break;
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
  char * abs_path =
    gtk_file_chooser_get_filename (chooser);

  if (!abs_path)
    return;

  SupportedFile * file =
    supported_file_new_from_path (abs_path);

  char * label;
  char * descr =
    supported_file_type_get_description (
      file->type);
  if (file->type == FILE_TYPE_MP3 ||
      file->type == FILE_TYPE_FLAC ||
      file->type == FILE_TYPE_OGG ||
      file->type == FILE_TYPE_WAV)
    {
      /* open with sndfile */
      AudecInfo nfo;
      audec_finfo (abs_path, &nfo);
      audec_dump_info (3, &nfo);
      label =
        g_strdup_printf (
        "%s\nFormat: TODO\nSample rate: %d\n"
        "Channels:%d Bitrate: %d\nBit depth: %d",
        descr,
        nfo.sample_rate,
        nfo.channels,
        nfo.bit_rate,
        nfo.bit_depth);
    }
  else
    {
      label =
        g_strdup_printf (
        "%s\nType: %s",
        descr,
        descr);
    }
  g_free (descr);
  update_file_info_label (self,
                          label);

  g_free (abs_path);
  supported_file_free (file);
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

static void
on_position_change (
  GtkStack * stack,
  GParamSpec * pspec,
  FileBrowserWidget * self)
{
  int divider_pos;
  if (!self->start_saving_pos)
    return;

  gint64 curr_time = g_get_monotonic_time ();

  if (self->first_time_position_set ||
      curr_time -
        self->first_time_position_set_time < 400000)
    {
      /* get divider position */
      divider_pos =
        g_settings_get_int (
          S_UI,
          "browser-divider-position");
      gtk_paned_set_position (
        GTK_PANED (self),
        divider_pos);

      self->first_time_position_set = 0;
      /*g_message ("***************************got file position %d",*/
                 /*divider_pos);*/
    }
  else
    {
      /* save the divide position */
      divider_pos =
        gtk_paned_get_position (
          GTK_PANED (self));
      g_settings_set_int (
        S_UI,
        "browser-divider-position",
        divider_pos);
      /*g_message ("***************************set file position to %d",*/
                 /*divider_pos);*/
    }
}

FileBrowserWidget *
file_browser_widget_new ()
{
  FileBrowserWidget * self =
    g_object_new (FILE_BROWSER_WIDGET_TYPE, NULL);

  g_message ("Instantiating file_browser widget...");

  gtk_label_set_xalign (self->file_info, 0);

  self->file_chooser =
    GTK_FILE_CHOOSER_WIDGET (
      gtk_file_chooser_widget_new (
        GTK_FILE_CHOOSER_ACTION_OPEN));
  gtk_widget_set_visible (
    GTK_WIDGET (self->file_chooser), 1);
  gtk_file_chooser_set_local_only (
    GTK_FILE_CHOOSER (self->file_chooser), 0);
  gtk_box_pack_start (
    GTK_BOX (self->browser_bot),
    GTK_WIDGET (self->file_chooser),
    1, 1, 0);
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

  gtk_widget_add_events (
    GTK_WIDGET (self), GDK_STRUCTURE_MASK);

  g_signal_connect (
    G_OBJECT (self), "notify::position",
    G_CALLBACK (on_position_change), self);
  g_signal_connect (
    G_OBJECT (self), "map-event",
    G_CALLBACK (on_map_event), self);

  return self;
}

static void
file_browser_widget_class_init (FileBrowserWidgetClass * _klass)
{
  GtkWidgetClass * klass =
    GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (
    klass,
    "file_browser.ui");

  gtk_widget_class_set_css_name (
    klass, "browser");

#define BIND_CHILD(x) \
  gtk_widget_class_bind_template_child ( \
    klass, \
    FileBrowserWidget, \
    x)

  BIND_CHILD (browser_top);
  BIND_CHILD (browser_search);
  BIND_CHILD (collections_exp);
  BIND_CHILD (types_exp);
  BIND_CHILD (locations_exp);
  BIND_CHILD (browser_bot);
  BIND_CHILD (file_info);

#undef BIND_CHILD
}

static void
file_browser_widget_init (FileBrowserWidget * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}
