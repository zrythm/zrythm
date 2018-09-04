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
#include "gui/widgets/channel_color.h"
#include "gui/widgets/channel_meter.h"
#include "gui/widgets/fader.h"
#include "gui/widgets/knob.h"
#include "gui/widget_manager.h"

#include <gtk/gtk.h>

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
static gboolean
tick_callback (GtkWidget * widget, GdkFrameClock * frame_clock,
               gpointer user_data)
{
  Channel * channel = (Channel *) user_data;
  gtk_label_set_text (GTK_LABEL (widget),
                      g_strdup_printf ("%.1f", channel->r_port_db));
  gtk_widget_queue_draw (GTK_WIDGET (channel->widget->cm));
  return G_SOURCE_CONTINUE;
}

/**
 * Adds a plugin to the given channel widget at the last slot
 */
static void
channel_widget_add_plugin (ChannelWidget * self,    ///< channel
                           Plugin        * plugin,   ///< plugin to add
                           int           slot)    ///< slot to add it to
{
  gtk_label_set_text (self->labels[slot], g_strdup (plugin->descr->name));

  /* set on/off icon */
  GtkWidget * image = gtk_image_new_from_resource (
          "/online/alextee/zrythm/slot-on.svg");
  gtk_button_set_image (GTK_BUTTON (self->toggles[slot]), image);
}

static void
on_drag_data_received (GtkWidget        *widget,
               GdkDragContext   *context,
               gint              x,
               gint              y,
               GtkSelectionData *data,
               guint             info,
               guint             time,
               gpointer          user_data)
{
  Channel * channel = CHANNEL_WIDGET (user_data)->channel;
  int index = channel_get_last_active_slot_index (channel);
  if (index == MAX_PLUGINS - 1)
    {
      /* TODO */
      g_error ("Channel is full");
      return;
    }

  Plugin_Descriptor * descr = *(gpointer *) gtk_selection_data_get_data (data);

  /* FIXME if lv2... */
  Plugin * plugin = plugin_new ();
  plugin->descr = descr;
  LV2_Plugin * lv2_plugin = lv2_create_from_uri (plugin, descr->uri);

  plugin_instantiate (plugin);

  /* TODO add to specific channel */
  channel_add_plugin (channel, index, plugin);
  channel_widget_add_plugin (CHANNEL_WIDGET (user_data), plugin, index);
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
  self->cm = channel_meter_widget_new (channel_get_current_l_db,
                                       channel_get_current_r_db,
                                  self->channel,
                                  12);
  gtk_box_pack_start (self->meter_area,
                       GTK_WIDGET (self->cm),
                       1, 1, 0);
}

static void
create_slots (ChannelWidget * self)
{
  gtk_container_remove (GTK_CONTAINER (self->slots_box),
                        GTK_WIDGET (self->add_slot));
  for (int i = 0; i < MAX_PLUGINS; i++)
    {
      self->slots[i] = GTK_BOX (gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0));
      self->toggles[i]= GTK_TOGGLE_BUTTON (gtk_toggle_button_new ());
      self->labels[i]= GTK_LABEL (gtk_label_new (""));

      /* set on/off icon */
      GtkWidget * image = gtk_image_new_from_resource (
              "/online/alextee/zrythm/slot-off.svg");
      gtk_button_set_image (GTK_BUTTON (self->toggles[i]), image);

      gtk_box_pack_start (self->slots[i],
                          GTK_WIDGET (self->toggles[i]),
                          0, 1, 0);
      gtk_box_pack_start (self->slots[i],
                          GTK_WIDGET (self->labels[i]),
                          1, 1, 0);
      gtk_box_pack_start (self->slots_box,
                          GTK_WIDGET (self->slots[i]),
                          0, 1, 0);
    }
  gtk_box_pack_start (self->slots_box,
                      GTK_WIDGET (self->add_slot),
                      0, 1, 0);
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
                      g_strdup (channel->name));

  /* add dummy box for dnd */
  self->dummy_slot_box = GTK_BOX (
                            gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0));
  gtk_box_pack_start (self->slots_box,
                      GTK_WIDGET (self->dummy_slot_box),
                      1, 1, 0);
  gtk_drag_dest_set (GTK_WIDGET (self->dummy_slot_box),
                            GTK_DEST_DEFAULT_ALL,
                            WIDGET_MANAGER->entries,
                            WIDGET_MANAGER->num_entries,
                            GDK_ACTION_COPY);

  g_signal_connect (GTK_WIDGET (self->dummy_slot_box),
                    "drag-data-received",
                    G_CALLBACK(on_drag_data_received), self);
  gtk_widget_add_tick_callback (GTK_WIDGET (self->meter_reading),
                                tick_callback,
                                channel,
                                NULL);

  return self;
}

