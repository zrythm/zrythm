/*
 * Copyright (C) 2018-2019 Alexandros Theodotou <alex at zrythm dot org>
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

#ifndef __GUI_WIDGETS_PROJECT_ASSISTANT_H__
#define __GUI_WIDGETS_PROJECT_ASSISTANT_H__

#include "audio/channel.h"
#include "gui/widgets/meter.h"

#include <gtk/gtk.h>

#define PROJECT_ASSISTANT_WIDGET_TYPE \
  (project_assistant_widget_get_type ())
G_DECLARE_FINAL_TYPE (ProjectAssistantWidget,
                      project_assistant_widget,
                      Z,
                      PROJECT_ASSISTANT_WIDGET,
                      GtkAssistant)

typedef struct ProjectInfo
{
  char *   name;
  char *   filename;
  char *   modified;
} ProjectInfo;

enum
{
  COLUMN_NAME,
  COLUMN_FILENAME,
  COLUMN_MODIFIED,
  COLUMN_PROJECT_INFO,
  NUM_COLUMNS
};

typedef struct _ProjectAssistantWidget
{
  GtkAssistant        parent_instance;
  GtkTreeView         * projects;
  GtkTreeSelection    * projects_selection;
  GtkTreeModel        * model;
  GtkCheckButton      * create_new_project;
  GtkButton *         remove_btn;
  ProjectInfo         project_infos[300];
  ProjectInfo         * selection;
  int                 num_project_infos;
} ProjectAssistantWidget;

/**
 * Creates a channel widget using the given channel data.
 */
ProjectAssistantWidget *
project_assistant_widget_new (GtkWindow * parent,
                            int show_create_new_project);

#endif
