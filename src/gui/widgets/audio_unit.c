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

#include "gui/widgets/audio_unit.h"

G_DEFINE_TYPE (AudioUnitWidget, audio_unit_widget, GTK_TYPE_GRID)

AudioUnitWidget *
audio_unit_widget_new ()
{
  AudioUnitWidget * self = g_object_new (AUDIO_UNIT_WIDGET_TYPE, NULL);

  return self;
}


static void
audio_unit_widget_class_init (AudioUnitWidgetClass * klass)
{
  gtk_widget_class_set_template_from_resource (
                        GTK_WIDGET_CLASS (klass),
                        "/online/alextee/zrythm/ui/audio_unit.ui");

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


