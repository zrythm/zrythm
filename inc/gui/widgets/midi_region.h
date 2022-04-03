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

#if 0

/**
 * \file
 *
 * Widget for MIDI regions, inheriting from
 * RegionWidget.
 */

#  ifndef __GUI_WIDGETS_MIDI_REGION_H__
#    define __GUI_WIDGETS_MIDI_REGION_H__

#    include "audio/region.h"
#    include "gui/widgets/region.h"
#    include "utils/ui.h"

#    include <gtk/gtk.h>

#    define MIDI_REGION_WIDGET_TYPE \
      (midi_region_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  MidiRegionWidget,
  midi_region_widget,
  Z, MIDI_REGION_WIDGET,
  RegionWidget);

typedef struct ZRegion MidiRegion;

/**
 * A widget that represents a ZRegion in the
 * TimelineArrangerWidget.
 *
 * It displays the MidiNotes of the ZRegion in
 * miniature size.
 */
typedef struct _MidiRegionWidget
{
  RegionWidget             parent_instance;
} MidiRegionWidget;

/**
 * Creates a region.
 */
MidiRegionWidget *
midi_region_widget_new (
  ZRegion * midi_region);

#  endif
#endif
