/*
 * inc/zrythm_app.h - The GTK application
 *
 * Copyright (C) 2018 Alexandros Theodotou
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

#ifndef __ZRYTHM_APP_H__
#define __ZRYTHM_APP_H__

#include "utils/selections.h"

#include <gtk/gtk.h>


#define ZRYTHM_APP_TYPE (zrythm_app_get_type ())
G_DECLARE_FINAL_TYPE (ZrythmApp, zrythm_app, ZRYTHM, APP, GtkApplication)

typedef struct Widget_Manager Widget_Manager;
typedef struct Settings_Manager Settings_Manager;
typedef struct Audio_Engine Audio_Engine;
typedef struct Plugin_Manager Plugin_Manager;
typedef struct Project Project;

typedef struct Zrythm_System
{

  /**
   * The audio backend
   */
  Audio_Engine *    audio_engine;

  /**
   * Manages plugins (loading, instantiating, etc.)
   */
  Plugin_Manager *  plugin_manager;

  /**
   * Manages GUI widgets
   */
  Widget_Manager * widget_manager;

  /**
   * Application settings
   */
  Settings_Manager *       settings_manager;

  /**
   * Currently selected items.
   *
   * Currently selected track, selected channel, selected region, selected
   * note, etc.
   */
  Selections           selections;

  /**
   * The project global variable, containing all the information that
   * should be available to all files.
   */
  Project * project;

} Zrythm_System;

extern Zrythm_System * zrythm_system;

/**
 * Global variable, should be available to all files.
 */
Zrythm_System * zrythm_system;

ZrythmApp     *zrythm_app_new         (void);


#endif /* __ZRYTHM_APP_H__ */

