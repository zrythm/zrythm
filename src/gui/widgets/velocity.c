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

/** \file */

#include "audio/channel.h"
#include "audio/channel_track.h"
#include "audio/chord_track.h"
#include "audio/midi_note.h"
#include "audio/region.h"
#include "gui/backend/midi_arranger_selections.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/bot_bar.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/clip_editor.h"
#include "gui/widgets/midi_modifier_arranger.h"
#include "gui/widgets/velocity.h"
#include "project.h"
#include "utils/cairo.h"
#include "utils/flags.h"
#include "utils/ui.h"
#include "zrythm_app.h"

/**
 * Draws the Velocity in the given cairo context in
 * relative coordinates.
 *
 * @param cr The arranger cairo context.
 * @param rect Arranger rectangle.
 */
void
velocity_draw (
  Velocity *     self,
  cairo_t *      cr,
  GdkRectangle * rect)
{
  ArrangerObject * obj = (ArrangerObject *) self;
  ArrangerWidget * arranger =
    arranger_object_get_arranger (obj);
  MidiNote * mn = velocity_get_midi_note (self);
  ArrangerObject *  mn_obj =
    (ArrangerObject *) mn;
  ZRegion * region =
    arranger_object_get_region (obj);
  Position global_start_pos;
  midi_note_get_global_start_pos (
    mn, &global_start_pos);
  ChordObject * co =
    chord_track_get_chord_at_pos (
      P_CHORD_TRACK, &global_start_pos);
  ScaleObject * so =
    chord_track_get_scale_at_pos (
      P_CHORD_TRACK, &global_start_pos);
  int in_scale =
    so && musical_scale_is_key_in_scale (
      so->scale, mn->val % 12);
  int in_chord =
    co && chord_descriptor_is_key_in_chord (
      chord_object_get_chord_descriptor (co),
      mn->val % 12);

  GdkRGBA color;
  if (PIANO_ROLL->highlighting ==
        PR_HIGHLIGHT_BOTH &&
      in_scale && in_chord)
    {
      color = UI_COLORS->highlight_both;
    }
  else if ((PIANO_ROLL->highlighting ==
        PR_HIGHLIGHT_SCALE ||
      PIANO_ROLL->highlighting ==
        PR_HIGHLIGHT_BOTH) && in_scale)
    {
      color = UI_COLORS->highlight_in_scale;
    }
  else if ((PIANO_ROLL->highlighting ==
        PR_HIGHLIGHT_CHORD ||
      PIANO_ROLL->highlighting ==
        PR_HIGHLIGHT_BOTH) && in_chord)
    {
      color = UI_COLORS->highlight_in_chord;
    }
  else
    {
      Track * track =
        arranger_object_get_track (obj);
      color = track->color;
    }

  /* draw velocities of main region */
  if (region == clip_editor_get_region (CLIP_EDITOR))
    {
      ui_get_arranger_object_color (
        &color,
        arranger->hovered_object == mn_obj,
        velocity_is_selected (self),
        /* FIXME */
        false, arranger_object_get_muted (mn_obj));
      gdk_cairo_set_source_rgba (cr, &color);
      cairo_rectangle (
        cr,
        obj->full_rect.x - rect->x,
        obj->full_rect.y - rect->y,
        obj->full_rect.width,
        obj->full_rect.height);
      cairo_fill(cr);
    }
  /* draw other notes */
  else
    {
      cairo_set_source_rgba (
        cr, color.red, color.green,
        color.blue, 0.5);
      cairo_rectangle (
        cr,
        obj->full_rect.x - rect->x,
        obj->full_rect.y - rect->y,
        obj->full_rect.width,
        obj->full_rect.height);
      cairo_fill(cr);
    }
}


#if 0
/**
 * Creates a velocity.
 */
VelocityWidget *
velocity_widget_new (Velocity * velocity)
{
  VelocityWidget * self =
    g_object_new (VELOCITY_WIDGET_TYPE,
                  NULL);

  arranger_object_widget_setup (
    Z_ARRANGER_OBJECT_WIDGET (self),
    (ArrangerObject *) velocity);

  self->velocity = velocity;

  return self;
}
#endif
