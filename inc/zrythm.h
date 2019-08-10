/*
 * Copyright (C) 2019 Alexandros Theodotou
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * \file
 *
 * The GTK application.
 */

#ifndef __ZRYTHM_H__
#define __ZRYTHM_H__

#include "audio/metronome.h"
#include "audio/snap_grid.h"
#include "gui/backend/events.h"
#include "gui/backend/file_manager.h"
#include "plugins/plugin_manager.h"
#include "settings/settings.h"

#include <gtk/gtk.h>

#define ZRYTHM_APP_TYPE (zrythm_app_get_type ())
G_DECLARE_FINAL_TYPE (ZrythmApp,
                      zrythm_app,
                      ZRYTHM,
                      APP,
                      GtkApplication)

typedef struct _MainWindowWidget MainWindowWidget;
typedef struct Project Project;
typedef struct Symap Symap;

/**
 * @addtogroup general
 *
 * @{
 */

#define ZRYTHM zrythm

#define MAX_RECENT_PROJECTS 20
#define DEBUGGING (ZRYTHM->debug)

/**
 * To be used throughout the program.
 *
 * Everything here should be global and function regardless
 * of the project.
 */
typedef struct Zrythm
{
  /**
   * Manages plugins (loading, instantiating, etc.)
   */
  PluginManager           plugin_manager;

  FileManager             file_manager;

  MainWindowWidget *      main_window; ///< main window

  /**
   * Application settings
   */
  Settings                settings;

  /**
   * Project data.
   *
   * This is what should be exported/imported when saving/
   * loading projects.
   *
   * The only reason this is a pointer is to easily
   * deserialize.
   */
  Project *               project;

  /** /home/user/zrythm */
  char *                  zrythm_dir;
  /** <zrythm_dir>/Projects */
  char *                  projects_dir;
  /** <zrythm_dir>/Templates */
  char *                  templates_dir;
  /** <zrythm_dir>/log */
  char *                  log_dir;

  /** +1 to ensure last element is NULL in case
   * full. */
  char *                  recent_projects[MAX_RECENT_PROJECTS + 1];
  int                     num_recent_projects;

  /** The metronome. */
  Metronome               metronome;

  /**
   * Filename to open passed through the command line.
   *
   * Used only when a filename is passed.
   * E.g., zrytm myproject.xml
   */
  char *                  open_filename;

  /**
   * Event queue, mainly for GUI events.
   */
  GAsyncQueue *           event_queue;

  /**
   * String interner for internal things.
   */
  Symap *                 symap;

  /**
   * In debug mode or not (determined by GSetting).
   */
  int                     debug;
} Zrythm;

/**
 * The global struct.
 *
 * Contains data that is irrelevant to the project.
 */
struct _ZrythmApp
{
  GtkApplication      parent;

  Zrythm *            zrythm;
};

extern Zrythm * zrythm;
extern ZrythmApp * zrythm_app;

/**
 * Global variable, should be available to all files.
 */
Zrythm * zrythm;
ZrythmApp * zrythm_app;

ZrythmApp *
zrythm_app_new ();

void
zrythm_add_to_recent_projects (
  Zrythm * self,
  const char * filepath);

void
zrythm_remove_recent_project (
  char * filepath);

/**
 * Gets the zrythm directory (by default
 * /home/user/zrythm).
 *
 * Must be free'd by caler.
 */
char *
zrythm_get_dir (
  Zrythm * self);

/**
 * @}
 */

#endif /* __ZRYTHM_H__ */
