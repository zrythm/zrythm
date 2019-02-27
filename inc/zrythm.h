/*
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

/**
 * \file
 *
 * The GTK application.
 */

#ifndef __ZRYTHM_H__
#define __ZRYTHM_H__

#include "audio/snap_grid.h"
#include "gui/backend/events.h"
#include "gui/backend/file_manager.h"
#include "plugins/plugin_manager.h"
#include "settings/settings.h"

#include <gtk/gtk.h>

#define ZRYTHM zrythm

#define ZRYTHM_APP_TYPE (zrythm_app_get_type ())
G_DECLARE_FINAL_TYPE (ZrythmApp,
                      zrythm_app,
                      ZRYTHM,
                      APP,
                      GtkApplication)

/* Gtk target entries */
/* Plugin descriptor, used to instantiate plugins */
#define TARGET_ENTRY_PLUGIN_DESCR "PLUGIN_DESCR"
#define TARGET_ENTRY_ID_PLUGIN_DESCR 0
/* Plugin ID, used to move/copy plugins */
#define TARGET_ENTRY_PLUGIN "PLUGIN"
#define TARGET_ENTRY_ID_PLUGIN 1
/* */
#define TARGET_ENTRY_TIMELINE_SELECTIONS \
  "TIMELINE_SELECTIONS"
#define TARGET_ENTRY_ID_TIMELINE_SELECTIONS 2

typedef struct _MainWindowWidget MainWindowWidget;
typedef struct Project Project;

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

  char *                  zrythm_dir;
  char *                  projects_dir;
  char *                  recent_projects_file;
  char *                  recent_projects[1000];
  int                     num_recent_projects;

  /**
   * Filename to open passed through the command line.
   *
   * Used only when a filename is passed.
   * E.g., zrytm myproject.xml
   */
  char *                  open_filename;

  /**
   * GUI events.
   */
  Events                  events;
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
zrythm_add_to_recent_projects (Zrythm * self,
                               const char * filepath);

#endif /* __ZRYTHM_H__ */
