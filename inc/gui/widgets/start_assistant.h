/*
 * gui/widgets/start_assistant.h - Start assistant to show when launching Zrythm
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

#ifndef __GUI_WIDGETS_START_ASSISTANT_H__
#define __GUI_WIDGETS_START_ASSISTANT_H__

#include "audio/channel.h"
#include "gui/widgets/meter.h"

#include <gtk/gtk.h>

#define START_ASSISTANT_WIDGET_TYPE                  (start_assistant_widget_get_type ())
#define START_ASSISTANT_WIDGET(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), START_ASSISTANT_WIDGET_TYPE, StartAssistantWidget))
#define START_ASSISTANT_WIDGET_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST  ((klass), START_ASSISTANT_WIDGET, StartAssistantWidgetClass))
#define IS_START_ASSISTANT_WIDGET(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), START_ASSISTANT_WIDGET_TYPE))
#define IS_START_ASSISTANT_WIDGET_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE  ((klass), START_ASSISTANT_WIDGET_TYPE))
#define START_ASSISTANT_WIDGET_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS  ((obj), START_ASSISTANT_WIDGET_TYPE, StartAssistantWidgetClass))

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

typedef struct StartAssistantWidget
{
  GtkAssistant        parent_instance;
  GtkTreeView         * projects;
  GtkTreeSelection    * projects_selection;
  GtkTreeModel        * model;
  GtkCheckButton      * create_new_project;
  ProjectInfo         project_infos[300];
  ProjectInfo         * selection;
  int                 num_project_infos;
} StartAssistantWidget;

typedef struct StartAssistantWidgetClass
{
  GtkAssistantClass       parent_class;
} StartAssistantWidgetClass;


/**
 * Creates a channel widget using the given channel data.
 */
StartAssistantWidget *
start_assistant_widget_new (GtkWindow * parent,
                            int show_create_new_project);

#endif

