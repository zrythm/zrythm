/*
 * Copyright (C) 2019-2020 Alexandros Theodotou <alex at zrythm dot org>
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

#include <stdbool.h>
#include <stdlib.h>

#include "gui/widgets/automation_mode.h"
#include "utils/cairo.h"
#include "utils/ui.h"
#include "zrythm.h"
#include "zrythm_app.h"

void
automation_mode_widget_init (
  AutomationModeWidget * self)
{
  gdk_rgba_parse (
    &self->def_color, UI_COLOR_BUTTON_NORMAL);
  gdk_rgba_parse (
    &self->hovered_color, UI_COLOR_BUTTON_HOVER);
  self->toggled_colors[0] = UI_COLORS->solo_checked;
  self->held_colors[0] = UI_COLORS->solo_active;
  gdk_rgba_parse (
    &self->toggled_colors[1], UI_COLOR_RECORD_CHECKED);
  gdk_rgba_parse (
    &self->held_colors[1], UI_COLOR_RECORD_ACTIVE);
  gdk_rgba_parse (
    &self->toggled_colors[2], "#666666");
  gdk_rgba_parse (
    &self->held_colors[2], "#888888");
  self->aspect = 1.0;
  self->corner_radius = 8.0;

  /* set dimensions */
  PangoLayout * layout = self->layout;
  int total_width = 0;
  int x_px, y_px;
  char txt[400];
#define DO(caps) \
  if (AUTOMATION_MODE_##caps == \
        AUTOMATION_MODE_RECORD) \
    { \
      automation_record_mode_get_localized ( \
        self->owner->record_mode, txt); \
    } \
  else \
    { \
      automation_mode_get_localized ( \
        AUTOMATION_MODE_##caps, txt); \
    } \
  pango_layout_set_text (layout, txt, -1); \
  pango_layout_get_pixel_size ( \
    layout, &x_px, &y_px); \
  self->text_widths[AUTOMATION_MODE_##caps] = x_px; \
  self->text_heights[AUTOMATION_MODE_##caps] = y_px; \
  if (y_px > self->max_text_height) \
    self->max_text_height = y_px; \
  total_width += x_px

  DO (READ);
  DO (RECORD);
  DO (OFF);

  self->width =
    total_width +
    AUTOMATION_MODE_HPADDING * 6 +
    AUTOMATION_MODE_HSEPARATOR_SIZE * 2;
}

/**
 * Creates a new track widget from the given track.
 */
AutomationModeWidget *
automation_mode_widget_new (
  int             height,
  PangoLayout *   layout,
  AutomationTrack * owner)
{
  AutomationModeWidget * self =
    calloc (1, sizeof (AutomationModeWidget));

  self->owner = owner;
  self->height = height;
  self->layout = pango_layout_copy (layout);
  PangoFontDescription * desc =
    pango_font_description_from_string ("7");
  pango_layout_set_font_description (
    self->layout, desc);
  pango_font_description_free (desc);
  automation_mode_widget_init (self);

  return self;
}

static AutomationMode
get_hit_mode (
  AutomationModeWidget * self,
  double                 x)
{
  for (int i = 0; i < NUM_AUTOMATION_MODES - 1; i++)
    {
      int total_widths = 0;
      for (int j = 0; j <= i; j++)
        {
          total_widths += self->text_widths[j];
        }
      total_widths +=
        2 * AUTOMATION_MODE_HPADDING * (i + 1);
      double next_start = self->x + total_widths;
      /*g_message ("[%d] x: %f next start: %f",*/
        /*i, x, next_start);*/
      if (x < next_start)
        {
          return (AutomationMode) i;
        }
    }
  return AUTOMATION_MODE_OFF;
}

/**
 * Gets the color for \ref state for \ref mode.
 *
 * @param state new state.
 * @param mode AutomationMode.
 */
static void
get_color_for_state (
  AutomationModeWidget *  self,
  CustomButtonWidgetState state,
  AutomationMode          mode,
  GdkRGBA *               c)
{
  (void) c;
  switch (state)
    {
    case CUSTOM_BUTTON_WIDGET_STATE_NORMAL:
      *c = self->def_color;
      break;
    case CUSTOM_BUTTON_WIDGET_STATE_HOVERED:
      *c = self->hovered_color;
      break;
    case CUSTOM_BUTTON_WIDGET_STATE_ACTIVE:
      *c = self->held_colors[mode];
      break;
    case CUSTOM_BUTTON_WIDGET_STATE_TOGGLED:
      *c = self->toggled_colors[mode];
      break;
    default:
      g_warn_if_reached ();
    }
}

/**
 * @param mode Mode the state applies to.
 * @param state This state only applies to the mode.
 */
static void
draw_bg (
  AutomationModeWidget *  self,
  cairo_t *               cr,
  double                  x,
  double                  y,
  int                     draw_frame)
{
  if (draw_frame)
    {
      cairo_set_source_rgba (
        cr, 1, 1, 1, 0.4);
      cairo_set_line_width (cr, 0.5);
      z_cairo_rounded_rectangle (
        cr, x, y, self->width, self->height,
        self->aspect, self->corner_radius);
      cairo_stroke (cr);
    }

  /* draw bg with fade from last state */
  int draw_order[NUM_AUTOMATION_MODES] = {
    AUTOMATION_MODE_READ, AUTOMATION_MODE_OFF,
    AUTOMATION_MODE_RECORD };
  for (int idx = 0; idx < NUM_AUTOMATION_MODES; idx++)
    {
      int i = draw_order[idx];

      CustomButtonWidgetState cur_state =
        self->current_states[i];
      GdkRGBA c;
      get_color_for_state (
        self, cur_state, (AutomationMode) i, &c);
      if (self->last_states[i] != cur_state)
        {
          self->transition_frames =
            CUSTOM_BUTTON_WIDGET_MAX_TRANSITION_FRAMES;
        }

      /* draw transition if transition frames exist */
      if (self->transition_frames)
        {
          GdkRGBA mid_c;
          ui_get_mid_color (
            &mid_c, &c, &self->last_colors[i],
            1.0 -
            (double) self->transition_frames /
              (double) CUSTOM_BUTTON_WIDGET_MAX_TRANSITION_FRAMES);
          c = mid_c;
          if (i == NUM_AUTOMATION_MODES - 1)
            self->transition_frames--;
        }
      gdk_cairo_set_source_rgba (cr, &c);
      self->last_colors[i] = c;

      double new_x = x;
      int new_width =
        self->text_widths[i] +
        2 * AUTOMATION_MODE_HPADDING;
      int rounded = 1;
      switch (i)
        {
        case AUTOMATION_MODE_READ:
          new_width +=
            self->text_widths[i + 1];
          break;
        case AUTOMATION_MODE_RECORD:
          rounded = 0;
          new_x +=
            self->text_widths[0] +
            2 * AUTOMATION_MODE_HPADDING;
          break;
        case AUTOMATION_MODE_OFF:
          new_x +=
            self->text_widths[0] +
            2 * AUTOMATION_MODE_HPADDING +
            self->text_widths[1];
          new_width =
            ((int) x + self->width) - (int) new_x;
          break;
        }
      if (rounded)
        {
          z_cairo_rounded_rectangle (
            cr, new_x, y, new_width, self->height,
            self->aspect, self->corner_radius);
        }
      else
        {
          cairo_rectangle (
            cr, new_x, y, new_width, self->height);
        }
      cairo_fill (cr);
    }
}

void
automation_mode_widget_draw (
  AutomationModeWidget * self,
  cairo_t *                 cr,
  double                    x,
  double                    y,
  double                    x_cursor,
  CustomButtonWidgetState   state)
{
  /* get hit button */
  self->has_hit_mode = 0;
  if (state == CUSTOM_BUTTON_WIDGET_STATE_HOVERED ||
      state == CUSTOM_BUTTON_WIDGET_STATE_ACTIVE)
    {
      self->has_hit_mode = 1;
      self->hit_mode = get_hit_mode (self, x_cursor);
      /*g_message ("hit mode %d", self->hit_mode);*/
    }

  /* get current states */
  for (unsigned int i = 0;
       i < NUM_AUTOMATION_MODES; i++)
    {
      AutomationMode prev_am =
          self->owner->automation_mode;
      if (self->has_hit_mode &&
          i == self->hit_mode)
        {
          if (prev_am != i ||
              (prev_am == i &&
               state ==
                 CUSTOM_BUTTON_WIDGET_STATE_ACTIVE))
            {
              self->current_states[i] = state;
            }
          else
            {
              self->current_states[i] =
                CUSTOM_BUTTON_WIDGET_STATE_TOGGLED;
            }
        }
      else
        self->current_states[i] =
          self->owner->automation_mode ==
            i ?
              CUSTOM_BUTTON_WIDGET_STATE_TOGGLED :
              CUSTOM_BUTTON_WIDGET_STATE_NORMAL;
    }

  draw_bg (self, cr, x, y, false);

  /*draw_icon_with_shadow (self, cr, x, y, state);*/

  /* draw text */
  int total_text_widths = 0;
  for (int i = 0; i < NUM_AUTOMATION_MODES; i++)
    {
      PangoLayout * layout = self->layout;
      cairo_set_source_rgba (
        cr, 1, 1, 1, 1);
      cairo_move_to (
        cr,
        x + AUTOMATION_MODE_HPADDING +
          i * (2 * AUTOMATION_MODE_HPADDING) +
          total_text_widths,
        (y + self->height / 2) -
          self->text_heights[i] / 2);
      char mode_str[400];
      if (i == AUTOMATION_MODE_RECORD)
        {
          automation_record_mode_get_localized (
            self->owner->record_mode, mode_str);
          pango_layout_set_text (
            layout, mode_str, -1);
        }
      else
        {
          automation_mode_get_localized (
            (AutomationMode) i, mode_str);
          pango_layout_set_text (
            layout, mode_str, -1);
        }
      pango_cairo_show_layout (cr, layout);

      total_text_widths += self->text_widths[i];

      self->last_states[i] = self->current_states[i];
    }
}

void
automation_mode_widget_free (
  AutomationModeWidget * self)
{
  free (self);
}

