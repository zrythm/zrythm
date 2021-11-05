/*
 * Copyright (C) 2018-2019, 2021 Alexandros Theodotou <alex at zrythm dot org>
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

/**
 * @addtogroup widgets
 *
 * @{
 */

#if 0
/**
 * A widget that allows selecting a project to
 * load or create.
 */
typedef struct _ProjectAssistantWidget
{
  GtkWidget           parent_instance;

  GtkAssistant *      assistant;

  GtkTreeView *       projects;
  GtkTreeSelection *  projects_selection;
  GtkTreeView *       templates;
  GtkTreeSelection *  templates_selection;
  GtkTreeModel *      project_model;
  GtkTreeModel *      template_model;
  GtkCheckButton *    create_new_project;
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
#endif

/**
 * Runs the project assistant.
 *
 * @param zrythm_already_running If true, the logic
 *   applied is different (eg, close does not quit
 *   the program). Used when doing Ctrl+N. This
 *   should be set to false if during startup.
 */
void
project_assistant_widget_present (
  GtkWindow * parent,
  bool        show_create_new_project,
  bool        zrythm_already_running);

/**
 * @}
 */

#endif
