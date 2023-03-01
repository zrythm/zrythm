// SPDX-FileCopyrightText: © 2019-2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __GUI_WIDGETS_HEADER_H__
#define __GUI_WIDGETS_HEADER_H__

#include <stdbool.h>

#include <adwaita.h>
#include <gtk/gtk.h>

#define HEADER_WIDGET_TYPE (header_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  HeaderWidget,
  header_widget,
  Z,
  HEADER_WIDGET,
  GtkWidget)

/**
 * \file
 *
 * Header notebook.
 */

#define MW_HEADER MAIN_WINDOW->header

typedef struct _HomeToolbarWidget      HomeToolbarWidget;
typedef struct _ProjectToolbarWidget   ProjectToolbarWidget;
typedef struct _ViewToolbarWidget      ViewToolbarWidget;
typedef struct _HelpToolbarWidget      HelpToolbarWidget;
typedef struct _MidiActivityBarWidget  MidiActivityBarWidget;
typedef struct _LiveWaveformWidget     LiveWaveformWidget;
typedef struct _SpectrumAnalyzerWidget SpectrumAnalyzerWidget;

/**
 * Header notebook to be used at the very top of the
 * main window.
 */
typedef struct _HeaderWidget
{
  GtkWidget parent_instance;

  /** Notebook toolbars. */
  HomeToolbarWidget *    home_toolbar;
  ProjectToolbarWidget * project_toolbar;
  ViewToolbarWidget *    view_toolbar;
  HelpToolbarWidget *    help_toolbar;

  AdwViewStack * stack;
  GtkBox *       end_box;

  LiveWaveformWidget *     live_waveform;
  SpectrumAnalyzerWidget * spectrum_analyzer;
  MidiActivityBarWidget *  midi_activity;
  GtkLabel *               midi_in_lbl;
  GtkBox *                 meter_box;
} HeaderWidget;

void
header_widget_refresh (HeaderWidget * self);

void
header_widget_setup (HeaderWidget * self, const char * title);

void
header_widget_set_subtitle (
  HeaderWidget * self,
  const char *   subtitle);

#endif
