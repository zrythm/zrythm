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
#include "audio/mixer.h"
#include "plugins/lv2_plugin.h"
#include "gui/widgets/channel.h"
#include "gui/widgets/color_area.h"
#include "gui/widgets/meter.h"
#include "gui/widgets/channel_slot.h"
#include "gui/widgets/fader.h"
#include "gui/widgets/knob.h"
#include "gui/widget_manager.h"

#include <gtk/gtk.h>

#include <time.h>
#include <sys/time.h>

G_DEFINE_TYPE (ChannelWidget, channel_widget, GTK_TYPE_GRID)

/**
 * Tick function.
 *
 * usually, the way to do that is via g_idle_add() or g_main_context_invoke()
 * the other option is to use polling and have the GTK thread check if the
 * value changed every monitor refresh
 * that would be done via gtk_widget_set_tick_functions()
 * gtk_widget_set_tick_function()
 */
void
channel_widget_update_meter_reading (ChannelWidget * widget)
{
  double val = (channel_get_current_l_db (widget->channel) +
                channel_get_current_r_db (widget->channel)) / 2;
  char * string;
  if (val < -100.)
    gtk_label_set_text (widget->meter_reading, "-∞");
  else
    {
      string = g_strdup_printf ("%.1f", val);
      gtk_label_set_text (widget->meter_reading, string);
      g_free (string);
    }
  gtk_widget_queue_draw (GTK_WIDGET (widget->meters[0]));
  gtk_widget_queue_draw (GTK_WIDGET (widget->meters[1]));
}

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
                                                ChannelWidget, slots_box);
  /*gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),*/
                                                /*ChannelWidget, add_slot);*/
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
                                                ChannelWidget, meters_box);
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
  if (self->color)
    {
      /*gtk_container_remove (GTK_CONTAINER (self),*/
                            /*GTK_WIDGET (self->color));*/
    }
  self->color = color_area_widget_new (&self->channel->color, -1, 10);
  gtk_grid_attach (GTK_GRID (self),
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
  self->meters[0] = meter_widget_new (
                          channel_get_current_l_db,
                          self->channel,
                          METER_TYPE_DB,
                          12);
  self->meters[1] = meter_widget_new (
                          channel_get_current_r_db,
                          self->channel,
                          METER_TYPE_DB,
                          12);
  gtk_box_pack_start (self->meters_box,
                       GTK_WIDGET (self->meters[0]),
                       1, 1, 0);
  gtk_box_pack_start (self->meters_box,
                       GTK_WIDGET (self->meters[1]),
                       1, 1, 0);
}

/**
 * Updates the slots.
 */
void
channel_update_slots (ChannelWidget * self)
{
  gtk_widget_queue_draw (GTK_WIDGET (self));
}


/**
 * Sets up the slots.
 *
 * First removes the add button, then creates each slot.
 */
static void
setup_slots (ChannelWidget * self)
{
  /*gtk_container_remove (GTK_CONTAINER (self->slots_box),*/
                        /*GTK_WIDGET (self->add_slot));*/
  Channel * channel = self->channel;
  for (int i = 0; i < STRIP_SIZE; i++)
    {
      self->slot_boxes[i] =
        GTK_BOX (gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0));

      self->slots[i] = channel_slot_widget_new (i, channel);
      /* FIXME set to channel widget width */
      /*gtk_widget_set_size_request (GTK_WIDGET (self->slot_boxes[i]),*/
                                   /*20, 20);*/
      gtk_box_pack_start (self->slot_boxes[i],
                          GTK_WIDGET (self->slots[i]),
                          1, 1, 0);
      gtk_box_pack_start (self->slots_box,
                          GTK_WIDGET (self->slot_boxes[i]),
                          0, 1, 0);
    }
}

static void
setup_channel_icon (ChannelWidget * self)
{
  switch (self->channel->type)
    {
    case CT_MIDI:
      gtk_image_set_from_resource (self->icon,
                  "/online/alextee/zrythm/instrument.svg");
      break;
    case CT_MASTER:
      gtk_image_set_from_resource (self->icon,
                  "/online/alextee/zrythm/audio.svg");
      break;
    case CT_AUDIO:
      gtk_image_set_from_resource (self->icon,
                  "/online/alextee/zrythm/audio.svg");
      break;
    }
}

static void
setup_output (ChannelWidget * self)
{
  switch (self->channel->type)
    {
    case CT_MIDI:
      gtk_label_set_text (self->output,
                          "Master");
      break;
    case CT_MASTER:
      gtk_label_set_text (self->output,
                          "Stereo out");
      break;
    case CT_AUDIO:
      gtk_label_set_text (self->output,
                          "Master");
      break;
    }
  if (self->channel->output)
    {
      gtk_label_set_text (self->output,
                          self->channel->output->name);
    }
}

static void
setup_name (ChannelWidget * self)
{
  if (self->channel->name )
    {
      gtk_label_set_text (self->name,
                          self->channel->name);
    }
}

/**
 * Updates everything on the widget.
 *
 * It is reduntant but keeps code organized. Should fix if it causes lags.
 */
void
channel_widget_update_all (ChannelWidget * self)
{
  setup_channel_icon (self);
  setup_name (self);
  setup_output (self);
  setup_color (self);
}


ChannelWidget *
channel_widget_new (Channel * channel)
{
  ChannelWidget * self = g_object_new (CHANNEL_WIDGET_TYPE, NULL);
  self->channel = channel;
  channel->widget = self;

  setup_phase_panel (self);
  /*setup_pan (self);*/
  setup_slots (self);
  setup_fader (self);
  setup_meter (self);
  channel_widget_update_all (self);

  GtkWidget * image = gtk_image_new_from_resource (
          "/online/alextee/zrythm/plus.svg");
  /*gtk_button_set_image (self->add_slot, image);*/
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
          "/online/alextee/zrythm/phase-invert.svg");
  gtk_button_set_image (self->phase_invert, image);

  return self;
}

