// SPDX-FileCopyrightText: © 2018-2019, 2022-2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

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

#include "dsp/channel.h"
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
  AdwWindow)

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
  AdwWindow parent_instance;

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
