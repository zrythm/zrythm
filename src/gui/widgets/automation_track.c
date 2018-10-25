/*
 * gui/widgets/automation_track.c - AutomationTrack
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

/** \file
 */

#include "audio/automation_track.h"
#include "gui/widgets/automation_track.h"
#include "gui/widgets/main_window.h"

G_DEFINE_TYPE (AutomationTrackWidget, automation_track_widget, GTK_TYPE_GRID)

static void
size_allocate_cb (GtkWidget * widget, GtkAllocation * allocation, void * data)
{
  gtk_widget_queue_draw (GTK_WIDGET (MAIN_WINDOW->timeline));
  gtk_widget_queue_allocate (GTK_WIDGET (MAIN_WINDOW->timeline));
}

static void
on_add_lane_clicked (GtkWidget * widget, void * data)
{

}

/**
 * Creates a new Fader widget and binds it to the given value.
 */
AutomationTrackWidget *
automation_track_widget_new (AutomationTrack * automation_track)
{
  AutomationTrackWidget * self = g_object_new (
                            AUTOMATION_TRACK_WIDGET_TYPE,
                            NULL);
  /*self->track = track;*/

  /*self->value = digital_meter_widget_new (DIGITAL_METER_TYPE_VALUE,*/
                                          /*NULL,*/

  /*gtk_box_pack_start (self->value_box,*/
                      /*GTK_WIDGET (self->color),*/
                      /*1,*/
                      /*1,*/
                      /*0);*/

  GtkWidget *image = gtk_image_new_from_resource (
          "/online/alextee/zrythm/mute.svg");
  gtk_button_set_image (GTK_BUTTON (self->mute_toggle), image);
  gtk_button_set_label (GTK_BUTTON (self->mute_toggle),
                        "");
  g_signal_connect (self, "size-allocate",
                    G_CALLBACK (size_allocate_cb), NULL);

  return self;
}

static void
automation_track_widget_init (AutomationTrackWidget * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}

static void
automation_track_widget_class_init (AutomationTrackWidgetClass * klass)
{
  gtk_widget_class_set_template_from_resource (GTK_WIDGET_CLASS (klass),
                                               "/online/alextee/zrythm/ui/automation_track.ui");

  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                        AutomationTrackWidget,
                                        selector);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                        AutomationTrackWidget,
                                        value_box);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                        AutomationTrackWidget,
                                        mute_toggle);
  gtk_widget_class_bind_template_callback (GTK_WIDGET_CLASS (klass),
                                        on_add_lane_clicked);
}



