/*
 * Copyright (C) 2018-2019 Alexandros Theodotou <alex at zrythm dot org>
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
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * \file
 *
 * Start assistant to be shown on startup to select
 * a project.
 *
 * FIXME rename to project selector.
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

/**
 * @addtogroup widgets
 *
 * @{
 */

/**
 * Project file information.
 */
typedef struct ProjectInfo
{
  char *   name;
  /** Full path. */
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

/**
 * A widget that allows selecting a project to
 * load or create.
 */
typedef struct _ProjectAssistantWidget
{
  GtkAssistant        parent_instance;
  GtkTreeView         * projects;
  GtkTreeSelection    * projects_selection;
  GtkTreeView         * templates;
  GtkTreeSelection    * templates_selection;
  GtkTreeModel        * project_model;
  GtkTreeModel        * template_model;
  GtkCheckButton      * create_new_project;
  GtkBox *            templates_box;

  /** The project info label. */
  //GtkLabel *          label;

  GtkButton *         remove_btn;
  ProjectInfo         project_infos[300];
  int                 num_project_infos;
  ProjectInfo         template_infos[300];
  int                 num_template_infos;

  /** 1 if a template should be loaded. */
  int                 load_template;

  /** The selected project/template. */
  ProjectInfo         * project_selection;
  ProjectInfo         * template_selection;
} ProjectAssistantWidget;

/**
 * Creates a ProjectAssistantWidget.
 */
ProjectAssistantWidget *
project_assistant_widget_new (
  GtkWindow * parent,
  int show_create_new_project);

/**
 * @}
 */

#endif
