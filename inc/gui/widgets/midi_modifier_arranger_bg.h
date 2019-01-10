/*
 * inc/gui/widgets/midi_modifier_arranger.h - MIDI modifier
 *   arranger background
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __GUI_WIDGETS_MIDI_MODIFIER_ARRANGER_BG_H__
#define __GUI_WIDGETS_MIDI_MODIFIER_ARRANGER_BG_H__

#include "gui/widgets/arranger_bg.h"

#include <gtk/gtk.h>

#define MIDI_MODIFIER_ARRANGER_BG_WIDGET_TYPE \
  (midi_modifier_arranger_bg_widget_get_type ())
G_DECLARE_FINAL_TYPE (MidiModifierArrangerBgWidget,
                      midi_modifier_arranger_bg_widget,
                      Z,
                      MIDI_MODIFIER_ARRANGER_BG_WIDGET,
                      ArrangerBgWidget)

typedef struct _MidiModifierArrangerBgWidget
{
  ArrangerBgWidget         parent_instance;
} MidiModifierArrangerBgWidget;

/**
 * Creates a midi_modifier_arranger widget using the given midi_modifier_arranger data.
 */
MidiModifierArrangerBgWidget *
midi_modifier_arranger_bg_widget_new (
  RulerWidget *    ruler,
  ArrangerWidget * arranger);

#endif
