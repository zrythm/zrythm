/*
 * gui/widgets/velocity.c - velocity for MIDI notes
 *
 * Copyright (C) 2019 Alexandros Theodotou
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

/** \file */

#include "gui/widgets/velocity.h"

G_DEFINE_TYPE (VelocityWidget,
               velocity_widget,
               GTK_TYPE_BOX)


/**
 * Creates a velocity.
 */
VelocityWidget *
velocity_widget_new (Velocity * velocity)
{
  VelocityWidget * self =
    g_object_new (VELOCITY_WIDGET_TYPE,
                  NULL);

  self->velocity = velocity;

  return self;
}

static void
velocity_widget_class_init (VelocityWidgetClass * klass)
{
}

static void
velocity_widget_init (VelocityWidget * self)
{
  gtk_widget_add_events (GTK_WIDGET (self), GDK_ALL_EVENTS_MASK);


  /* connect signals */
  /*g_signal_connect (G_OBJECT (self), "draw",*/
                    /*G_CALLBACK (draw_cb), self);*/
  /*g_signal_connect (G_OBJECT (self), "enter-notify-event",*/
                    /*G_CALLBACK (on_motion),  self);*/
  /*g_signal_connect (G_OBJECT(self), "leave-notify-event",*/
                    /*G_CALLBACK (on_motion),  self);*/
  /*g_signal_connect (G_OBJECT(self), "motion-notify-event",*/
                    /*G_CALLBACK (on_motion),  self);*/

  /* set tooltip */
  gtk_widget_set_tooltip_text (GTK_WIDGET (self),
                               "Velocity");
}
