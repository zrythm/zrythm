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

#include "zrythm.h"

#include <gtk/gtk.h>

#define PROJECT                 ZRYTHM->project
#define DEFAULT_PROJECT_NAME    "Untitled Project"
#define PROJECT_FILE            "project.xml"
#define PROJECT_REGIONS_FILE    "regions.xml"
#define PROJECT_PORTS_FILE      "ports.xml"
#define PROJECT_REGIONS_DIR     "Regions"
#define PROJECT_STATES_DIR      "states"

/* project xml version */
#define PROJECT_XML_VER "0.1"

typedef struct Timeline Timeline;
typedef struct Transport Transport;
typedef struct Port Port;
typedef struct Channel Channel;
typedef struct Plugin Plugin;
typedef struct Track Track;
typedef struct Region Region;
typedef struct AutomationPoint AutomationPoint;
typedef struct AutomationCurve AutomationCurve;
typedef struct MidiNote MidiNote;
typedef struct ChordTrack ChordTrack;
typedef struct Tracklist Tracklist;

typedef struct Project
{
  char              * title; ///< project title

  /**
   * Absolute path to project xml.
   */
  char              * filename;

  char              * dir; ///< path to save the project
  char              * project_file_path; ///< for convenience
  char              * regions_file_path;
  char              * ports_file_path;
  char              * regions_dir;
  char              * states_dir;

  /* for bookkeeping FIXME no, delete. manually go through each channel to find them */
  Channel           * channels[3000];
  int               num_channels;
  Plugin            * plugins[30000];
  int               num_plugins;
  Region            * regions [30000];
  int               num_regions;
  AutomationPoint * automation_points[30000];
  int               num_automation_points;
  AutomationCurve * automation_curves[30000];
  int               num_automation_curves;
  MidiNote *        midi_notes[30000];
  int               num_midi_notes;

  ChordTrack *      chord_track; ///< the chord track
  int               loaded; ///< project is loaded or not
} Project;


Project *
project_new ();

/**
 * If project has a filename set, it loads that. Otherwise
 * it loads the default project.
 */
void
project_setup (Project * self,
               char * filename);

/**
 * Saves project to a file.
 */
void
project_save (Project *    self,
              const char * dir);

void
project_add_region (Project * project,
                    Region *  region);

#endif
