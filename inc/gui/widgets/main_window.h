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
typedef struct BpmWidget BpmWidget;
typedef struct TracklistWidget TracklistWidget;
typedef struct DigitalMeterWidget DigitalMeterWidget;
typedef struct ColorAreaWidget ColorAreaWidget;
typedef struct MixerWidget MixerWidget;
typedef struct MidiEditorWidget MidiEditorWidget;
typedef struct BrowserWidget BrowserWidget;
typedef struct Region Region;
typedef struct SnapGridWidget SnapGridWidget;
typedef struct ArrangerWidget ArrangerWidget;

typedef struct _MainWindowWidget
{
  GtkApplicationWindow     parent_instance;
  GtkLabel                 * title;
  GtkBox                   * main_box;
  GtkBox                   * top_bar;
  GtkBox                   * top_menubar;
  GtkMenuItem              * file;
  GtkImageMenuItem         * file_new;
  GtkImageMenuItem         * file_open;
  GtkImageMenuItem         * file_save;
  GtkImageMenuItem         * file_save_as;
  GtkImageMenuItem         * file_export;
  GtkImageMenuItem         * file_quit;
  GtkMenuItem              * edit;
  GtkMenuItem              * view;
  GtkMenuItem              * help;
  GtkImageMenuItem         * help_about;
  GtkBox                   * window_buttons;
  GtkButton                * minimize;
  GtkButton                * maximize;
  GtkButton                * close;
  GtkToolbar               * top_toolbar;
  GtkBox                   * snap_grid_timeline_box;
  SnapGridWidget           * snap_grid_timeline;
  GtkBox                   * center_box;
  GtkBox                   * inspector;
  GtkButton                * inspector_button;
  GtkPaned                 * editor_plus_browser;
  GtkPaned                 * editor;
  GtkBox                   * editor_top;
  GtkPaned                 * tracklist_timeline;
  GtkBox                   * tracklist_top;
  GtkScrolledWindow        * tracklist_scroll;
  GtkViewport              * tracklist_viewport;
  TracklistWidget          * tracklist;
  GtkGrid                  * tracklist_header;
  GtkBox                   * timeline_ruler;
  GtkScrolledWindow        * ruler_scroll;
  GtkViewport              * ruler_viewport;
  RulerWidget              * ruler;     ///< created in code
  GtkScrolledWindow        * timeline_scroll;
  GtkViewport              * timeline_viewport;
  ArrangerWidget           * timeline;
  GtkToolbar               * instruments_toolbar;
  GtkBox                   * snap_grid_midi_box;
  SnapGridWidget           * snap_grid_midi;
  GtkToolButton            * instrument_add;
  GtkNotebook              * bot_notebook;
  MidiEditorWidget         * midi_editor;
  MixerWidget              * mixer;
  GtkNotebook              * right_notebook;
  BrowserWidget            * browser;
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
  Region                   * selected_region;  ///< FIXME move to a new class "Selections" or something in zrythm_system
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

