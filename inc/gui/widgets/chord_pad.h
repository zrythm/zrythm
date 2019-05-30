/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
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

/**
 * \file
 *
 * Chord pad in the bottom panel.
 */

#ifndef __GUI_WIDGETS_CHORD_PAD_H__
#define __GUI_WIDGETS_CHORD_PAD_H__

#include <gtk/gtk.h>

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
 * The ChordPadWidget contains the
 * ModulatorWidgets for the selected Track.
 */
typedef struct _ChordPadWidget
{
  GtkGrid            parent_instance;

  GtkOverlay *       pads_overlay;
} ChordPadWidget;

#endif
