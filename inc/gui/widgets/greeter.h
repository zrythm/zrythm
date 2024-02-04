// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __GUI_WIDGETS_GREETER_H__
#define __GUI_WIDGETS_GREETER_H__

#include "utils/types.h"

#include <adwaita.h>

#include <zix/sem.h>

#define GREETER_WIDGET_TYPE (greeter_widget_get_type ())
G_DECLARE_FINAL_TYPE (GreeterWidget, greeter_widget, Z, GREETER_WIDGET, AdwWindow)

TYPEDEF_STRUCT_UNDERSCORED (ZrythmApp);
TYPEDEF_STRUCT_UNDERSCORED (IdeFileChooserEntry);

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
  char * name;
  /** Full path. */
  char * filename;
  gint64 modified;
  char * modified_str;
} ProjectInfo;

/**
 * This widget handles the UI part of the initialization of Zrythm and presents
 * a project selector when ready so that a project can be opened.
 *
 * Projects are opened in a MainWindowWidget when this widget is disposed.
 */
typedef struct _GreeterWidget
{
  AdwWindow parent_instance;

  /** Stack for transitioning between progress -> project selector. */
  GtkStack * stack;

  /* -- welcome/config pages -- */

  AdwCarousel *   welcome_carousel;
  GtkButton *     continue_to_config_btn;
  AdwStatusPage * read_manual_status_page;
  AdwStatusPage * donate_status_page;
  AdwStatusPage * about_flatpak_status_page;
  // AdwStatusPage * legal_info_status_page;
  AdwNavigationView * nav_view;
  AdwNavigationPage * nav_config_page;

  AdwPreferencesPage *  pref_page;
  AdwComboRow *         language_dropdown;
  GtkLabel *            lang_error_txt;
  IdeFileChooserEntry * fc_entry;
  GtkButton *           config_ok_btn;
  GtkButton *           config_reset_btn;

  /* -- progress page -- */

  AdwStatusPage * status_page;
  // GtkLabel * status_title;
  // GtkLabel * status_description;
  GtkProgressBar * progress_bar;

  /** Semaphore for setting the progress from multiple threads. */
  ZixSem progress_status_lock;

  /** Progress done (0.0 ~ 1.0). */
  double progress;

  char title[800];
  char description[800];

  /* -- projects page -- */

  AdwNavigationView * open_prj_navigation_view;

  AdwPreferencesGroup * recent_projects_pref_group;
  GPtrArray *           project_infos_arr;
  GPtrArray *           recent_projects_item_factories;
  GtkButton *           create_new_project_btn;
  GtkButton *           select_folder_btn;

  AdwNavigationPage *   create_project_nav_page;
  AdwEntryRow *         project_title_row;
  AdwActionRow *        project_parent_dir_row;
  IdeFileChooserEntry * project_parent_dir_fc;
  AdwComboRow *         templates_combo_row;
  GPtrArray *           templates_arr;
  GPtrArray *           templates_item_factories;
  AdwPreferencesGroup * templates_pref_group;
  GtkButton *           create_project_confirm_btn;

  char * template;

  bool zrythm_already_running;

  /* --- other --- */

  guint tick_cb_id;

  /** Initialization thread. */
  GThread * init_thread;
} GreeterWidget;

/**
 * Creates a greeter widget.
 *
 * @param is_startup Whether the greeter is shown as part of the Zrythm startup
 * process. If not, The greeter will jump to the project selection screen.
 * @param is_for_new_project Whether this is called for a new project (ie, when
 * using Ctrl+N while Zrythm is running).
 */
GreeterWidget *
greeter_widget_new (
  ZrythmApp * app,
  GtkWindow * parent,
  bool        is_startup,
  bool        is_for_new_project);

/**
 * Sets the current status and progress percentage.
 */
void
greeter_widget_set_progress_and_status (
  GreeterWidget * self,
  const char *    title,
  const char *    description,
  const double    perc);

/**
 * Proceed to the project selection screen.
 *
 * @param zrythm_already_running If true, the logic applied is
 *   different (eg, close does not quit the program). Used when
 *   doing Ctrl+N. This should be set to false if during
 *   startup.
 * @param is_for_new_project Whether this is called for a new project (ie, when
 * using Ctrl+N while Zrythm is running).
 * @param template_to_use Template to create a new project from, if non-NULL.
 */
void
greeter_widget_select_project (
  GreeterWidget * self,
  bool            zrythm_already_running,
  bool            is_for_new_project,
  const char *    template_to_use);

#if 0
void
greeter_widget_close (GreeterWidget * self);
#endif

/**
 * @}
 */

#endif
