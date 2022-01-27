/*
 * Copyright (C) 2019-2020 Alexandros Theodotou <alex at zrythm dot org>
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
 * Chord pad in the bottom panel.
 */

#ifndef __GUI_WIDGETS_CHORD_PAD_H__
#define __GUI_WIDGETS_CHORD_PAD_H__

#include <gtk/gtk.h>

typedef struct _ChordWidget ChordWidget;

/**
 * @addtogroup widgets
 *
 * @{
 */

#define CHORD_PAD_WIDGET_TYPE \
  (chord_pad_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  ChordPadWidget,
  chord_pad_widget,
  Z, CHORD_PAD_WIDGET,
  GtkGrid)

#define MW_CHORD_PAD \
  MW_BOT_DOCK_EDGE->chord_pad

/**
 * Brings up the ChordPadWidget in the notebook.
 */
#define SHOW_CHORD_PAD \
  gtk_notebook_set_current_page ( \
    MW_CHORD_PAD->bot_notebook, 3)

/**
 * Tab for chord pads.
 */
typedef struct _ChordPadWidget
{
  GtkGrid          parent_instance;

  GtkGrid *        chords_grid;

  GtkButton *      save_preset_btn;
  GtkMenuButton *  load_preset_btn;
  GtkButton *      transpose_up_btn;
  GtkButton *      transpose_down_btn;

  /** Chords inside the grid. */
  ChordWidget *    chords[12];
} ChordPadWidget;

void
chord_pad_widget_setup (
  ChordPadWidget * self);

void
chord_pad_widget_refresh (
  ChordPadWidget * self);

ChordPadWidget *
chord_pad_widget_new (void);

/**
 * @}
 */

#endif
