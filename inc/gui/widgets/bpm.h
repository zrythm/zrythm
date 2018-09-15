/*
 * gui/widgets/bpm.h - BPM widget
 *
 * Copyright (C) 2018 Alexandros Theodotou
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

#ifndef __GUI_WIDGETS_BPM_H__
#define __GUI_WIDGETS_BPM_H__

#include <gtk/gtk.h>

#define BPM_WIDGET_TYPE                  (bpm_widget_get_type ())
#define BPM_WIDGET(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), BPM_WIDGET_TYPE, BpmWidget))
#define BPM_WIDGET_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST  ((klass), BPM_WIDGET, BpmWidgetClass))
#define IS_BPM_WIDGET(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BPM_WIDGET_TYPE))
#define IS_BPM_WIDGET_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE  ((klass), BPM_WIDGET_TYPE))
#define BPM_WIDGET_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS  ((obj), BPM_WIDGET_TYPE, BpmWidgetClass))

typedef struct BpmWidget
{
  GtkSpinButton            parent_instance;
} BpmWidget;

typedef struct BpmWidgetClass
{
  GtkSpinButtonClass       parent_class;
} BpmWidgetClass;

/**
 * Creates bpm widget.
 */
BpmWidget *
bpm_widget_new ();

#endif /* __GUI_WIDGETS_BPM_H__*/
