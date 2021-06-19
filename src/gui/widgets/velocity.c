/*
 * Copyright (C) 2019-2021 Alexandros Theodotou <alex at zrythm dot org>
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

#include <math.h>

#include "audio/channel.h"
#include "audio/channel_track.h"
#include "audio/chord_track.h"
#include "audio/midi_note.h"
#include "audio/region.h"
#include "gui/backend/midi_arranger_selections.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/bot_bar.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/clip_editor.h"
#include "gui/widgets/midi_modifier_arranger.h"
#include "gui/widgets/midi_note.h"
#include "gui/widgets/velocity.h"
#include "project.h"
#include "utils/cairo.h"
#include "utils/color.h"
#include "utils/flags.h"
#include "utils/ui.h"
#include "zrythm_app.h"

/**
 * Draws the Velocity in the given cairo context in
 * relative coordinates.
 *
 * @param cr The arranger cairo context.
 * @param rect Arranger rectangle.
 */
void
velocity_draw (
  Velocity *     self,
  cairo_t *      cr,
  GdkRectangle * rect)
{
  ArrangerObject * obj = (ArrangerObject *) self;
  MidiNote * mn = velocity_get_midi_note (self);

  /* get color */
  GdkRGBA color;
  midi_note_get_adjusted_color (mn, &color);
  gdk_cairo_set_source_rgba (cr, &color);

  /* make velocity start at 0,0 to make it easier to
   * draw */
  cairo_save (cr);
  cairo_translate (
    cr,
    obj->full_rect.x - rect->x,
    obj->full_rect.y - rect->y);

#if 0
  cairo_rectangle (
    cr, 0, 0,
    obj->full_rect.width,
    obj->full_rect.height);
  cairo_stroke(cr);
#endif

  /* --- draw --- */

  const int circle_radius = obj->full_rect.width / 2;

  /* draw line */
  cairo_rectangle (
    cr,
    obj->full_rect.width / 2 -
      VELOCITY_LINE_WIDTH / 2,
    circle_radius,
    VELOCITY_LINE_WIDTH,
    obj->full_rect.height);
  cairo_fill (cr);

  /* draw circle */
  cairo_arc (
    cr, circle_radius, circle_radius, circle_radius,
    0, 2 * M_PI);
  cairo_set_source_rgba (cr, 0.8, 0.8, 0.8, 1);
  cairo_fill_preserve (cr);

  gdk_cairo_set_source_rgba (cr, &color);
  cairo_set_line_width (cr, 2);
  cairo_stroke (cr);

  cairo_restore (cr);
}


#if 0
/**
 * Creates a velocity.
 */
VelocityWidget *
velocity_widget_new (Velocity * velocity)
{
  VelocityWidget * self =
    g_object_new (VELOCITY_WIDGET_TYPE,
                  NULL);

  arranger_object_widget_setup (
    Z_ARRANGER_OBJECT_WIDGET (self),
    (ArrangerObject *) velocity);

  self->velocity = velocity;

  return self;
}
#endif
