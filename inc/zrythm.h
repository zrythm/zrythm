/*
 * inc/zrythm_app.h - The GTK application
 *
 * Copyright (C) 2019 Alexandros Theodotou
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __ZRYTHM_H__
#define __ZRYTHM_H__

#include "audio/snap_grid.h"

#include <gtk/gtk.h>

#define ZRYTHM zrythm

#define ZRYTHM_APP_TYPE (zrythm_app_get_type ())
G_DECLARE_FINAL_TYPE (ZrythmApp,
                      zrythm_app,
                      ZRYTHM,
                      APP,
                      GtkApplication)

typedef struct AudioEngine AudioEngine;
typedef struct PluginManager PluginManager;
typedef struct Project Project;
typedef struct PianoRoll PianoRoll;
typedef struct Settings Settings;
typedef struct Tracklist Tracklist;
typedef struct UndoManager UndoManager;
typedef struct Quantize Quantize;
typedef struct _MainWindowWidget MainWindowWidget;
typedef struct Preferences Preferences;
typedef struct TimelineMinimap TimelineMinimap;

struct _ZrythmApp
{
  GtkApplication parent;

  /**
   * The audio backend
   */
  AudioEngine *    audio_engine;

  /**
   * Manages plugins (loading, instantiating, etc.)
   */
  PluginManager *  plugin_manager;

  MainWindowWidget      * main_window;      ///< main window
  GtkTargetEntry        entries[10];        ///< dnd entries
  int                   num_entries;        ///< count

  Preferences *     preferences;

  /**
   * Application settings
   */
  Settings *             settings;

  Project * project;

  UndoManager *          undo_manager;

  Tracklist *         tracklist;

  PianoRoll *         piano_roll;

  SnapGrid *          snap_grid_timeline; ///< snap/grid info for timeline
  Quantize *          quantize_timeline;
  SnapGrid *         snap_grid_midi; ///< snap/grid info for midi editor
  Quantize *          quantize_midi;

  TimelineMinimap *   timeline_minimap; ///< minimap backend

  char                * zrythm_dir;
  char                * projects_dir;
  char                * recent_projects_file;
  char                * recent_projects[1000];
  int                 num_recent_projects;

  /**
   * Filename to open passed through the command line.
   *
   * Used only when a filename is passed.
   * E.g., zrytm myproject.xml
   */
  char *              open_filename;
};

extern ZrythmApp * zrythm;

/**
 * Global variable, should be available to all files.
 */
ZrythmApp * zrythm;

ZrythmApp *
zrythm_new ();

void
zrythm_add_to_recent_projects (ZrythmApp * self,
                                   const char * filepath);

#endif /* __ZRYTHM_H__ */
