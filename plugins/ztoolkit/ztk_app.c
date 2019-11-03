/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of ZPlugins
 *
 * ZPlugins is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * ZPlugins is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU General Affero Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "ztoolkit/ztk_app.h"
#include "ztoolkit/ztk_cairo.h"
#include "ztoolkit/ztk_widget.h"

#include <pugl/pugl.h>
#include <pugl/pugl_cairo_backend.h>

static void
on_close (PuglView* view)
{
	(void) view;
}

static void
on_expose (
  PuglView*               view,
  const PuglEventExpose * expose)
{
  ZtkApp * self = puglGetHandle (view);

  /** update each widget */
  ZtkWidget * w = NULL;
  for (int i = 0; i < self->num_widgets; i++)
    {
      w = self->widgets[i];
      w->update_cb (w);
    }

	cairo_t* cr = (cairo_t*)puglGetContext(view);

	// Draw background
	const PuglRect frame  = puglGetFrame(view);
	const double   width  = frame.width;
	const double   height = frame.height;
  ztk_color_set_for_cairo (
    &self->theme.bg_dark_variant2, cr);
	cairo_rectangle(cr, 0, 0, width, height);
	cairo_fill(cr);

  /* draw border */
  cairo_set_source_rgba (cr, 1,1,1, 0.4 );
  const double padding = 4.0;
  ztk_cairo_rounded_rectangle (
    cr, padding, padding, width - padding * 2,
    height - padding * 2, 1.0, 6.0);
	cairo_stroke (cr);

	// Draw label
  const char * lbl = "Z Series";
	cairo_text_extents_t extents;
	cairo_set_font_size(cr, 16.0);
	cairo_text_extents(cr, lbl, &extents);
	cairo_move_to(
    cr,
	  (width - extents.width) - 24.0,
	  padding * 2.3 + extents.height);
	cairo_set_source_rgba(cr, 1, 1, 1, 1);
	cairo_show_text(cr, lbl);

	// Scale to view size
	const double scaleX =
    (width - (self->width / width)) / self->width;
	const double scaleY =
    (height - (self->height / height)) /
      self->height;
	cairo_scale(cr, scaleX, scaleY);
  cairo_stroke (cr);

  ztk_app_draw (self, cr);
}

static void
post_event_to_widgets (
  ZtkApp *          self,
  const PuglEvent * event)
{
  ZtkWidget * w = NULL;
  for (int i = 0; i < self->num_widgets; i++)
    {
      w = self->widgets[i];
      switch (event->type)
        {
        case PUGL_KEY_PRESS:
        case PUGL_KEY_RELEASE:
          {
            const PuglEventKey * ev =
              (const PuglEventKey *) event;
            if (w->key_event_cb)
              {
                w->key_event_cb (w, ev);
              }
          }
          break;
        case PUGL_BUTTON_PRESS:
          {
            const PuglEventButton * ev =
              (const PuglEventButton *) event;
            if (ztk_widget_is_hit (
                  w, ev->x, ev->y))
              {
                w->state |=
                  ZTK_WIDGET_STATE_PRESSED;
                w->state |=
                  ZTK_WIDGET_STATE_SELECTED;
                if (w->button_event_cb)
                  {
                    w->button_event_cb (w, ev);
                  }
              }
            else
              {
                w->state &=
                  (unsigned int)
                  ~ZTK_WIDGET_STATE_SELECTED;
              }
          }
          break;
        case PUGL_BUTTON_RELEASE:
          {
            const PuglEventButton * ev =
              (const PuglEventButton *) event;
            w->state &=
              (unsigned int)
              ~ZTK_WIDGET_STATE_PRESSED;
            if (w->button_event_cb)
              {
                w->button_event_cb (w, ev);
              }
          }
          break;
        case PUGL_MOTION_NOTIFY:
          {
            const PuglEventMotion * ev =
              (const PuglEventMotion *) event;
            if (ztk_widget_is_hit (
                  w, ev->x, ev->y))
              {
                w->state |=
                  ZTK_WIDGET_STATE_HOVERED;
                if (w->motion_event_cb)
                  {
                    w->motion_event_cb (w, ev);
                  }
              }
            else
              {
                w->state &=
                  (unsigned int)
                  ~ZTK_WIDGET_STATE_HOVERED;
              }
          }
          break;
        case PUGL_SCROLL:
          {
            const PuglEventScroll * ev =
              (const PuglEventScroll *) event;
            if (ztk_widget_is_hit (
                  w, ev->x, ev->y) &&
                w->scroll_event_cb)
              {
                w->scroll_event_cb (w, ev);
              }
          }
          break;
        default:
          break;
        }
    }
}

#undef POST_EVENT_FUNC

static PuglStatus
on_event (
  PuglView *        view,
  const PuglEvent * event)
{
  ZtkApp * self = puglGetHandle (view);

  post_event_to_widgets (self, event);

	switch (event->type)
    {
    case PUGL_BUTTON_PRESS:
      {
        const PuglEventButton * ev =
          (const PuglEventButton *) event;
        self->pressing = 1;
        self->start_press_x = ev->x;
        self->start_press_y = ev->y;
        self->prev_press_x = ev->x;
        self->prev_press_y = ev->y;
        self->offset_press_x = ev->x;
        self->offset_press_y = ev->y;
      }
      puglPostRedisplay(view);
      break;
    case PUGL_BUTTON_RELEASE:
      {
        /*const PuglEventButton * ev =*/
          /*(const PuglEventButton *) event;*/
        self->pressing = 0;
        self->start_press_x = 0;
        self->start_press_y = 0;
        self->prev_press_x = 0;
        self->prev_press_y = 0;
        self->offset_press_x = 0;
        self->offset_press_y = 0;
      }
      puglPostRedisplay(view);
      break;
    case PUGL_MOTION_NOTIFY:
      {
        const PuglEventMotion * ev =
          (const PuglEventMotion *) event;
        if (self->pressing)
          {
            self->prev_press_x =
              self->offset_press_x;
            self->prev_press_y =
              self->offset_press_y;
            self->offset_press_x = ev->x;
            self->offset_press_y = ev->y;
          }
      }
      puglPostRedisplay(view);
      break;
    case PUGL_ENTER_NOTIFY:
    case PUGL_LEAVE_NOTIFY:
    case PUGL_SCROLL:
    case PUGL_KEY_PRESS:
    case PUGL_KEY_RELEASE:
      puglPostRedisplay(view);
      break;
    case PUGL_EXPOSE:
      on_expose (view, &event->expose);
      break;
    case PUGL_CLOSE:
      on_close(view);
      break;
    default:
      puglPostRedisplay(view);
      break;
    }

	return PUGL_SUCCESS;
}

/**
 * Creates a new ZtkApp.
 *
 * @param parent Parent window, if any.
 */
ZtkApp *
ztk_app_new (
  PuglWorld *      world,
  const char *     title,
  PuglNativeWindow parent,
  int              width,
  int              height)
{
  ZtkApp * self = calloc (1, sizeof (ZtkApp));

  ztk_theme_init (&self->theme);

  self->world = world;
  self->title = strdup (title);
  /*self->parent = parent;*/
  self->width = width;
  self->height = height;
  self->widgets = calloc (1, sizeof (ZtkWidget *));
  self->widgets_size = 1;

	puglSetClassName (self->world, title);
	PuglRect frame = { 0, 0, width, height };
	self->view  = puglNewView (self->world);
	puglSetFrame (self->view, frame);
	puglSetMinSize (self->view, width, height);
	puglSetViewHint (
    self->view, PUGL_RESIZABLE, 0);
	puglSetBackend (self->view, puglCairoBackend());
  puglSetHandle (self->view, self);
	puglSetViewHint (
    self->view, PUGL_IGNORE_KEY_REPEAT, 1);
	puglSetEventFunc (self->view, on_event);

  if (parent)
    {
      puglSetParentWindow (
        self->view, parent);
    }

	if (puglCreateWindow (self->view, "Pugl Test"))
    {
      printf ("error, can't create window\n");
    }

	puglShowWindow (self->view);

  return self;
}

/**
 * Adds a widget with the given Z axis.
 */
void
ztk_app_add_widget (
  ZtkApp *    self,
  ZtkWidget * widget,
  int         z)
{
  if (self->num_widgets == self->widgets_size)
    {
      self->widgets_size = self->widgets_size * 2;
      self->widgets =
        (ZtkWidget **)
        realloc (
          self->widgets,
          (size_t) self->widgets_size *
            sizeof (ZtkWidget *));
    }
  self->widgets[self->num_widgets++] = widget;
  widget->app = self;
  widget->z = z;

  /* TODO sort by z */
}

/**
 * Removes the given widget from the app.
 */
void
ztk_app_remove_widget (
  ZtkApp *    self,
  ZtkWidget * widget)
{
  for (int i = self->num_widgets - 1;
       i >= 0; i--)
    {
      ZtkWidget * w = self->widgets[i];
      if (w == widget)
        {
          for (int j = i; j < self->num_widgets - 1;
               j++)
            {
              self->widgets[j] = self->widgets[j + 1];
            }
          break;
        }
    }
  self->num_widgets--;
}

/**
 * Draws each widget.
 */
void
ztk_app_draw (
  ZtkApp *  self,
  cairo_t * cr)
{
  for (int i = 0; i < self->num_widgets; i++)
    {
      ZtkWidget * widget = self->widgets[i];
      ztk_widget_draw (widget, cr);
    }
}

void
ztk_app_idle (
  ZtkApp * self)
{
  puglPollEvents (self->world, 0);
  puglDispatchEvents (self->world);
}

/**
 * Shows the window.
 */
void
ztk_app_show_window (
  ZtkApp * self)
{
  puglShowWindow (self->view);
}

/**
 * Hides the window.
 */
void
ztk_app_hide_window (
  ZtkApp * self)
{
  puglHideWindow (self->view);
}

/**
 * Frees the app.
 */
void
ztk_app_free (
  ZtkApp * self)
{
	puglFreeView (self->view);
	puglFreeWorld (self->world);

  if (self->title)
    free (self->title);

  free (self);
}
