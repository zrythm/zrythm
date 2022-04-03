/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * \file
 *
 * Chord editor space.
 */

#ifndef __GUI_WIDGETS_CHORD_EDITOR_SPACE_H__
#define __GUI_WIDGETS_CHORD_EDITOR_SPACE_H__

#include <gtk/gtk.h>

#define CHORD_EDITOR_SPACE_WIDGET_TYPE \
  (chord_editor_space_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  ChordEditorSpaceWidget,
  chord_editor_space_widget,
  Z,
  CHORD_EDITOR_SPACE_WIDGET,
  GtkBox)

typedef struct _ArrangerWidget ArrangerWidget;
typedef struct _ChordKeyWidget ChordKeyWidget;

/**
 * @addtogroup widgets
 *
 * @{
 */

#define MW_CHORD_EDITOR_SPACE \
  MW_CLIP_EDITOR_INNER->chord_editor_space

/**
 * The piano roll widget is the whole space inside
 * the clip editor tab when a CHORD region is selected.
 */
typedef struct _ChordEditorSpaceWidget
{
  GtkBox parent_instance;

  /** The main vertical paned. */
  GtkPaned * main_top_bot_paned;

  /** The box dividing the chord keys and the
   * arranger. */
  GtkBox * chord_keys_slash_arranger_box;

  /** The box on the left of the arranger containing
   * the chord keys. */
  GtkBox * left_box;

  /** The box wrapped inside a scroll that will host
   * the ChordKeyWidget's. */
  GtkBox * chord_keys_box;

  /** The chord keys (see ChordEditor). */
  ChordKeyWidget * chord_keys[128];

  /** Containers for each chord key. */
  GtkBox * chord_key_boxes[128];

  GtkScrolledWindow * chord_keys_scroll;

  /** Vertical size goup for the keys and the
   * arranger. */
  GtkSizeGroup * arranger_and_keys_vsize_group;

  /** The arranger. */
  ArrangerWidget *    arranger;
  GtkScrolledWindow * arranger_scroll;
  GtkViewport *       arranger_viewport;
} ChordEditorSpaceWidget;

int
chord_editor_space_widget_get_chord_height (
  ChordEditorSpaceWidget * self);

int
chord_editor_space_widget_get_all_chords_height (
  ChordEditorSpaceWidget * self);

void
chord_editor_space_widget_setup (
  ChordEditorSpaceWidget * self);

/**
 * See CLIP_EDITOR_INNER_WIDGET_ADD_TO_SIZEGROUP.
 */
void
chord_editor_space_widget_update_size_group (
  ChordEditorSpaceWidget * self,
  int                      visible);

void
chord_editor_space_widget_refresh (
  ChordEditorSpaceWidget * self);

void
chord_editor_space_widget_refresh_chords (
  ChordEditorSpaceWidget * self);

/**
 * @}
 */

#endif
