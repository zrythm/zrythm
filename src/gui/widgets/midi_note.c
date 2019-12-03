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

/**
 * \file
 *
 */

#include "audio/audio_bus_track.h"
#include "audio/channel.h"
#include "audio/chord_track.h"
#include "audio/instrument_track.h"
#include "audio/region.h"
#include "audio/track.h"
#include "gui/backend/midi_arranger_selections.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/bot_bar.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/clip_editor.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/midi_arranger.h"
#include "gui/widgets/midi_note.h"
#include "gui/widgets/ruler.h"
#include "project.h"
#include "settings/settings.h"
#include "utils/cairo.h"
#include "utils/ui.h"

static void
recreate_pango_layouts (
  MidiNote * self,
  int        width)
{
  ArrangerObject * obj = (ArrangerObject *) self;

  if (!PANGO_IS_LAYOUT (self->layout))
    {
      self->layout =
        z_cairo_create_default_pango_layout (
          GTK_WIDGET (
            arranger_object_get_arranger (obj)));
    }
  pango_layout_set_width (
    self->layout,
    pango_units_from_double (
      width - 2));
}

static void
draw_midi_note_bg (
  MidiNote *     self,
  cairo_t *      cr,
  GdkRectangle * rect,
  GdkRectangle * draw_rect)
{
  ArrangerObject * obj = (ArrangerObject *) self;

  /* if there are still midi note parts
   * outside the
   * rect, add some padding so that the
   * midi note
   * doesn't curve when it's not its edge */
  int draw_x = draw_rect->x - rect->x;
  int draw_x_has_padding = 0;
  if (draw_rect->x > obj->full_rect.x)
    {
      draw_x -=
        MIN (draw_rect->x - obj->full_rect.x, 4);
      draw_x_has_padding = 1;
    }
  int draw_width = draw_rect->width;
  if (draw_rect->x + draw_rect->width <
      obj->full_rect.x + obj->full_rect.width)
    {
      draw_width +=
        MAX (
          (draw_rect->x + draw_rect->width) -
            (obj->full_rect.x + obj->full_rect.width), 4);
    }
  if (draw_x_has_padding)
    {
      draw_width += 4;
    }

  z_cairo_rounded_rectangle (
    cr, draw_x, obj->full_rect.y - rect->y,
    draw_width, obj->full_rect.height, 1.0, 4.0);
}

/**
 * @param cr Arranger's cairo context.
 * @param rect Arranger's rectangle.
 */
void
midi_note_draw (
  MidiNote *     self,
  cairo_t *      cr,
  GdkRectangle * rect)
{
  ArrangerObject * obj = (ArrangerObject *) self;
  ArrangerWidget * arranger =
    arranger_object_get_arranger (obj);
  Region * region = self->region;
  Position global_start_pos;
  midi_note_get_global_start_pos (
    self, &global_start_pos);
  ChordObject * co =
    chord_track_get_chord_at_pos (
      P_CHORD_TRACK, &global_start_pos);
  ScaleObject * so =
    chord_track_get_scale_at_pos (
      P_CHORD_TRACK, &global_start_pos);
  int in_scale =
    so && musical_scale_is_key_in_scale (
      so->scale, self->val % 12);
  int in_chord =
    co && chord_descriptor_is_key_in_chord (
      chord_object_get_chord_descriptor (co),
      self->val % 12);

  GdkRectangle draw_rect;
  arranger_object_get_draw_rectangle (
    obj, rect, &obj->full_rect, &draw_rect);

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
      color = region->lane->track->color;
    }

  /* draw notes of main region */
  if (region == CLIP_EDITOR->region)
    {
      /* get color */
      ui_get_arranger_object_color (
        &color,
        arranger->hovered_object == obj,
        midi_note_is_selected (self),
        0);
      gdk_cairo_set_source_rgba (
        cr, &color);

      if (PIANO_ROLL->drum_mode)
        {
          z_cairo_diamond (
            cr, draw_rect.x - rect->x,
            draw_rect.y - rect->y, draw_rect.width,
            draw_rect.height);
        }
      else
        {
          draw_midi_note_bg (
            self, cr, rect, &draw_rect);
        }
      cairo_fill (cr);
    }
  /* draw other notes */
  else
    {
      /* get color */
      ui_get_arranger_object_color (
        &color,
        arranger->hovered_object == obj,
        midi_note_is_selected (self),
        /* FIXME */
        0);
      color.alpha = 0.5;
      gdk_cairo_set_source_rgba (
        cr, &color);

      if (PIANO_ROLL->drum_mode)
        {
          z_cairo_diamond (
            cr, draw_rect.x - rect->x,
            draw_rect.y - rect->y,
            draw_rect.width, draw_rect.height);
        }
      else
        {
          draw_midi_note_bg (
            self, cr, rect, &draw_rect);
        }
      cairo_fill (cr);
    }

  char str[30];
  midi_note_get_val_as_string (self, str, 1);

  GdkRGBA c2;
  ui_get_contrast_color (
    &color, &c2);
  gdk_cairo_set_source_rgba (
    cr, &c2);
  if (DEBUGGING || !PIANO_ROLL->drum_mode)
    {
      recreate_pango_layouts (
        self,
        MIN (obj->full_rect.width, 400));
      cairo_move_to (
        cr,
        REGION_NAME_BOX_PADDING +
          (obj->full_rect.x - rect->x),
        REGION_NAME_BOX_PADDING +
          (obj->full_rect.y - rect->y));
      PangoLayout * layout = self->layout;
      z_cairo_draw_text (
        cr,
        GTK_WIDGET (
          arranger_object_get_arranger (obj)),
        layout, str);
    }
}

#if 0
MidiNoteWidget *
midi_note_widget_new (MidiNote * midi_note)
{
  MidiNoteWidget * self =
    g_object_new (MIDI_NOTE_WIDGET_TYPE,
                  "visible", 1,
                  NULL);

  arranger_object_widget_setup (
    Z_ARRANGER_OBJECT_WIDGET (self),
    (ArrangerObject *) midi_note);

  self->midi_note = midi_note;

  /* set tooltip text */
  char * tooltip =
    g_strdup_printf (
      "[%s%d] %d",
      chord_descriptor_note_to_string (
        self->midi_note->val % 12),
      self->midi_note->val / 12 - 1,
      self->midi_note->vel->vel);
  gtk_widget_set_tooltip_text (
    GTK_WIDGET (self), tooltip);
  g_free (tooltip);

  return self;
}
#endif
