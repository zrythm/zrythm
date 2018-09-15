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

#define MAIN_WINDOW_WIDGET_TYPE                  (main_window_widget_get_type ())
G_DECLARE_FINAL_TYPE (MainWindowWidget, main_window_widget, MAIN, WINDOW_WIDGET, GtkApplicationWindow)

typedef struct RulerWidget RulerWidget;
typedef struct TimelineWidget TimelineWidget;
typedef struct BpmWidget BpmWidget;

typedef struct _MainWindowWidget
{
  GtkApplicationWindow     parent_instance;
  GtkBox                   * main_box;
  GtkBox                   * top_bar;
  GtkBox                   * top_menubar;
  GtkMenuItem              * file;
  GtkMenuItem              * edit;
  GtkMenuItem              * view;
  GtkMenuItem              * help;
  GtkBox                   * window_buttons;
  GtkButton                * minimize;
  GtkButton                * maximize;
  GtkButton                * close;
  GtkToolbar               * top_toolbar;
  GtkBox                   * center_box;
  GtkBox                   * inspector;
  GtkButton                * inspector_button;
  GtkPaned                 * editor_plus_browser;
  GtkPaned                 * editor;
  GtkBox                   * editor_top;
  GtkPaned                 * tracks_timeline;
  GtkBox                   * tracks_top;
  GtkScrolledWindow        * tracks_scroll;
  GtkViewport              * tracks_viewport;
  GtkPaned                 * tracks_paned;
  GtkGrid                  * tracks_header;
  GtkBox                   * timeline_ruler;
  GtkScrolledWindow        * ruler_scroll;
  GtkViewport              * ruler_viewport;
  RulerWidget              * ruler;     ///< created in code
  GtkScrolledWindow        * timeline_scroll;
  GtkViewport              * timeline_viewport;
  GtkOverlay               * timeline_overlay;
  TimelineWidget           * timeline;
  GtkToolbar               * instruments_toolbar;
  GtkToolButton            * instrument_add;
  GtkNotebook              * editor_bot;
  GtkBox                   * mixer;
  GtkBox                   * dummy_mixer_box;  ///< dummy box for dnd
  GtkScrolledWindow        * channels_scroll;
  GtkViewport              * channels_viewport;
  GtkBox                   * channels;
  GtkButton                * channels_add;
  GtkNotebook              * browser_notebook;
  GtkPaned                 * browser;
  GtkGrid                  * browser_top;
  GtkSearchEntry           * browser_search;
  GtkExpander              * collections_exp;
  GtkExpander              * types_exp;
  GtkExpander              * cat_exp;
  GtkBox                   * browser_bot;
  GtkBox                   * bot_bar;
  GtkToolbar               * bot_bar_left;
  GtkBox                   * transport;
  GtkBox                   * bpm_box;
  BpmWidget                * bpm;    ///< created in code
  GtkButton                * play;
  GtkButton                * stop;
  GtkButton                * backward;
  GtkButton                * forward;
  GtkToggleButton          * trans_record;
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

#endif

