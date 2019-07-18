/*
 * Copyright (C) 2018-2019 Alexandros Theodotou <alex at zrythm dot org>
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

#include "audio/bus_track.h"
#include "audio/channel.h"
#include "audio/instrument_track.h"
#include "audio/midi_note.h"
#include "audio/track.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/midi_arranger.h"
#include "gui/widgets/automation_region.h"
#include "gui/widgets/region.h"
#include "gui/widgets/ruler.h"
#include "gui/widgets/timeline_arranger.h"
#include "utils/cairo.h"

G_DEFINE_TYPE (AutomationRegionWidget,
               automation_region_widget,
               REGION_WIDGET_TYPE)


#define Y_PADDING 6.f
#define Y_HALF_PADDING 3.f

static gboolean
automation_region_draw_cb (
  GtkWidget * widget,
  cairo_t *cr,
  AutomationRegionWidget * self)
{

  region_widget_draw_name (
    Z_REGION_WIDGET (self), cr);

  return FALSE;
}

AutomationRegionWidget *
automation_region_widget_new (
  Region * region)
{
  AutomationRegionWidget * self =
    g_object_new (
      AUTOMATION_REGION_WIDGET_TYPE,
      NULL);

  region_widget_setup (
    Z_REGION_WIDGET (self), region);
  REGION_WIDGET_GET_PRIVATE (self);

  /* connect signals */
  g_signal_connect (
    G_OBJECT (rw_prv->drawing_area), "draw",
    G_CALLBACK (automation_region_draw_cb), self);

  return self;
}

static void
automation_region_widget_class_init (
  AutomationRegionWidgetClass * klass)
{
}

static void
automation_region_widget_init (
  AutomationRegionWidget * self)
{
}
