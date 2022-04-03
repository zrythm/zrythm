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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * \file
 *
 * Automation editor space.
 */

#ifndef __GUI_WIDGETS_AUTOMATION_EDITOR_SPACE_H__
#define __GUI_WIDGETS_AUTOMATION_EDITOR_SPACE_H__

#include <gtk/gtk.h>

#define AUTOMATION_EDITOR_SPACE_WIDGET_TYPE \
  (automation_editor_space_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  AutomationEditorSpaceWidget,
  automation_editor_space_widget,
  Z,
  AUTOMATION_EDITOR_SPACE_WIDGET,
  GtkBox)

typedef struct _ArrangerWidget ArrangerWidget;
typedef struct _AutomationEditorLegendWidget
  AutomationEditorLegendWidget;

/**
 * @addtogroup widgets
 *
 * @{
 */

#define MW_AUTOMATION_EDITOR_SPACE \
  MW_CLIP_EDITOR_INNER->automation_editor_space

/**
 * The piano roll widget is the whole space inside
 * the clip editor tab when a AUTOMATION region is selected.
 */
typedef struct _AutomationEditorSpaceWidget
{
  GtkBox parent_instance;

  /** The box dividing the chord keys and the
   * arranger. */
  GtkBox * left_slash_arranger_box;

  /** The box on the left of the arranger containing
   * the chord keys. */
  GtkBox * left_box;

  /** The arranger. */
  ArrangerWidget *    arranger;
  GtkScrolledWindow * arranger_scroll;
  GtkViewport *       arranger_viewport;

  /* The legend on the left side. */
  AutomationEditorLegendWidget * legend;
} AutomationEditorSpaceWidget;

void
automation_editor_space_widget_setup (
  AutomationEditorSpaceWidget * self);

/**
 * See CLIP_EDITOR_INNER_WIDGET_ADD_TO_SIZEGROUP.
 */
void
automation_editor_space_widget_update_size_group (
  AutomationEditorSpaceWidget * self,
  int                           visible);

void
automation_editor_space_widget_refresh (
  AutomationEditorSpaceWidget * self);

/**
 * @}
 */

#endif
