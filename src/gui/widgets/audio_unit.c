/*
 * gui/widgets/audio_unit.c - An audio entity with input and output ports, used in
 *                            audio_unit widget for connecting ports.
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

#include "audio/channel.h"
#include "gui/widgets/audio_unit.h"
#include "gui/widgets/audio_unit_label.h"
#include "utils/gtk.h"

#include <gtk/gtk.h>

G_DEFINE_TYPE (AudioUnitWidget, audio_unit_widget, GTK_TYPE_GRID)

AudioUnitWidget *
audio_unit_widget_new (int is_right)
{
  AudioUnitWidget * self = g_object_new (AUDIO_UNIT_WIDGET_TYPE, NULL);

  self->is_right = is_right;

  return self;
}


static void
audio_unit_widget_class_init (AudioUnitWidgetClass * klass)
{
  gtk_widget_class_set_template_from_resource (
                        GTK_WIDGET_CLASS (klass),
                        "/org/zrythm/ui/audio_unit.ui");

  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                        AudioUnitWidget,
                                        label);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                        AudioUnitWidget,
                                        l_box);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                        AudioUnitWidget,
                                        r_box);
}

static void
audio_unit_widget_init (AudioUnitWidget * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}

void
audio_unit_widget_clear_ports (AudioUnitWidget * self)
{
  z_gtk_container_remove_all_children (GTK_CONTAINER (self->l_box));
  z_gtk_container_remove_all_children (GTK_CONTAINER (self->r_box));
  self->num_l_labels = 0;
  self->num_r_labels = 0;
}

void
audio_unit_widget_add_port (AudioUnitWidget * self,
                            AULType           type,
                            Port *            port)
{
  AudioUnitLabelWidget * aul =
    audio_unit_label_widget_new (type,
                                 port,
                                 self);
  GtkBox * box = (type == AUL_TYPE_LEFT) ? self->l_box : self->r_box;
  gtk_box_pack_start (box,
                      GTK_WIDGET (aul),
                      Z_GTK_NO_EXPAND,
                      Z_GTK_FILL,
                      0);
  gtk_widget_show (GTK_WIDGET (aul));

  if (type == AUL_TYPE_LEFT)
    {
      self->l_labels[self->num_l_labels++] = aul;
    }
  else
    {
      self->r_labels[self->num_r_labels++] = aul;
    }
}

void
audio_unit_widget_set_from_channel (AudioUnitWidget * self,
                                    Channel * chan)
{
  /* remove existing ports */
  audio_unit_widget_clear_ports (self);

  /* set label */
  gtk_label_set_text (self->label, chan->name);

  /* add ins */
  audio_unit_widget_add_port (self, AUL_TYPE_LEFT, chan->stereo_in->l);
  audio_unit_widget_add_port (self, AUL_TYPE_LEFT, chan->stereo_in->r);
  audio_unit_widget_add_port (self, AUL_TYPE_LEFT, chan->midi_in);
  audio_unit_widget_add_port (self, AUL_TYPE_LEFT, chan->piano_roll);

  /* add outs */
  audio_unit_widget_add_port (self, AUL_TYPE_RIGHT, chan->stereo_out->l);
  audio_unit_widget_add_port (self, AUL_TYPE_RIGHT, chan->stereo_out->r);
}

void
audio_unit_widget_set_from_plugin (AudioUnitWidget * self,
                                   Plugin *          plugin)
{
  /* remove existing ports */
  audio_unit_widget_clear_ports (self);

  /* set label */
  gtk_label_set_text (self->label, plugin->descr->name);

  /* add ins */
  for (int i = 0; i < plugin->num_in_ports; i++)
    {
      Port * port = plugin->in_ports[i];
      audio_unit_widget_add_port (self, AUL_TYPE_LEFT, port);
    }

  /* add outs */
  for (int i = 0; i < plugin->num_out_ports; i++)
    {
      Port * port = plugin->out_ports[i];
      audio_unit_widget_add_port (self, AUL_TYPE_RIGHT, port);
    }
}

