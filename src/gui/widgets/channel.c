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

#include "zrythm.h"
#include "audio/mixer.h"
#include "plugins/lv2_plugin.h"
#include "gui/widgets/channel.h"
#include "gui/widgets/color_area.h"
#include "gui/widgets/meter.h"
#include "gui/widgets/channel_slot.h"
#include "gui/widgets/fader.h"
#include "gui/widgets/knob.h"
#include "gui/widgets/pan.h"
#include "actions/edit_channel_action.h"
#include "actions/undo_manager.h"
#include "actions/undoable_action.h"
#include "utils/gtk.h"
#include "utils/resources.h"

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
  g_assert (widget);

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
  gtk_widget_queue_draw (GTK_WIDGET (widget->meter_l));
  gtk_widget_queue_draw (GTK_WIDGET (widget->meter_r));
}

static void
phase_invert_button_clicked (ChannelWidget * self,
                             GtkButton     * button)
{

}


static void
on_record_toggled (GtkToggleButton * btn,
                   ChannelWidget *   self)
{

}

static void
on_solo_toggled (GtkToggleButton * btn,
                 ChannelWidget *   self)
{
  if (!self->undo_redo_action)
    {
      UndoableAction * action =
        edit_channel_action_new_solo (self->channel);
      undo_manager_perform (UNDO_MANAGER,
                            action);
    }
  else
    {
      self->undo_redo_action = 0;
    }
}

static void
on_mute_toggled (GtkToggleButton * btn,
                 ChannelWidget * self)
{

}

static void
on_listen_toggled (GtkToggleButton * btn,
                   ChannelWidget *   self)
{

}

static void
on_e_activate (GtkButton *     btn,
               ChannelWidget * self)
{

}

static void
refresh_color (ChannelWidget * self)
{
  color_area_widget_set_color (self->color,
                               &self->channel->color);
}

static void
setup_phase_panel (ChannelWidget * self)
{
  self->phase_knob = knob_widget_new (channel_get_phase,
                                      channel_set_phase,
                                      self->channel,
                                      0,
                                      180,
                                      20,
                                      0.0f);
  gtk_box_pack_end (self->phase_controls,
                       GTK_WIDGET (self->phase_knob),
                       0, 1, 0);
  gtk_label_set_text (self->phase_reading,
                      g_strdup_printf ("%.1f", self->channel->phase));
}

static void
setup_meter (ChannelWidget * self)
{
  meter_widget_setup (self->meter_l,
               channel_get_current_l_db,
               self->channel,
               METER_TYPE_DB,
               12);
  meter_widget_setup (self->meter_r,
               channel_get_current_r_db,
               self->channel,
               METER_TYPE_DB,
               12);
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
  /*Channel * channel = self->channel;*/
  for (int i = 0; i < STRIP_SIZE; i++)
    {
      self->slot_boxes[i] =
        GTK_BOX (gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0));

      self->slots[i] = channel_slot_widget_new (i, self);
      /* FIXME set to channel widget width */
      /*gtk_widget_set_size_request (GTK_WIDGET (self->slot_boxes[i]),*/
                                   /*20, 20);*/
      gtk_box_pack_start (self->slot_boxes[i],
                          GTK_WIDGET (self->slots[i]),
                          1, 1, 0);
      gtk_box_pack_start (self->slots_box,
                          GTK_WIDGET (self->slot_boxes[i]),
                          0, 1, 0);
      gtk_widget_show_all (GTK_WIDGET (self->slot_boxes[i]));
    }
}

static void
setup_channel_icon (ChannelWidget * self)
{
  switch (self->channel->type)
    {
    case CT_MIDI:
      resources_set_image_icon (self->icon,
                                ICON_TYPE_ZRYTHM,
                                "instrument.svg");
      break;
    case CT_AUDIO:
      resources_set_image_icon (self->icon,
                                ICON_TYPE_ZRYTHM,
                                "audio.svg");
    case CT_BUS:
    case CT_MASTER:
      resources_set_image_icon (self->icon,
                                ICON_TYPE_ZRYTHM,
                                "bus.svg");
      break;
    }
}

static void
refresh_output (ChannelWidget * self)
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
    case CT_BUS:
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
refresh_name (ChannelWidget * self)
{
  g_assert (self->channel->name);
  gtk_label_set_text (self->name,
                      self->channel->name);
}

static void
setup_pan (ChannelWidget * self)
{
  self->pan = pan_widget_new (channel_get_pan,
                              channel_set_pan,
                              self->channel,
                              12);
  gtk_box_pack_start (self->pan_box,
                       GTK_WIDGET (self->pan),
                       Z_GTK_NO_EXPAND,
                       Z_GTK_FILL,
                       0);
}

static void
refresh_buttons (ChannelWidget * self)
{
  gtk_toggle_button_set_active (
    self->solo,
    self->channel->solo);
  gtk_toggle_button_set_active (
    self->mute,
    self->channel->mute);
}

/**
 * Updates everything on the widget.
 *
 * It is reduntant but keeps code organized. Should fix if it causes lags.
 */
void
channel_widget_refresh (ChannelWidget * self)
{
  refresh_name (self);
  refresh_output (self);
  channel_widget_update_meter_reading (self);
  refresh_buttons (self);
  refresh_color (self);
}


ChannelWidget *
channel_widget_new (Channel * channel)
{
  ChannelWidget * self = g_object_new (CHANNEL_WIDGET_TYPE, NULL);
  self->channel = channel;

  setup_phase_panel (self);
  /*setup_pan (self);*/
  setup_slots (self);
  fader_widget_setup (self->fader,
                      channel_get_fader_amp,
                      channel_set_fader_amp,
                      self->channel,
                      40);
  setup_meter (self);
  setup_pan (self);
  setup_channel_icon (self);
  channel_widget_refresh (self);

  return self;
}

static void
channel_widget_class_init (ChannelWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (klass,
                                "channel.ui");
  gtk_widget_class_set_css_name (klass,
                                 "channel");

  gtk_widget_class_bind_template_child (
    klass,
    ChannelWidget,
    color);
  gtk_widget_class_bind_template_child (
    klass,
    ChannelWidget,
    output);
  gtk_widget_class_bind_template_child (
    klass,
    ChannelWidget,
    name);
  gtk_widget_class_bind_template_child (
    klass,
    ChannelWidget,
    phase_invert);
  gtk_widget_class_bind_template_child (
    klass,
    ChannelWidget,
    phase_reading);
  gtk_widget_class_bind_template_child (
    klass,
    ChannelWidget,
    phase_controls);
  gtk_widget_class_bind_template_child (
    klass,
    ChannelWidget,
    slots_box);
  gtk_widget_class_bind_template_child (
    klass,
    ChannelWidget,
    e);
  gtk_widget_class_bind_template_child (
    klass,
    ChannelWidget,
    solo);
  gtk_widget_class_bind_template_child (
    klass,
    ChannelWidget,
    listen);
  gtk_widget_class_bind_template_child (
    klass,
    ChannelWidget,
    mute);
  gtk_widget_class_bind_template_child (
    klass,
    ChannelWidget,
    record);
  gtk_widget_class_bind_template_child (
    klass,
    ChannelWidget,
    fader);
  gtk_widget_class_bind_template_child (
    klass,
    ChannelWidget,
    meter_area);
  gtk_widget_class_bind_template_child (
    klass,
    ChannelWidget,
    meter_l);
  gtk_widget_class_bind_template_child (
    klass,
    ChannelWidget,
    meter_r);
  gtk_widget_class_bind_template_child (
    klass,
    ChannelWidget,
    meter_reading);
  gtk_widget_class_bind_template_child (
    klass,
    ChannelWidget,
    icon);
  gtk_widget_class_bind_template_child (
    klass,
    ChannelWidget,
    pan_box);
  gtk_widget_class_bind_template_child (
    klass,
    ChannelWidget,
    output_img);
  gtk_widget_class_bind_template_callback (
    klass,
    phase_invert_button_clicked);
  gtk_widget_class_bind_template_callback (
    klass,
    on_record_toggled);
  gtk_widget_class_bind_template_callback (
    klass,
    on_solo_toggled);
  gtk_widget_class_bind_template_callback (
    klass,
    on_mute_toggled);
  gtk_widget_class_bind_template_callback (
    klass,
    on_listen_toggled);
  gtk_widget_class_bind_template_callback (
    klass,
    on_e_activate);
}

static void
channel_widget_init (ChannelWidget * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}
