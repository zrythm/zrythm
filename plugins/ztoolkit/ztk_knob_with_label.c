/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU General Affero Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <stdlib.h>

#include "ztoolkit/ztk_app.h"
#include "ztoolkit/ztk_knob_with_label.h"
#include "ztoolkit/ztk_knob.h"
#include "ztoolkit/ztk_label.h"

static void
ztk_knob_with_label_draw_cb (
  ZtkWidget * widget,
  cairo_t *   cr)
{
  ZtkKnobWithLabel * self =
    (ZtkKnobWithLabel *) widget;
  ZtkWidget * knob_w = (ZtkWidget *) self->knob;
  knob_w->draw_cb (knob_w, cr);
  ZtkWidget * label_w = (ZtkWidget *) self->label;
  label_w->draw_cb (label_w, cr);
}

static void
ztk_knob_with_label_update_cb (
  ZtkWidget * widget)
{
  ZtkKnobWithLabel * self =
    (ZtkKnobWithLabel *) widget;
  ZtkWidget * knob_w = (ZtkWidget *) self->knob;
  knob_w->update_cb (knob_w);
  ZtkWidget * label_w = (ZtkWidget *) self->label;
  label_w->update_cb (label_w);
}

static void
ztk_knob_with_label_free (
  ZtkWidget * widget)
{
  ZtkKnobWithLabel * self =
    (ZtkKnobWithLabel *) widget;

  free (self);
}

/**
 * Creates a new knob with a label on top.
 *
 * @param rect The rectangle to draw this in. The
 *   knob and label will be adjusted so that they fit
 *   this exactly.
 */
ZtkKnobWithLabel *
ztk_knob_with_label_new (
  PuglRect * rect,
  ZtkKnob *  knob,
  ZtkLabel * label)
{
  ZtkKnobWithLabel * self =
    calloc (1, sizeof (ZtkKnobWithLabel));

  ztk_widget_init (
    (ZtkWidget *) self, rect,
    ztk_knob_with_label_update_cb,
    ztk_knob_with_label_draw_cb,
    ztk_knob_with_label_free);

  self->knob = knob;
  self->label = label;

  /* calculate the rectangles of the knob and the
   * label */

  /* label */
  cairo_t * cr =
    (cairo_t *)
    puglGetContext (
      ((ZtkWidget *) knob)->app->view);
	cairo_text_extents_t extents;
	cairo_set_font_size (cr, label->font_size);
	cairo_text_extents (cr, label->label, &extents);
  ZtkWidget * w = (ZtkWidget *) label;
  w->rect.x =
    rect->x +
    (rect->width / 2.0 - extents.width / 2.0);
  w->rect.y = rect->y;

  /* knob */
#define PADDING 2.0
  w = (ZtkWidget *) knob;
  w->rect.x = rect->x;
  w->rect.y = rect->y + extents.height + PADDING;
  w->rect.width = rect->width;
  w->rect.height =
    rect->height - (extents.height + PADDING);

  return self;
}
