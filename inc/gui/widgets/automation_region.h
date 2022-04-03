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
 * Widget for MIDI regions, inheriting from
 * RegionWidget.
 */
#if 0

#  ifndef __GUI_WIDGETS_AUTOMATION_REGION_H__
#    define __GUI_WIDGETS_AUTOMATION_REGION_H__

#    include "audio/region.h"
#    include "gui/widgets/region.h"
#    include "utils/ui.h"

#    include <gtk/gtk.h>

#    define AUTOMATION_REGION_WIDGET_TYPE \
      (automation_region_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  AutomationRegionWidget,
  automation_region_widget,
  Z, AUTOMATION_REGION_WIDGET,
  RegionWidget);

typedef struct ZRegion AutomationRegion;

/**
 * A widget that represents a AutomationZRegion in the
 * TimelineArrangerWidget.
 *
 * It displays the MidiNotes of the ZRegion in
 * miniature size.
 */
typedef struct _AutomationRegionWidget
{
  RegionWidget             parent_instance;
} AutomationRegionWidget;

/**
 * Creates a region.
 */
AutomationRegionWidget *
automation_region_widget_new (
  ZRegion * automation_region);

#  endif
#endif
