/*
 * project.h - A project (or song), containing all the project data
 *   as opposed to zrythm_app.h which manages things not project-dependent
 *   like plugins and overall settings
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

#ifndef __PROJECT_H__
#define __PROJECT_H__

#include "zrythm_app.h"

#include <gtk/gtk.h>

#define PROJECT zrythm_system->project

typedef struct Timeline Timeline;
typedef struct Transport Transport;

typedef struct Project
{
  /**
   * Project title
   */
  GString               * title;

  /**
   * Timeline metadata like BPM, time signature, etc.
   */
  Transport             * transport;
} Project;


/**
 * Creates a project with default or given variables
 */
void
create_project (char * filename);

/**
 * Loads project from a file TODO
 */
void
open_project (GString * filename);

/**
 * Saves project to a file TODO
 */
void
save_project (GString * filename);

#endif
