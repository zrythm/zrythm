/*
 * gui/widgets/channel.c - A channel widget
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

#include "zrythm_app.h"
#include "audio/channel.h"
#include "gui/widgets/channel.h"
#include "gui/widgets/channel_color.h"
#include "gui/widgets/channel_meter.h"
#include "gui/widgets/fader.h"
#include "gui/widgets/knob.h"
#include "gui/widget_manager.h"

#include <gtk/gtk.h>

G_DEFINE_TYPE (ChannelWidget, channel_widget, GTK_TYPE_GRID)

static void
phase_invert_button_clicked (ChannelWidget * self,
                             GtkButton     * button)
{

}

static void
channel_widget_class_init (ChannelWidgetClass * klass)
{
  gtk_widget_class_set_template_from_resource (GTK_WIDGET_CLASS (klass),
                                               "/online/alextee/zrythm/ui/channel.ui");

  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                                ChannelWidget, output);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                                ChannelWidget, name);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                                ChannelWidget, phase_invert);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                                ChannelWidget, phase_reading);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                                ChannelWidget, phase_controls);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                                ChannelWidget, slots);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                                ChannelWidget, slot1b);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                                ChannelWidget, slot2b);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                                ChannelWidget, add_slot);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                                ChannelWidget, pan);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                                ChannelWidget, e);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                                ChannelWidget, solo);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                                ChannelWidget, listen);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                                ChannelWidget, mute);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                                ChannelWidget, record);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                                ChannelWidget, fader_area);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                                ChannelWidget, meter_area);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                                ChannelWidget, meter_reading);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                                ChannelWidget, icon);
  gtk_widget_class_bind_template_callback (GTK_WIDGET_CLASS (klass),
                                           phase_invert_button_clicked);

}

static void
channel_widget_init (ChannelWidget * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}


static void
setup_color (ChannelWidget * self)
{
  self->color = channel_color_widget_new (&self->channel->color);
  gtk_grid_attach (&self->parent_instance,
                   GTK_WIDGET (self->color),
                   0, 1, 2, 1);
}

static void
setup_phase_panel (ChannelWidget * self)
{
  self->phase_knob = knob_widget_new (channel_get_phase,
                                      channel_set_phase,
                                      self->channel,
                                      0,
                                      180,
                                         30,
                                         0.0f);
  gtk_box_pack_end (self->phase_controls,
                       GTK_WIDGET (self->phase_knob),
                       0, 1, 0);
  gtk_label_set_text (self->phase_reading,
                      g_strdup_printf ("%.1f", self->channel->phase));
}

static void
setup_fader (ChannelWidget * self)
{
  self->fader = fader_widget_new (channel_get_volume,
                                  channel_set_volume,
                                  self->channel,
                                  40);
  gtk_box_pack_start (self->fader_area,
                       GTK_WIDGET (self->fader),
                       0, 1, 0);
}

static void
setup_meter (ChannelWidget * self)
{
  self->cm = channel_meter_widget_new (channel_get_current_l_db,
                                       channel_get_current_r_db,
                                  self->channel,
                                  12);
  gtk_box_pack_start (self->meter_area,
                       GTK_WIDGET (self->cm),
                       1, 1, 0);
}

ChannelWidget *
channel_widget_new (Channel * channel)
{
  ChannelWidget * self = g_object_new (CHANNEL_WIDGET_TYPE, NULL);
  self->channel = channel;
  channel->widget = self;

  setup_color (self);
  setup_phase_panel (self);
  /*setup_pan (self);*/
  setup_fader (self);
  setup_meter (self);

  GtkWidget * image = gtk_image_new_from_resource (
          "/online/alextee/zrythm/plus.svg");
  gtk_button_set_image (self->add_slot, image);
  image = gtk_image_new_from_resource (
          "/online/alextee/zrythm/listen.svg");
  gtk_button_set_image (self->listen, image);
  image = gtk_image_new_from_resource (
          "/online/alextee/zrythm/solo.svg");
  gtk_button_set_image (self->solo, image);
  image = gtk_image_new_from_resource (
          "/online/alextee/zrythm/mute.svg");
  gtk_button_set_image (self->mute, image);
  image = gtk_image_new_from_resource (
          "/online/alextee/zrythm/record.svg");
  gtk_button_set_image (GTK_BUTTON (self->record), image);
  image = gtk_image_new_from_resource (
          "/online/alextee/zrythm/slot-on.svg");
  gtk_button_set_image (GTK_BUTTON (self->slot1b), image);
  image = gtk_image_new_from_resource (
          "/online/alextee/zrythm/slot-off.svg");
  gtk_button_set_image (GTK_BUTTON (self->slot2b), image);
  image = gtk_image_new_from_resource (
          "/online/alextee/zrythm/phase-invert.svg");
  gtk_button_set_image (self->phase_invert, image);
  switch (channel->type)
    {
    case CT_MIDI:
      gtk_image_set_from_resource (self->icon,
                  "/online/alextee/zrythm/instrument.svg");
      gtk_label_set_text (self->output,
                          "Master");
      break;
    case CT_MASTER:
      gtk_image_set_from_resource (self->icon,
                  "/online/alextee/zrythm/audio.svg");
      gtk_label_set_text (self->output,
                          "Stereo out");
      break;
    case CT_AUDIO:
      gtk_image_set_from_resource (self->icon,
                  "/online/alextee/zrythm/audio.svg");
      gtk_label_set_text (self->output,
                          "Master");
      break;
    }
    gtk_label_set_text (self->name,
                        channel->name);

  return self;
}

