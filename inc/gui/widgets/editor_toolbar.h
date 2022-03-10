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

/**
 * \file
 */

#ifndef __GUI_WIDGETS_PIANO_ROLL_TOOLBAR_H__
#define __GUI_WIDGETS_PIANO_ROLL_TOOLBAR_H__

#include <gtk/gtk.h>

#define EDITOR_TOOLBAR_WIDGET_TYPE \
  (editor_toolbar_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  EditorToolbarWidget, editor_toolbar_widget,
  Z, EDITOR_TOOLBAR_WIDGET, GtkBox)

#define MW_EDITOR_TOOLBAR \
  MW_CLIP_EDITOR->editor_toolbar

typedef struct _ToolboxWidget ToolboxWidget;
typedef struct _QuantizeMbWidget QuantizeMbWidget;
typedef struct _QuantizeBoxWidget QuantizeBoxWidget;
typedef struct _SnapBoxWidget SnapBoxWidget;
typedef struct _ButtonWithMenuWidget
  ButtonWithMenuWidget;
typedef struct _PlayheadScrollButtonsWidget
  PlayheadScrollButtonsWidget;
typedef struct _VelocitySettingsWidget
  VelocitySettingsWidget;
typedef struct _ZoomButtonsWidget ZoomButtonsWidget;

/**
 * The PianoRoll toolbar in the top.
 */
typedef struct _EditorToolbarWidget
{
  GtkBox              parent_instance;
  GtkComboBoxText *   chord_highlighting;
  SnapBoxWidget *     snap_box;
  QuantizeBoxWidget * quantize_box;
  GtkButton *         event_viewer_toggle;
  GtkStack *          functions_btn_stack;
  ButtonWithMenuWidget * midi_functions_btn;
  ButtonWithMenuWidget * audio_functions_btn;
  ButtonWithMenuWidget * automation_functions_btn;
  GtkButton *         audio_apply_function_btn;
  GtkButton *         midi_apply_function_btn;
  GtkButton *         automation_apply_function_btn;

  GtkSeparator *      sep_after_chord_highlight;
  GtkBox *            chord_highlight_box;

  VelocitySettingsWidget * velocity_settings;
  GtkSeparator *      sep_after_velocity_settings;

  PlayheadScrollButtonsWidget * playhead_scroll;
  ZoomButtonsWidget * zoom_buttons;

  GMenuModel *        midi_functions_menu;
  GMenuModel *        automation_functions_menu;
  GMenuModel *        audio_functions_menu;
} EditorToolbarWidget;

/**
 * Refreshes relevant widgets.
 */
void
editor_toolbar_widget_refresh (
  EditorToolbarWidget * self);

void
editor_toolbar_widget_setup (
  EditorToolbarWidget * self);

#endif
