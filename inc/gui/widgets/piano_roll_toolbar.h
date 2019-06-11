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

#ifndef __GUI_WIDGETS_PIANO_ROLL_TOOLBAR_H__
#define __GUI_WIDGETS_PIANO_ROLL_TOOLBAR_H__

#include <gtk/gtk.h>

#define PIANO_ROLL_TOOLBAR_WIDGET_TYPE \
  (piano_roll_toolbar_widget_get_type ())
G_DECLARE_FINAL_TYPE (PianoRollToolbarWidget,
                      piano_roll_toolbar_widget,
                      Z,
                      PIANO_ROLL_TOOLBAR_WIDGET,
                      GtkToolbar)

/**
 * \file
 */

#define MW_PIANO_ROLL_TOOLBAR \
  MW_HEADER_NOTEBOOK->piano_roll_toolbar

typedef struct _ToolboxWidget ToolboxWidget;
typedef struct _QuantizeMbWidget QuantizeMbWidget;
typedef struct _SnapBoxWidget SnapBoxWidget;

/**
 * The PianoRoll toolbar in the top.
 */
typedef struct _PianoRollToolbarWidget
{
  GtkToolbar         parent_instance;
  GtkMenuToolButton * chord_highlighting;
} PianoRollToolbarWidget;

void
piano_roll_toolbar_widget_refresh_undo_redo_buttons (
  PianoRollToolbarWidget * self);

void
piano_roll_toolbar_widget_setup (
  PianoRollToolbarWidget * self);

#endif
