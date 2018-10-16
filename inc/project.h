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

/* project xml version */
#define PROJECT_XML_VER "0.1"

typedef struct Timeline Timeline;
typedef struct Transport Transport;

typedef struct Project
{
  /**
   * Project title
   */
  char                  * title;

  /**
   * Timeline metadata like BPM, time signature, etc.
   */
  Transport             * transport;
} Project;


/**
 * Creates a project with default or given variables
 */
void
project_create (char * filename);

/**
 * Loads project from a file.
 */
void
project_load (char * filename);

/**
 * Saves project to a file.
 */
void
project_save (char * filename);

#endif
