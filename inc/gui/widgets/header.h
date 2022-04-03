/*
 * Copyright (C) 2019-2022 Alexandros Theodotou <alex at zrythm dot org>
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

#ifndef __GUI_WIDGETS_HEADER_H__
#define __GUI_WIDGETS_HEADER_H__

#include <stdbool.h>

#include <adwaita.h>
#include <gtk/gtk.h>

#define HEADER_WIDGET_TYPE \
  (header_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  HeaderWidget,
  header_widget,
  Z,
  HEADER_WIDGET,
  GtkBox)

/**
 * \file
 *
 * Header notebook.
 */

#define MW_HEADER MAIN_WINDOW->header

typedef struct _HomeToolbarWidget HomeToolbarWidget;
typedef struct _ProjectToolbarWidget
  ProjectToolbarWidget;
typedef struct _ViewToolbarWidget ViewToolbarWidget;
typedef struct _HelpToolbarWidget HelpToolbarWidget;
typedef struct _MidiActivityBarWidget
  MidiActivityBarWidget;
typedef struct _LiveWaveformWidget LiveWaveformWidget;
typedef struct _RotatedLabelWidget RotatedLabelWidget;

/**
 * Header notebook to be used at the very top of the
 * main window.
 */
typedef struct _HeaderWidget
{
  GtkBox parent_instance;

  /** Notebook toolbars. */
  HomeToolbarWidget *    home_toolbar;
  ProjectToolbarWidget * project_toolbar;
  ViewToolbarWidget *    view_toolbar;
  HelpToolbarWidget *    help_toolbar;

  AdwViewStack * stack;

  LiveWaveformWidget *    live_waveform;
  MidiActivityBarWidget * midi_activity;
  RotatedLabelWidget *    midi_in_rotated_lbl;
} HeaderWidget;

void
header_widget_refresh (HeaderWidget * self);

void
header_widget_setup (
  HeaderWidget * self,
  const char *   title);

void
header_widget_set_subtitle (
  HeaderWidget * self,
  const char *   subtitle);

#endif
