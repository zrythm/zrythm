// SPDX-FileCopyrightText: © 2018-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "audio/control_port.h"
#include "audio/instrument_track.h"
#include "audio/track.h"
#include "gui/widgets/automation_mode.h"
#include "gui/widgets/custom_button.h"
#include "gui/widgets/track.h"
#include "gui/widgets/track_canvas.h"
#include "utils/color.h"
#include "utils/gtk.h"
#include "utils/objects.h"
#include "utils/string.h"
#include "utils/ui.h"
#include "zrythm.h"
#include "zrythm_app.h"

G_DEFINE_TYPE (
  TrackCanvasWidget,
  track_canvas_widget,
  GTK_TYPE_WIDGET)

#ifdef _WOE32
#  define AUTOMATABLE_NAME_FONT "8.5"
#  define AUTOMATABLE_VAL_FONT "7"
#else
#  define AUTOMATABLE_NAME_FONT "9.5"
#  define AUTOMATABLE_VAL_FONT "8"
#endif

/** Width of the "label" to show the automation
 * value. */
#define AUTOMATION_VALUE_WIDTH 32

#define icon_texture_size 16

static void
recreate_icon_texture (TrackCanvasWidget * self)
{
  object_free_w_func_and_null (
    g_object_unref, self->track_icon);
  self
    ->track_icon = z_gdk_texture_new_from_icon_name (
    self->parent->track->icon_name,
    icon_texture_size, icon_texture_size, 1);
  object_free_w_func_and_null (
    g_free, self->last_track_icon_name);
  self->last_track_icon_name =
    g_strdup (self->parent->track->icon_name);
}

static void
update_pango_layouts (
  TrackCanvasWidget * self,
  int                 w,
  int                 h)
{
  if (!self->layout)
    {
      self->layout = gtk_widget_create_pango_layout (
        GTK_WIDGET (self), NULL);
      pango_layout_set_ellipsize (
        self->layout, PANGO_ELLIPSIZE_END);
    }

  if (!self->automation_value_layout)
    {
      self->automation_value_layout =
        gtk_widget_create_pango_layout (
          GTK_WIDGET (self), NULL);
      PangoFontDescription * desc =
        pango_font_description_from_string (
          AUTOMATABLE_VAL_FONT);
      pango_layout_set_font_description (
        self->automation_value_layout, desc);
      pango_font_description_free (desc);
    }

  pango_layout_set_width (
    self->layout, pango_units_from_double (w));
  pango_layout_set_width (
    self->automation_value_layout,
    pango_units_from_double (w));
}

/**
 * @param height Total track widget height.
 */
static void
draw_color_area (
  TrackCanvasWidget * self,
  GtkSnapshot *       snapshot,
  GdkRectangle *      rect,
  int                 height)
{
  TrackWidget * tw = self->parent;
  Track *       track = tw->track;

  /* draw background */
  GdkRGBA bg_color = tw->track->color;
  if (!track_is_enabled (track))
    {
      bg_color.red = 0.5;
      bg_color.green = 0.5;
      bg_color.blue = 0.5;
    }
  if (tw->color_area_hovered)
    {
      color_brighten_default (&bg_color);
    }
  gtk_snapshot_append_color (
    snapshot, &bg_color,
    &GRAPHENE_RECT_INIT (
      0, 0, TRACK_COLOR_AREA_WIDTH, height));

  /* TODO */
  GdkRGBA c2, c3;
  ui_get_contrast_color (&track->color, &c2);
  ui_get_contrast_color (&c2, &c3);

#if 0
  /* add shadow in the back */
  cairo_set_source_rgba (
    cr, c3.red, c3.green, c3.blue, 0.4);
  cairo_mask_surface (
    cr, surface, 2, 2);
  cairo_fill (cr);
#endif

  /* add main icon */
  if (tw->icon_hovered)
    {
      color_brighten_default (&c2);
    }

#if 0
  cairo_set_source_rgba (
    cr, c2.red, c2.green, c2.blue, 1);
  /*cairo_set_source_surface (*/
    /*self->cached_cr, surface, 1, 1);*/
  cairo_mask_surface (
    cr, surface, 1, 1);
  cairo_fill (cr);
#endif

  if (
    !self->track_icon
    || !string_is_equal (
      self->last_track_icon_name, track->icon_name))
    {
      recreate_icon_texture (self);
    }

  /* TODO figure out how to draw dark colors */
  graphene_matrix_t color_matrix;
  graphene_matrix_init_from_float (
    &color_matrix,
    (float[16]){
      1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 0, 0, 0,
      c2.alpha });
  graphene_vec4_t color_offset;
  graphene_vec4_init (
    &color_offset, c2.red, c2.green, c2.blue, 0);

  gtk_snapshot_push_color_matrix (
    snapshot, &color_matrix, &color_offset);

  gtk_snapshot_append_texture (
    snapshot, self->track_icon,
    &GRAPHENE_RECT_INIT (
      0, 0, icon_texture_size, icon_texture_size));

  gtk_snapshot_pop (snapshot);
}

static void
draw_name (
  TrackCanvasWidget * self,
  GtkSnapshot *       snapshot,
  int                 width)
{
  TrackWidget * tw = self->parent;
  Track *       track = tw->track;

  char name[strlen (track->name) + 10];
  if (DEBUGGING)
    {
      sprintf (
        name, "%s[%d - %d] %s",
        track_is_selected (track) ? "* " : "",
        track->pos, track->size, track->name);
    }
  else
    strcpy (name, track->name);

  int first_button_x =
    width
    - (TRACK_BUTTON_SIZE + TRACK_BUTTON_PADDING)
        * tw->num_top_buttons;

  PangoLayout * layout = self->layout;
  pango_layout_set_text (layout, name, -1);
  pango_layout_set_width (
    layout,
    pango_units_from_double (first_button_x - 22));

  GtkStyleContext * context =
    gtk_widget_get_style_context (GTK_WIDGET (self));
  gtk_snapshot_render_layout (
    snapshot, context, 22, 2, layout);
}

/**
 * @param top 1 to draw top, 0 to draw bottom.
 * @param width Track width.
 */
static void
draw_buttons (
  TrackCanvasWidget * self,
  GtkSnapshot *       snapshot,
  int                 top,
  int                 width)
{
  TrackWidget * tw = self->parent;

  CustomButtonWidget * hovered_cb =
    track_widget_get_hovered_button (
      tw, (int) tw->last_x, (int) tw->last_y);
  int num_buttons =
    top ? tw->num_top_buttons : tw->num_bot_buttons;
  CustomButtonWidget ** buttons =
    top ? tw->top_buttons : tw->bot_buttons;
  Track * track = tw->track;
  for (int i = 0; i < num_buttons; i++)
    {
      CustomButtonWidget * cb = buttons[i];

      if (top)
        {
          cb->x =
            width
            - (TRACK_BUTTON_SIZE
               + TRACK_BUTTON_PADDING)
                * (num_buttons - i);
          cb->y = TRACK_BUTTON_PADDING_FROM_EDGE;
        }
      else
        {
          cb->x =
            width
            - (TRACK_BUTTON_SIZE
               + TRACK_BUTTON_PADDING)
                * (tw->num_bot_buttons - i);
          cb->y =
            track->main_height
            - (TRACK_BUTTON_PADDING_FROM_EDGE + TRACK_BUTTON_SIZE);
        }

      CustomButtonWidgetState state =
        CUSTOM_BUTTON_WIDGET_STATE_NORMAL;

      bool is_solo = TRACK_CB_ICON_IS (SOLO);

      if (cb == tw->clicked_button)
        {
          /* currently clicked button */
          state = CUSTOM_BUTTON_WIDGET_STATE_ACTIVE;
        }
      else if (is_solo && track_get_soloed (track))
        {
          state =
            CUSTOM_BUTTON_WIDGET_STATE_TOGGLED;
        }
      else if (
        is_solo && track_get_implied_soloed (track))
        {
          state =
            CUSTOM_BUTTON_WIDGET_STATE_SEMI_TOGGLED;
        }
      else if (
        TRACK_CB_ICON_IS (SHOW_UI)
        && instrument_track_is_plugin_visible (
          track))
        {
          state =
            CUSTOM_BUTTON_WIDGET_STATE_TOGGLED;
        }
      else if (
        TRACK_CB_ICON_IS (MUTE)
        && track_get_muted (track))
        {
          state =
            CUSTOM_BUTTON_WIDGET_STATE_TOGGLED;
        }
      else if (
        TRACK_CB_ICON_IS (LISTEN)
        && track_get_listened (track))
        {
          state =
            CUSTOM_BUTTON_WIDGET_STATE_TOGGLED;
        }
      else if (
        TRACK_CB_ICON_IS (MONITOR_AUDIO)
        && track_get_monitor_audio (track))
        {
          state =
            CUSTOM_BUTTON_WIDGET_STATE_TOGGLED;
        }
      else if (
        TRACK_CB_ICON_IS (FREEZE) && track->frozen)
        {
          state =
            CUSTOM_BUTTON_WIDGET_STATE_TOGGLED;
        }
      else if (
        TRACK_CB_ICON_IS (MONO_COMPAT)
        && channel_get_mono_compat_enabled (
          track->channel))
        {
          state =
            CUSTOM_BUTTON_WIDGET_STATE_TOGGLED;
        }
      else if (
        TRACK_CB_ICON_IS (RECORD)
        && track_get_recording (track))
        {
          state =
            CUSTOM_BUTTON_WIDGET_STATE_TOGGLED;
        }
      else if (
        TRACK_CB_ICON_IS (SHOW_TRACK_LANES)
        && track->lanes_visible)
        {
          state =
            CUSTOM_BUTTON_WIDGET_STATE_TOGGLED;
        }
      else if (
        TRACK_CB_ICON_IS (SHOW_AUTOMATION_LANES)
        && track->automation_visible)
        {
          state =
            CUSTOM_BUTTON_WIDGET_STATE_TOGGLED;
        }
      else if (TRACK_CB_ICON_IS (FOLD_OPEN))
        {
          state =
            CUSTOM_BUTTON_WIDGET_STATE_TOGGLED;
        }
      else if (hovered_cb == cb)
        {
          state =
            CUSTOM_BUTTON_WIDGET_STATE_HOVERED;
        }

      custom_button_widget_draw (
        cb, snapshot, cb->x, cb->y, state);
    }
}

static void
draw_lanes (
  TrackCanvasWidget * self,
  GtkSnapshot *       snapshot,
  int                 width)
{
  TrackWidget * tw = self->parent;
  Track *       track = tw->track;
  g_return_if_fail (track);

  if (!track->lanes_visible)
    return;

  int total_height = (int) track->main_height;
  for (int i = 0; i < track->num_lanes; i++)
    {
      TrackLane * lane = track->lanes[i];

      /* remember y */
      lane->y = total_height;

      /* draw separator */
      gtk_snapshot_append_color (
        snapshot, &Z_GDK_RGBA_INIT (1, 1, 1, 0.3f),
        &GRAPHENE_RECT_INIT (
          TRACK_COLOR_AREA_WIDTH, total_height,
          width - TRACK_COLOR_AREA_WIDTH, 1));

      /* draw text */
      gtk_snapshot_save (snapshot);
      gtk_snapshot_translate (
        snapshot,
        &GRAPHENE_POINT_INIT (
          TRACK_COLOR_AREA_WIDTH
            + TRACK_BUTTON_PADDING_FROM_EDGE,
          total_height
            + TRACK_BUTTON_PADDING_FROM_EDGE));
      PangoLayout * layout = self->layout;
      pango_layout_set_text (
        layout, lane->name, -1);
      gtk_snapshot_append_layout (
        snapshot, layout,
        &Z_GDK_RGBA_INIT (1, 1, 1, 1));
      gtk_snapshot_restore (snapshot);

      /* create buttons if necessary */
      CustomButtonWidget * cb;
      if (lane->num_buttons == 0)
        {
          lane
            ->buttons[0] = custom_button_widget_new (
            TRACK_ICON_NAME_SOLO,
            TRACK_BUTTON_SIZE);
          cb = lane->buttons[0];
          cb->owner_type =
            CUSTOM_BUTTON_WIDGET_OWNER_LANE;
          cb->owner = lane;
          cb->toggled_color =
            UI_COLORS->solo_checked;
          cb->held_color = UI_COLORS->solo_active;
          lane
            ->buttons[1] = custom_button_widget_new (
            TRACK_ICON_NAME_MUTE,
            TRACK_BUTTON_SIZE);
          cb = lane->buttons[1];
          cb->owner_type =
            CUSTOM_BUTTON_WIDGET_OWNER_LANE;
          cb->owner = lane;
          lane->num_buttons = 2;
        }

      /* draw buttons */
      CustomButtonWidget * hovered_cb =
        track_widget_get_hovered_button (
          tw, (int) tw->last_x, (int) tw->last_y);
      for (int j = 0; j < lane->num_buttons; j++)
        {
          cb = lane->buttons[j];

          cb->x =
            width
            - (TRACK_BUTTON_SIZE
               + TRACK_BUTTON_PADDING)
                * (lane->num_buttons - j);
          cb->y =
            total_height
            + TRACK_BUTTON_PADDING_FROM_EDGE;

          CustomButtonWidgetState state =
            CUSTOM_BUTTON_WIDGET_STATE_NORMAL;

          if (cb == tw->clicked_button)
            {
              /* currently clicked button */
              state =
                CUSTOM_BUTTON_WIDGET_STATE_ACTIVE;
            }
          else if (
            TRACK_CB_ICON_IS (SOLO) && lane->solo)
            {
              state =
                CUSTOM_BUTTON_WIDGET_STATE_TOGGLED;
            }
          else if (
            TRACK_CB_ICON_IS (MUTE) && lane->mute)
            {
              state =
                CUSTOM_BUTTON_WIDGET_STATE_TOGGLED;
            }
          else if (hovered_cb == cb)
            {
              state =
                CUSTOM_BUTTON_WIDGET_STATE_HOVERED;
            }

          custom_button_widget_draw (
            cb, snapshot, cb->x, cb->y, state);
        }

      total_height += (int) lane->height;
    }
}

static void
draw_automation (
  TrackCanvasWidget * self,
  GtkSnapshot *       snapshot,
  int                 width)
{
  TrackWidget * tw = self->parent;
  Track *       track = tw->track;
  g_return_if_fail (track);

  if (!track->automation_visible)
    return;

  AutomationTracklist * atl =
    track_get_automation_tracklist (track);
  g_return_if_fail (atl);
  int total_height = (int) track->main_height;

  if (track->lanes_visible)
    {
      for (int j = 0; j < track->num_lanes; j++)
        {
          TrackLane * lane = track->lanes[j];
          total_height += (int) lane->height;
        }
    }

  for (int i = 0; i < atl->num_ats; i++)
    {
      AutomationTrack * at = atl->ats[i];

      if (!(at->created && at->visible))
        continue;

      /* remember y */
      at->y = total_height;

      /* draw separator above at */
      gtk_snapshot_append_color (
        snapshot, &Z_GDK_RGBA_INIT (1, 1, 1, 0.3),
        &GRAPHENE_RECT_INIT (
          TRACK_COLOR_AREA_WIDTH, total_height,
          width - TRACK_COLOR_AREA_WIDTH, 1));

      /* create buttons if necessary */
      CustomButtonWidget * cb;
      if (at->num_top_left_buttons == 0)
        {
          at->top_left_buttons
            [0] = custom_button_widget_new (
            TRACK_ICON_NAME_SHOW_AUTOMATION_LANES,
            TRACK_BUTTON_SIZE);
          cb = at->top_left_buttons[0];
          cb->owner_type =
            CUSTOM_BUTTON_WIDGET_OWNER_AT;
          cb->owner = at;
          /*char text[500];*/
          /*sprintf (*/
          /*text, "%d - %s",*/
          /*at->index, at->automatable->label);*/
          custom_button_widget_set_text (
            cb, self->layout, at->port_id.label,
            AUTOMATABLE_NAME_FONT);
          pango_layout_set_ellipsize (
            cb->layout, PANGO_ELLIPSIZE_END);
          at->num_top_left_buttons = 1;
        }
      if (at->num_top_right_buttons == 0)
        {
#if 0
          at->top_right_buttons[0] =
            custom_button_widget_new (
              TRACK_ICON_NAME_MUTE, TRACK_BUTTON_SIZE);
          at->top_right_buttons[0]->owner_type =
            CUSTOM_BUTTON_WIDGET_OWNER_AT;
          at->top_right_buttons[0]->owner = at;
          at->num_top_right_buttons = 1;
#endif
        }
      if (at->num_bot_left_buttons == 0) { }
      if (!at->am_widget)
        {
          at->am_widget =
            automation_mode_widget_new (
              TRACK_BUTTON_SIZE, self->layout, at);
        }
      if (at->num_bot_right_buttons == 0)
        {
          at->bot_right_buttons[0] =
            custom_button_widget_new (
              TRACK_ICON_NAME_MINUS,
              TRACK_BUTTON_SIZE);
          at->bot_right_buttons[0]->owner_type =
            CUSTOM_BUTTON_WIDGET_OWNER_AT;
          at->bot_right_buttons[0]->owner = at;
          at->bot_right_buttons[1] =
            custom_button_widget_new (
              TRACK_ICON_NAME_PLUS,
              TRACK_BUTTON_SIZE);
          at->bot_right_buttons[1]->owner_type =
            CUSTOM_BUTTON_WIDGET_OWNER_AT;
          at->bot_right_buttons[1]->owner = at;
          at->num_bot_right_buttons = 2;
        }

      /* draw top left buttons */
      CustomButtonWidget * hovered_cb =
        track_widget_get_hovered_button (
          tw, (int) tw->last_x, (int) tw->last_y);
      AutomationModeWidget * hovered_am =
        track_widget_get_hovered_am_widget (
          tw, (int) tw->last_x, (int) tw->last_y);
      for (int j = 0; j < at->num_top_left_buttons;
           j++)
        {
          cb = at->top_left_buttons[j];

          cb->x =
            TRACK_BUTTON_PADDING_FROM_EDGE
            + TRACK_COLOR_AREA_WIDTH;
          cb->y =
            total_height
            + TRACK_BUTTON_PADDING_FROM_EDGE;

          CustomButtonWidgetState state =
            CUSTOM_BUTTON_WIDGET_STATE_NORMAL;

          if (cb == tw->clicked_button)
            {
              /* currently clicked button */
              state =
                CUSTOM_BUTTON_WIDGET_STATE_ACTIVE;
            }
          else if (hovered_cb == cb)
            {
              state =
                CUSTOM_BUTTON_WIDGET_STATE_HOVERED;
            }

          custom_button_widget_draw_with_text (
            cb, snapshot, cb->x, cb->y,
            width
              - (TRACK_COLOR_AREA_WIDTH + TRACK_BUTTON_PADDING_FROM_EDGE * 2 + at->num_top_right_buttons * (TRACK_BUTTON_SIZE + TRACK_BUTTON_PADDING) + AUTOMATION_VALUE_WIDTH + TRACK_BUTTON_PADDING),
            state);
        }

      /* draw automation value */
      PangoLayout * layout =
        self->automation_value_layout;
      char   str[50];
      Port * port =
        port_find_from_identifier (&at->port_id);
      sprintf (
        str, "%.2f",
        (double) control_port_get_val (port));
      cb = at->top_left_buttons[0];
      pango_layout_set_text (layout, str, -1);
      PangoRectangle pangorect;
      pango_layout_get_pixel_extents (
        layout, NULL, &pangorect);

      float orig_start_x =
        (float) (cb->x + cb->width + TRACK_BUTTON_PADDING);
      float orig_start_y =
        (float) total_height
        + TRACK_BUTTON_PADDING_FROM_EDGE;
      float remaining_x =
        (float) (width - orig_start_x)
        - pangorect.width;
      float remaining_y =
        (float) cb->size - pangorect.height;

      gtk_snapshot_save (snapshot);
      gtk_snapshot_translate (
        snapshot,
        &GRAPHENE_POINT_INIT (
          orig_start_x + remaining_x / 2.f,
          orig_start_y + remaining_y / 2.f));
      gtk_snapshot_append_layout (
        snapshot, layout,
        &Z_GDK_RGBA_INIT (1, 1, 1, 1));
      gtk_snapshot_restore (snapshot);

      /* draw top right buttons */
      for (int j = 0;
           j < at->num_top_right_buttons; j++)
        {
          cb = at->top_right_buttons[j];

          cb->x =
            width
            - (TRACK_BUTTON_SIZE
               + TRACK_BUTTON_PADDING)
                * (at->num_top_right_buttons - j);
          cb->y =
            total_height
            + TRACK_BUTTON_PADDING_FROM_EDGE;

          CustomButtonWidgetState state =
            CUSTOM_BUTTON_WIDGET_STATE_NORMAL;

          if (cb == tw->clicked_button)
            {
              /* currently clicked button */
              state =
                CUSTOM_BUTTON_WIDGET_STATE_ACTIVE;
            }
          else if (hovered_cb == cb)
            {
              state =
                CUSTOM_BUTTON_WIDGET_STATE_HOVERED;
            }

          custom_button_widget_draw (
            cb, snapshot, cb->x, cb->y, state);
        }

      if (TRACK_BOT_BUTTONS_SHOULD_BE_VISIBLE (
            at->height))
        {
          /* automation mode */
          AutomationModeWidget * am = at->am_widget;

          am->x =
            TRACK_BUTTON_PADDING_FROM_EDGE
            + TRACK_COLOR_AREA_WIDTH;
          am->y =
            (total_height + at->height)
            - (TRACK_BUTTON_PADDING_FROM_EDGE + TRACK_BUTTON_SIZE);

          CustomButtonWidgetState state =
            CUSTOM_BUTTON_WIDGET_STATE_NORMAL;
          if (tw->clicked_am == am)
            {
              /* currently clicked button */
              state =
                CUSTOM_BUTTON_WIDGET_STATE_ACTIVE;
            }
          else if (hovered_am == am)
            {
              state =
                CUSTOM_BUTTON_WIDGET_STATE_HOVERED;
            }

          automation_mode_widget_draw (
            am, snapshot, am->x, am->y, tw->last_x,
            state);

          for (int j = 0;
               j < at->num_bot_right_buttons; j++)
            {
              cb = at->bot_right_buttons[j];

              cb->x =
                width
                - (TRACK_BUTTON_SIZE
                   + TRACK_BUTTON_PADDING)
                    * (at->num_bot_right_buttons - j);
              cb->y =
                (total_height + at->height)
                - (TRACK_BUTTON_PADDING_FROM_EDGE + TRACK_BUTTON_SIZE);

              state =
                CUSTOM_BUTTON_WIDGET_STATE_NORMAL;

              if (cb == tw->clicked_button)
                {
                  /* currently clicked button */
                  state =
                    CUSTOM_BUTTON_WIDGET_STATE_ACTIVE;
                }
              else if (hovered_cb == cb)
                {
                  state =
                    CUSTOM_BUTTON_WIDGET_STATE_HOVERED;
                }

              custom_button_widget_draw (
                cb, snapshot, cb->x, cb->y, state);
            }
        }
      total_height += (int) at->height;
    }
}

static void
track_canvas_snapshot (
  GtkWidget *   widget,
  GtkSnapshot * snapshot)
{
  TrackCanvasWidget * self =
    Z_TRACK_CANVAS_WIDGET (widget);

  int width =
    gtk_widget_get_allocated_width (widget);
  int height =
    gtk_widget_get_allocated_height (widget);

  GtkStyleContext * context =
    gtk_widget_get_style_context (widget);

  gtk_snapshot_render_background (
    snapshot, context, 0, 0, width, height);

  if (
    self->last_width != width
    || self->last_height != height)
    {
      update_pango_layouts (self, width, height);
    }

  TrackWidget * tw = self->parent;
  Track *       track = tw->track;

  GdkRectangle rect = {
    .x = 0, .y = 0, .width = width, .height = height
  };

  if (tw->highlight_inside)
    {
      /* TODO */
#if 0
      gdk_cairo_set_source_rgba (
        self->cached_cr,
        &UI_COLORS->bright_orange);
      const int line_w = MIN (128, width);
      const int line_h = 4;
      cairo_rectangle (
        self->cached_cr,
        width / 2 - line_w / 2,
        height / 2 - line_h / 2,
        line_w, line_h);
      cairo_fill (self->cached_cr);
#endif
    }

  /* tint background */
  gtk_snapshot_append_color (
    snapshot,
    &Z_GDK_RGBA_INIT (
      track->color.red, track->color.green,
      track->color.blue, 0.15),
    &GRAPHENE_RECT_INIT (0, 0, width, height));

  if (tw->bg_hovered)
    {
      gtk_snapshot_append_color (
        snapshot, &Z_GDK_RGBA_INIT (1, 1, 1, 0.1),
        &GRAPHENE_RECT_INIT (0, 0, width, height));
    }
  else if (track_is_selected (track))
    {
      gtk_snapshot_append_color (
        snapshot, &Z_GDK_RGBA_INIT (1, 1, 1, 0.07),
        &GRAPHENE_RECT_INIT (0, 0, width, height));
    }

  draw_color_area (self, snapshot, &rect, height);

  draw_name (self, snapshot, width);

  draw_buttons (self, snapshot, 1, width);

  /* only show bot buttons if enough space */
  if (TRACK_BOT_BUTTONS_SHOULD_BE_VISIBLE (
        track->main_height))
    {
      draw_buttons (self, snapshot, 0, width);
    }

  draw_lanes (self, snapshot, width);

  draw_automation (self, snapshot, width);

  self->last_width = width;
  self->last_height = height;
}

void
track_canvas_widget_setup (
  TrackCanvasWidget * self,
  TrackWidget *       parent)
{
  self->parent = parent;
}

static void
finalize (TrackCanvasWidget * self)
{
  object_free_w_func_and_null (
    g_object_unref, self->track_icon);
  object_free_w_func_and_null (
    g_object_unref, self->layout);
  object_free_w_func_and_null (
    g_free, self->last_track_icon_name);

  G_OBJECT_CLASS (track_canvas_widget_parent_class)
    ->finalize (G_OBJECT (self));
}

static gboolean
tick_cb (
  GtkWidget *     widget,
  GdkFrameClock * frame_clock,
  gpointer        user_data)
{
  TrackCanvasWidget * self =
    Z_TRACK_CANVAS_WIDGET (user_data);

  TrackWidget * tw = self->parent;
  Track *       track = tw->track;
  if (!track)
    return G_SOURCE_CONTINUE;

  if (track_is_selected (track))
    {
      gtk_widget_add_css_class (
        widget, "caption-heading");
      gtk_widget_remove_css_class (
        widget, "caption");
    }
  else
    {
      gtk_widget_add_css_class (widget, "caption");
      gtk_widget_remove_css_class (
        widget, "caption-heading");
    }

  return G_SOURCE_CONTINUE;
}

static void
track_canvas_widget_init (TrackCanvasWidget * self)
{
  gtk_widget_add_tick_callback (
    GTK_WIDGET (self), tick_cb, self, NULL);
}

static void
track_canvas_widget_class_init (
  TrackCanvasWidgetClass * klass)
{
  GtkWidgetClass * wklass =
    GTK_WIDGET_CLASS (klass);
  wklass->snapshot = track_canvas_snapshot;
  gtk_widget_class_set_css_name (
    wklass, "track-canvas");

  GObjectClass * oklass = G_OBJECT_CLASS (klass);
  oklass->finalize = (GObjectFinalizeFunc) finalize;
}
