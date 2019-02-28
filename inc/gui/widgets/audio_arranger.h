/*
 * Copyright (C) 2018-2019 Alexandros Theodotou <alex at zrythm dot org>
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __GUI_WIDGETS_AUDIO_ARRANGER_H__
#define __GUI_WIDGETS_AUDIO_ARRANGER_H__

#include "gui/widgets/arranger.h"
#include "gui/widgets/main_window.h"
#include "audio/position.h"

#include <gtk/gtk.h>

#define AUDIO_ARRANGER_WIDGET_TYPE \
  (audio_arranger_widget_get_type ())
G_DECLARE_FINAL_TYPE (AudioArrangerWidget,
                      audio_arranger_widget,
                      Z,
                      AUDIO_ARRANGER_WIDGET,
                      ArrangerWidget)

#define AUDIO_ARRANGER MW_PIANO_ROLL->arranger

typedef struct _ArrangerBgWidget ArrangerBgWidget;

typedef struct _AudioArrangerWidget
{
  ArrangerWidget           parent_instance;

} AudioArrangerWidget;

/**
 * To be called from get_child_position in parent widget.
 *
 * Used to allocate the overlay children.
 */
void
audio_arranger_widget_set_allocation (
  AudioArrangerWidget * self,
  GtkWidget *          widget,
  GdkRectangle *       allocation);

/**
 * Shows context menu.
 *
 * To be called from parent on right click.
 */
void
audio_arranger_widget_show_context_menu (AudioArrangerWidget * self);

/**
 * Sets up the widget.
 */
void
audio_arranger_widget_setup (
  AudioArrangerWidget * self);

#endif
