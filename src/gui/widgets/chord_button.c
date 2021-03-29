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
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "gui/widgets/chord_button.h"

G_DEFINE_TYPE (ChordButtonWidget,
               chord_button_widget,
               GTK_TYPE_OVERLAY)

/*static gboolean*/
/*chord_button_draw_cb (*/
  /*ChordButtonWidget * self,*/
  /*cairo_t *cr,*/
  /*gpointer data)*/
/*{*/
  /*guint width, height;*/
  /*GtkStyleContext *context;*/

  /*context =*/
    /*gtk_widget_get_style_context (*/
      /*GTK_WIDGET (self));*/

  /*width =*/
    /*gtk_widget_get_allocated_width (*/
      /*GTK_WIDGET (self));*/
  /*height =*/
    /*gtk_widget_get_allocated_height (*/
      /*GTK_WIDGET (self));*/

  /*gtk_render_background (*/
    /*context, cr, 0, 0, width, height);*/

 /*return FALSE;*/
/*}*/

ChordButtonWidget *
chord_button_widget_new ()
{
  ChordButtonWidget * self =
    g_object_new (CHORD_BUTTON_WIDGET_TYPE,
                  NULL);

  return self;
}

static void
chord_button_widget_class_init (
  ChordButtonWidgetClass * klass)
{
}

static void
chord_button_widget_init (
  ChordButtonWidget * self)
{
}
