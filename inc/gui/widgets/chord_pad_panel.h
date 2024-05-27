/*
 * SPDX-FileCopyrightText: © 2019-2022 Alexandros Theodotou <alex@zrythm.org>
 *
 * SPDX-License-Identifier: LicenseRef-ZrythmLicense
 */

/**
 * \file
 *
 * Chord pad panel in the bottom panel.
 */

#ifndef __GUI_WIDGETS_CHORD_PAD_PANEL_H__
#define __GUI_WIDGETS_CHORD_PAD_PANEL_H__

#include "gtk_wrapper.h"

typedef struct _ChordPadWidget ChordPadWidget;

/**
 * @addtogroup widgets
 *
 * @{
 */

#define CHORD_PAD_PANEL_WIDGET_TYPE (chord_pad_panel_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  ChordPadPanelWidget,
  chord_pad_panel_widget,
  Z,
  CHORD_PAD_PANEL_WIDGET,
  GtkGrid)

#define MW_CHORD_PAD_PANEL MW_BOT_DOCK_EDGE->chord_pad_panel

/**
 * Brings up the ChordPadPanelWidget in the notebook.
 */
#define SHOW_CHORD_PAD_PANEL \
  gtk_notebook_set_current_page (MW_CHORD_PAD_PANEL->bot_notebook, 3)

/**
 * Tab for chord pads.
 */
typedef struct _ChordPadPanelWidget
{
  GtkGrid parent_instance;

  GtkGrid * chords_grid;

  GtkButton *     save_preset_btn;
  GtkMenuButton * load_preset_btn;
  GtkButton *     transpose_up_btn;
  GtkButton *     transpose_down_btn;

  /** Chords inside the grid. */
  ChordPadWidget * chords[12];
} ChordPadPanelWidget;

void
chord_pad_panel_widget_setup (ChordPadPanelWidget * self);

void
chord_pad_panel_widget_refresh_load_preset_menu (ChordPadPanelWidget * self);

void
chord_pad_panel_widget_refresh (ChordPadPanelWidget * self);

ChordPadPanelWidget *
chord_pad_panel_widget_new (void);

/**
 * @}
 */

#endif
