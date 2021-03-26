/*
 * Copyright (C) 2020 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, version 3.
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
 * Legend for AutomationArranger.
 */

#ifndef __GUI_WIDGETS_AUTOMATION_EDITOR_LEGEND_H__
#define __GUI_WIDGETS_AUTOMATION_EDITOR_LEGEND_H__

#include <gtk/gtk.h>

#define AUTOMATION_EDITOR_LEGEND_WIDGET_TYPE \
  (automation_editor_legend_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  AutomationEditorLegendWidget,
  automation_editor_legend_widget,
  Z, AUTOMATION_EDITOR_LEGEND_WIDGET,
  GtkDrawingArea)

/**
 * @addtogroup widgets
 *
 * @{
 */

#define MW_AUTOMATION_EDITOR_LEGEND \
  MW_MIDI_EDITOR_SPACE->automation_editor_legend

/**
 * The legend on the left side of the
 * AutomationArranger.
 */
typedef struct _AutomationEditorLegendWidget
{
  GtkDrawingArea     parent_instance;

  /** Cache layout for drawing text. */
  PangoLayout *      layout;

  GtkGestureMultiPress * multipress;
} AutomationEditorLegendWidget;

void
automation_editor_legend_widget_refresh (
  AutomationEditorLegendWidget * self);

void
automation_editor_legend_widget_setup (
  AutomationEditorLegendWidget * self);

/**
 * @}
 */

#endif
