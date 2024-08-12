// SPDX-FileCopyrightText: © 2018-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-FileCopyrightText: © 2024 Miró Allard <miro.allard@pm.me>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __GUI_WIDGETS_MAIN_WINDOW_H__
#define __GUI_WIDGETS_MAIN_WINDOW_H__

#include "zrythm.h"

#include "gtk_wrapper.h"
#include "libadwaita_wrapper.h"
#include "libpanel_wrapper.h"

#define MAIN_WINDOW_WIDGET_TYPE (main_window_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  MainWindowWidget,
  main_window_widget,
  Z,
  MAIN_WINDOW_WIDGET,
  AdwApplicationWindow)

TYPEDEF_STRUCT_UNDERSCORED (ToolboxWidget);
TYPEDEF_STRUCT_UNDERSCORED (CenterDockWidget);
TYPEDEF_STRUCT_UNDERSCORED (BotBarWidget);
TYPEDEF_STRUCT_UNDERSCORED (TopBarWidget);
TYPEDEF_STRUCT_UNDERSCORED (ZrythmApp);
class ArrangerSelections;

/**
 * @addtogroup widgets
 *
 * @{
 */

#define MAIN_WINDOW zrythm_app->main_window
#define MW MAIN_WINDOW

/**
 * The main window of Zrythm.
 *
 * Inherits from GtkApplicationWindow, meaning that
 * it is the parent of all other sub-windows of
 * Zrythm.
 */
using MainWindowWidget = struct _MainWindowWidget
{
  AdwApplicationWindow parent_instance;

  PanelToggleButton * start_dock_switcher;
  AdwWindowTitle *    window_title;
  PanelToggleButton * end_dock_switcher;

  ToolboxWidget * toolbox;

  AdwSplitButton * undo_btn;
  AdwSplitButton * redo_btn;

  GtkBox *    header_start_box;
  GtkBox *    header_end_box;
  GtkButton * z_icon;

  GtkBox *           center_box;
  CenterDockWidget * center_dock;
  BotBarWidget *     bot_bar;
  int                is_fullscreen;
  int                height;
  int                width;
  AdwToastOverlay *  toast_overlay;

  /** Whether preferences window is opened. */
  bool preferences_opened;

  /** Whether log has pending warnings (if true,
   * the log viewer button will have an emblem until
   * clicked). */
  bool log_has_pending_warnings;

  /** Whether set up already or not. */
  bool setup;
};

/**
 * Creates a main_window widget using the given
 * app data.
 */
MainWindowWidget *
main_window_widget_new (ZrythmApp * app);

void
main_window_widget_refresh_undo_redo_buttons (MainWindowWidget * self);

/**
 * Refreshes the state of the main window.
 */
void
main_window_widget_setup (MainWindowWidget * self);

/**
 * Updates the project name at the top of the window.
 */
void
main_window_widget_set_project_title (MainWindowWidget * self, Project * prj);

/**
 * Prepare for finalization.
 */
void
main_window_widget_tear_down (MainWindowWidget * self);

void
main_window_widget_quit (MainWindowWidget * self);

/**
 * @}
 */

#endif
