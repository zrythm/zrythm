/*
 * gui/widgets/track.c - Track
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

#include "audio/automatable.h"
#include "audio/automation_track.h"
#include "audio/track.h"
#include "audio/region.h"
#include "gui/widgets/automation_track.h"
#include "gui/widgets/color_area.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/track.h"

G_DEFINE_TYPE (TrackWidget, track_widget, GTK_TYPE_GRID)

static void
size_allocate_cb (GtkWidget * widget, GtkAllocation * allocation, void * data)
{
  gtk_widget_queue_draw (GTK_WIDGET (MAIN_WINDOW->timeline));
  /*TrackWidget * track_widget = TRACK_WIDGET (widget);*/
  /*for (int i = 0; i < track_widget->track->num_regions; i++)*/
    /*{*/
      /*gtk_widget_queue_resize (GTK_WIDGET (track_widget->track->regions[i]->widget));*/
    /*}*/
  gtk_widget_queue_allocate (GTK_WIDGET (MAIN_WINDOW->timeline));
}

static void
on_show_automation (GtkWidget * widget, void * data)
{
  TrackWidget * self = (TrackWidget *) data;
  if (!self->automation_paned)
    {
      self->automation_paned = GTK_PANED (gtk_paned_new (GTK_ORIENTATION_VERTICAL));

      AutomationTrack * at =
        automation_track_new (self->track,
                              track_get_fader_automatable (self->track));
      AutomationTrackWidget * automation_lane =
        automation_track_widget_new (at);
      gtk_paned_pack1 (self->automation_paned,
                      GTK_WIDGET (automation_lane),
                      1,
                      0);
      gtk_box_pack_start (self->automation_box,
                          GTK_WIDGET (self->automation_paned),
                          0,
                          0,
                          0);
      gtk_widget_show_all (GTK_WIDGET (self->automation_box));
    }
  else if (gtk_widget_get_visible (GTK_WIDGET (self->automation_paned)))
    {
      gtk_widget_set_visible (GTK_WIDGET (self->automation_paned), 0);
    }
  else
    {
      gtk_widget_set_visible (GTK_WIDGET (self->automation_paned), 1);
    }
}

/**
 * Creates a new Fader widget and binds it to the given value.
 */
TrackWidget *
track_widget_new (Track * track)
{
  TrackWidget * self = g_object_new (
                            TRACK_WIDGET_TYPE,
                            NULL);
  self->track = track;

  self->color = color_area_widget_new (&track->channel->color,
                                       5,
                                       -1);
  gtk_box_pack_start (self->color_box,
                      GTK_WIDGET (self->color),
                      1,
                      1,
                      0);

  track_widget_update_all (self);

  GtkWidget * image = gtk_image_new_from_resource (
          "/online/alextee/zrythm/solo.svg");
  gtk_button_set_image (self->solo, image);
  gtk_button_set_label (self->solo,
                        "");
  image = gtk_image_new_from_resource (
          "/online/alextee/zrythm/mute.svg");
  gtk_button_set_image (self->mute, image);
  gtk_button_set_label (self->mute,
                        "");
  /*image = gtk_image_new_from_resource (*/
          /*"/online/alextee/zrythm/record.svg");*/
  gtk_button_set_image (GTK_BUTTON (self->record),
                        gtk_image_new_from_icon_name ("gtk-media-record",
                                                      GTK_ICON_SIZE_BUTTON));
  gtk_widget_set_size_request (GTK_WIDGET (self->record),
                               16,
                               16);
  gtk_button_set_label (self->record,
                        "");
  gtk_button_set_image (
            self->show_automation,
            gtk_image_new_from_icon_name ("gtk-justify-fill",
                                          GTK_ICON_SIZE_BUTTON));
  gtk_button_set_label (self->show_automation,
                        "");
  gtk_image_set_from_resource (self->icon,
                               "/online/alextee/zrythm/instrument.svg");

  g_signal_connect (self, "size-allocate",
                    G_CALLBACK (size_allocate_cb), NULL);
  g_signal_connect (self->show_automation, "clicked",
                    G_CALLBACK (on_show_automation), self);

  return self;
}

static void
track_widget_init (TrackWidget * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}

static void
track_widget_class_init (TrackWidgetClass * klass)
{
  gtk_widget_class_set_template_from_resource (GTK_WIDGET_CLASS (klass),
                                               "/online/alextee/zrythm/ui/track.ui");

  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                                TrackWidget, color_box);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                                TrackWidget, track_name);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                                TrackWidget, record);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                                TrackWidget, solo);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                                TrackWidget, mute);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                        TrackWidget,
                                        show_automation);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                        TrackWidget,
                                        automation_box);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                                TrackWidget, icon);
}

void
track_widget_update_all (TrackWidget * self)
{
  gtk_label_set_text (self->track_name, self->track->channel->name);
}
