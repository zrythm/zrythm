/*
 * Copyright (C) 2018-2019, 2022 Alexandros Theodotou <alex at zrythm dot org>
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

#include <adwaita.h>
#include <gtk/gtk.h>

/**
 * @addtogroup widgets
 *
 * @{
 */

#define PROJECT_ASSISTANT_WIDGET_TYPE \
  (project_assistant_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  ProjectAssistantWidget,
  project_assistant_widget,
  Z,
  PROJECT_ASSISTANT_WIDGET,
  GtkDialog)

/**
 * Project file information.
 */
typedef struct ProjectInfo
{
  char * name;
  /** Full path. */
  char * filename;
  gint64 modified;
  char * modified_str;
} ProjectInfo;

/**
 * A widget that allows selecting a project to
 * load or create.
 */
typedef struct _ProjectAssistantWidget
{
  GtkDialog parent_instance;

  AdwViewStack * stack;

  /** Array of ItemFactory pointers for each
   * column. */
  GPtrArray *      recent_projects_item_factories;
  GtkColumnView *  recent_projects_column_view;
  GtkCheckButton * create_new_project_check_btn;
  GPtrArray *      templates_item_factories;
  GtkColumnView *  templates_column_view;

  /* action buttons */
  GtkButton * ok_btn;
  GtkButton * open_from_path_btn;
  GtkButton * cancel_btn;

  GPtrArray * project_infos_arr;
  GPtrArray * templates_arr;

  GtkWindow * parent;

  char * template;

  bool zrythm_already_running;
} ProjectAssistantWidget;

/**
 * Runs the project assistant.
 *
 * @param zrythm_already_running If true, the logic applied is
 *   different (eg, close does not quit the program). Used when
 *   doing Ctrl+N. This should be set to false if during
 *   startup.
 * @param template Template to create a new project from, if
 *   non-NULL.
 */
void
project_assistant_widget_present (
  GtkWindow * parent,
  bool        zrythm_already_running,
  const char * template);

/**
 * @}
 */

#endif
