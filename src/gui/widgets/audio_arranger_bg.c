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

#include "zrythm.h"
#include "project.h"
#include "settings/settings.h"
#include "audio/audio_region.h"
#include "audio/clip.h"
#include "audio/engine.h"
#include "audio/pool.h"
#include "audio/transport.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/clip_editor.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/audio_arranger.h"
#include "gui/widgets/audio_arranger_bg.h"
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
  /*g_message ("start");*/
  ZRegion * ar =
    (ZRegion *) CLIP_EDITOR->region;
  GdkRGBA * color =
    &CLIP_EDITOR->region->lane->track->color;
  cairo_set_source_rgba (cr,
                         color->red + 0.3,
                         color->green + 0.3,
                         color->blue + 0.3,
                         0.9);
  cairo_set_line_width (cr, 1);

  int height =
    gtk_widget_get_allocated_height (
      GTK_WIDGET (self));

  int width =
    gtk_widget_get_allocated_width (
      GTK_WIDGET (self));

  AudioClip * clip =
    AUDIO_POOL->clips[ar->pool_id];
  long frame_interval =
    ui_px_to_frames_editor (clip->channels, 0);

  long prev_frames = 0;
  for (double i = SPACE_BEFORE_START_D;
       i < (double) width; i+= 0.6)
    {
      long curr_frames =
        (long) i * frame_interval;
        /*ui_px_to_frames_audio_clip_editor (*/
          /*i * 2.0, 1);*/
      /*g_message ("curr frames %ld, total frames %ld",*/
                 /*curr_frames,*/
                 /*ar->buff_size);*/
      if (curr_frames >=
          clip->num_frames * clip->channels)
        return;
      /*Position pos;*/
      /*position_init (&pos);*/
      /*position_add_frames (&pos, curr_frames);*/
      /*position_print_yaml (&pos);*/
      float min = 0, max = 0;
      for (long j = prev_frames;
           j < curr_frames; j++)
        {
          float val = clip->frames[j];
          if (val > max)
            max = val;
          if (val < min)
            min = val;
          break;
        }
      min = (min + 1.f) / 2.f; /* normallize */
      max = (max + 1.f) / 2.f; /* normalize */
      z_cairo_draw_vertical_line (
        cr,
        i,
        MAX (
          (double) min * (double) height, 0.0),
        MIN (
          (double) max * (double) height,
          (double) height));

      prev_frames = curr_frames;
      /*g_message ("i %f width %f",*/
                 /*i, (double) width);*/
    }
  /*g_message ("end");*/
}

static gboolean
audio_arranger_bg_draw_cb (
  GtkWidget *widget, cairo_t *cr, gpointer data)
{
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

