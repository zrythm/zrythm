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

#include "zrythm.h"
#include "project.h"
#include "settings/settings.h"
#include "audio/transport.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/clip_editor.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/audio_arranger.h"
#include "gui/widgets/audio_arranger_bg.h"
#include "gui/widgets/audio_ruler.h"
#include "gui/widgets/ruler.h"
#include "gui/widgets/piano_roll.h"
#include "gui/widgets/piano_roll_labels.h"
#include "gui/widgets/ruler.h"
#include "gui/widgets/tracklist.h"
#include "utils/cairo.h"

#include <gtk/gtk.h>

G_DEFINE_TYPE (AudioArrangerBgWidget,
               audio_arranger_bg_widget,
               ARRANGER_BG_WIDGET_TYPE)

static void
draw_audio_clip (GtkWidget * self,
                 cairo_t * cr,
                 GdkRectangle * rect)
{
  AudioRegion * ar =
    (AudioRegion *) CLIP_EDITOR->region;
  GdkRGBA * color =
    &CLIP_EDITOR->region->track->color;
  cairo_set_source_rgba (cr,
                         color->red + 0.3,
                         color->green + 0.3,
                         color->blue + 0.3,
                         0.9);
  cairo_set_line_width (cr, 1);

  guint height =
    gtk_widget_get_allocated_height (
      GTK_WIDGET (self));

  guint width =
    gtk_widget_get_allocated_width (
      GTK_WIDGET (self));

  long frame_interval =
    ui_px_to_frames_audio_clip_editor (ar->channels, 0);

  long prev_frames = 0;
  for (double i = SPACE_BEFORE_START_D;
       i < (double) width; i+= 0.6)
    {
      long curr_frames =
        i * frame_interval;
        /*ui_px_to_frames_audio_clip_editor (*/
          /*i * 2.0, 1);*/
      /*g_message ("curr frames %ld, total frames %ld",*/
                 /*curr_frames,*/
                 /*ar->buff_size);*/
      if (curr_frames >= ar->buff_size)
        return;
      /*Position pos;*/
      /*position_init (&pos);*/
      /*position_add_frames (&pos, curr_frames);*/
      /*position_print (&pos);*/
      float min = 0, max = 0;
      for (int j = prev_frames; j < curr_frames; j++)
        {
          float val = ar->buff[j];
          if (val > max)
            max = val;
          if (val < min)
            min = val;
        }
      min = (min + 1.0) / 2.0; /* normallize */
      max = (max + 1.0) / 2.0; /* normalize */
      z_cairo_draw_vertical_line (
        cr,
        i,
        MAX (min * height, 0),
        MIN (max * height, height));

      prev_frames = curr_frames;
    }
}

static gboolean
audio_arranger_bg_draw_cb (
  GtkWidget *widget, cairo_t *cr, gpointer data)
{
  AudioArrangerBgWidget * self =
    Z_AUDIO_ARRANGER_BG_WIDGET (widget);

  GdkRectangle rect;
  gdk_cairo_get_clip_rectangle (cr,
                                &rect);

  /*g_message ("rect %d %d %d %d",*/
             /*rect.x, rect.y,*/
             /*rect.width, rect.height);*/

  /*GtkStyleContext *context;*/
  /*context =*/
    /*gtk_widget_get_style_context (GTK_WIDGET (self));*/
  /*gtk_render_background (*/
    /*context, cr,*/
    /*rect.x, rect.y, rect.width, rect.height);*/

  draw_audio_clip (widget, cr, &rect);

  return 0;
}

AudioArrangerBgWidget *
audio_arranger_bg_widget_new (RulerWidget *    ruler,
                             ArrangerWidget * arranger)
{
  AudioArrangerBgWidget * self =
    g_object_new (AUDIO_ARRANGER_BG_WIDGET_TYPE,
                  NULL);

  ARRANGER_BG_WIDGET_GET_PRIVATE (self);
  ab_prv->ruler = ruler;
  ab_prv->arranger = arranger;

  g_signal_connect (
    G_OBJECT (self), "draw",
    G_CALLBACK (audio_arranger_bg_draw_cb), NULL);

  return self;
}

static void
audio_arranger_bg_widget_class_init (AudioArrangerBgWidgetClass * _klass)
{
}

static void
audio_arranger_bg_widget_init (AudioArrangerBgWidget *self )
{
}

