/*
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

/** \file */

#ifndef __GUI_WIDGETS_MIDI_REGION_H__
#define __GUI_WIDGETS_MIDI_REGION_H__

#include "audio/region.h"
#include "gui/widgets/region.h"

#include <gtk/gtk.h>

#define MIDI_REGION_WIDGET_TYPE (midi_region_widget_get_type ())
G_DECLARE_FINAL_TYPE (MidiRegionWidget,
                      midi_region_widget,
                      MIDI_REGION,
                      WIDGET,
                      RegionWidget);

typedef struct MidiRegion MidiRegion;

typedef enum MidiRegionCursor
{
  MIDI_REGION_CURSOR_DEFAULT,
  MIDI_REGION_CURSOR_RESIZE_L,
  MIDI_REGION_CURSOR_REPEAT_L,
  MIDI_REGION_CURSOR_RESIZE_R,
  MIDI_REGION_CURSOR_REPEAT_R
} MidiRegionCursor;

typedef struct _MidiRegionWidget
{
  RegionWidget             parent_instance;
  MidiRegionCursor         cursor_state;
} MidiRegionWidget;

/**
 * Creates a region.
 */
MidiRegionWidget *
midi_region_widget_new (MidiRegion * midi_region);

GType midi_region_widget_get_type(void);

#endif
