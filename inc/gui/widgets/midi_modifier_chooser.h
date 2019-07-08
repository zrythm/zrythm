/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * \file
 *
 * This widget is for choosing a MIDI modifier
 * (velocity, pitch bend, etc.) to show in the
 * MIDI modifier arranger.
 */

#ifndef __GUI_WIDGETS_MIDI_MODIFIER_CHOOSER_H__
#define __GUI_WIDGETS_MIDI_MODIFIER_CHOOSER_H__

#include "audio/channel.h"
#include "gui/widgets/track.h"

#include <gtk/gtk.h>

#define MIDI_MODIFIER_CHOOSER_WIDGET_TYPE \
  (midi_modifier_chooser_widget_get_type ())
G_DECLARE_FINAL_TYPE (MidiModifierChooserWidget,
                      midi_modifier_chooser_widget,
                      Z,
                      MIDI_MODIFIER_CHOOSER_WIDGET,
                      GtkComboBox)

typedef enum MidiModifier MidiModifier;

typedef struct _MidiModifierChooserWidget
{
  GtkComboBox                   parent_instance;

  /**
   * Pointer to backend value.
   */
  MidiModifier *                midi_modifier;
} MidiModifierChooserWidget;

/**
 * Updates changes in the backend to the ui
 */
void
midi_modifier_chooser_widget_setup (
  MidiModifierChooserWidget * self,
  MidiModifier *              midi_modifier);

#endif
