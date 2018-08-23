/*
 * zrythm_system.h - Manages the whole application
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
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __ZRYTHM_SYSTEM_H__
#define __ZRYTHM_SYSTEM_H__

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

void
zrythm_system_init ();

#endif
