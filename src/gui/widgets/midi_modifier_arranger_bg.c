/*
 * gui/widgets/midi_modifier_arranger_bg.c - MidiModifierArranger background
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
#include "project.h"
#include "audio/automation_track.h"
#include "audio/automation_tracklist.h"
#include "audio/channel.h"
#include "audio/instrument_track.h"
#include "audio/mixer.h"
#include "audio/track.h"
#include "audio/tracklist.h"
#include "audio/transport.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/automation_point.h"
#include "gui/widgets/automation_track.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/ruler.h"
#include "gui/widgets/midi_modifier_arranger.h"
#include "gui/widgets/midi_modifier_arranger_bg.h"
#include "gui/widgets/timeline_ruler.h"
#include "gui/widgets/track.h"
#include "gui/widgets/tracklist.h"
#include "settings/settings.h"

#include <gtk/gtk.h>

G_DEFINE_TYPE (MidiModifierArrangerBgWidget,
               midi_modifier_arranger_bg_widget,
               ARRANGER_BG_WIDGET_TYPE)

static gboolean
draw_cb (GtkWidget *widget, cairo_t *cr, gpointer data)
{
  GdkRectangle rect;
  gdk_cairo_get_clip_rectangle (cr,
                                &rect);
  return 0;
}

MidiModifierArrangerBgWidget *
midi_modifier_arranger_bg_widget_new (
  RulerWidget *    ruler,
  ArrangerWidget * arranger)
{
  MidiModifierArrangerBgWidget * self =
    g_object_new (MIDI_MODIFIER_ARRANGER_BG_WIDGET_TYPE,
                  NULL);

  ARRANGER_BG_WIDGET_GET_PRIVATE (self);
  ab_prv->ruler = ruler;
  ab_prv->arranger = arranger;

  return self;
}

static void
midi_modifier_arranger_bg_widget_class_init (
  MidiModifierArrangerBgWidgetClass * _klass)
{
}

static void
midi_modifier_arranger_bg_widget_init (
  MidiModifierArrangerBgWidget *self )
{
  g_signal_connect (G_OBJECT (self), "draw",
                    G_CALLBACK (draw_cb), NULL);

}
