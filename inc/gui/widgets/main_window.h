/*
 * gui/widgets/main_window.h - Main window widget
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

#ifndef __GUI_WIDGETS_MAIN_WINDOW_H__
#define __GUI_WIDGETS_MAIN_WINDOW_H__

#include "zrythm_app.h"
#include "gui/widget_manager.h"

#include <gtk/gtk.h>

#define MAIN_WINDOW WIDGET_MANAGER->main_window
#define MW MAIN_WINDOW

#define MAIN_WINDOW_WIDGET_TYPE                  (main_window_widget_get_type ())
G_DECLARE_FINAL_TYPE (MainWindowWidget,
                      main_window_widget,
                      MAIN_WINDOW,
                      WIDGET,
                      GtkApplicationWindow)

typedef struct BpmWidget BpmWidget;
typedef struct DigitalMeterWidget DigitalMeterWidget;
typedef struct ColorAreaWidget ColorAreaWidget;
typedef struct _SnapGridWidget SnapGridWidget;
typedef struct _HeaderBarWidget HeaderBarWidget;
typedef struct _CenterDockWidget CenterDockWidget;

typedef struct _MainWindowWidget
{
  GtkApplicationWindow     parent_instance;
  GtkBox *                 main_box;
  GtkBox *                 header_bar_box;
  HeaderBarWidget *        header_bar;
  GtkToolbar               * top_toolbar;
  /* FIXME split top toolbar to separate widget */
  SnapGridWidget           * snap_grid_timeline;
  GtkBox                   * center_box;
  CenterDockWidget *       center_dock;
  GtkBox                   * bot_bar;
  GtkToolbar               * bot_bar_left;
  GtkBox                   * digital_meters;
  DigitalMeterWidget       * digital_bpm;
  DigitalMeterWidget       * digital_transport;
  GtkBox                   * transport;
  BpmWidget                * bpm;    ///< created in code
  GtkButton                * play;
  GtkButton                * stop;
  GtkButton                * backward;
  GtkButton                * forward;
  GtkToggleButton          * trans_record;
  GtkToggleButton          * loop;
  int                      is_maximized;
} MainWindowWidget;

/**
 * Creates a main_window widget using the given main_window data.
 */
MainWindowWidget *
main_window_widget_new (ZrythmApp * app);

/**
 * TODO
 */
void
main_window_widget_open (MainWindowWidget  * win,
                         GFile             * file);

void
main_window_widget_quit (MainWindowWidget * self);

void
main_window_widget_toggle_maximize (MainWindowWidget * self);

#endif

