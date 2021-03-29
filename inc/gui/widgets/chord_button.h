/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version approved by
 * Alexandros Theodotou in a public statement of acceptance.
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
 * A button-like Chord pad used in the
 * ChordPadWidget.
 */

#ifndef __GUI_WIDGETS_CHORD_BUTTON_H__
#define __GUI_WIDGETS_CHORD_BUTTON_H__

#include <gtk/gtk.h>

#define CHORD_BUTTON_WIDGET_TYPE \
  (chord_button_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  ChordButtonWidget,
  chord_button_widget,
  Z, CHORD_BUTTON_WIDGET,
  GtkOverlay);

/**
 * A big button-like widget used in the chord pads
 * view (ChordPadWidget) for each key of a piano
 * keyboard (12 buttons total) to trigger a specific
 * Chord.
 *
 * It is an overlay whose main child is a button and
 * it has additional buttons on top to change
 * voicings, etc.
 */
typedef struct _ChordButtonWidget
{
  GtkOverlay      parent_instance;
} ChordButtonWidget;

/**
 * Creates a ChordButtonWidget.
 */
ChordButtonWidget *
chord_button_widget_new (void);

#endif
