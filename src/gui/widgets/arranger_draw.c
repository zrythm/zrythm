// SPDX-FileCopyrightText: © 2018-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "audio/control_port.h"
#include "audio/fade.h"
#include "audio/track_lane.h"
#include "audio/tracklist.h"
#include "gui/backend/arranger_object.h"
#include "gui/widgets/arranger_draw.h"
#include "gui/widgets/arranger_object.h"
#include "gui/widgets/automation_arranger.h"
#include "gui/widgets/bot_bar.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/clip_editor.h"
#include "gui/widgets/clip_editor_inner.h"
#include "gui/widgets/cpu.h"
#include "gui/widgets/editor_ruler.h"
#include "gui/widgets/main_notebook.h"
#include "gui/widgets/midi_arranger.h"
#include "gui/widgets/midi_editor_space.h"
#include "gui/widgets/midi_modifier_arranger.h"
#include "gui/widgets/piano_roll_keys.h"
#include "gui/widgets/ruler.h"
#include "gui/widgets/timeline_arranger.h"
#include "gui/widgets/timeline_panel.h"
#include "gui/widgets/timeline_ruler.h"
#include "gui/widgets/track.h"
#include "gui/widgets/tracklist.h"
#include "project.h"
#include "settings/settings.h"
#include "utils/cairo.h"
#include "utils/color.h"
#include "utils/debug.h"
#include "utils/flags.h"
#include "utils/gtk.h"
#include "utils/math.h"
#include "utils/object_pool.h"
#include "utils/objects.h"
#include "zrythm_app.h"

/*#include <valgrind/callgrind.h>*/

#define TYPE(x) ARRANGER_WIDGET_TYPE_##x

static const GdkRGBA thick_grid_line_color = {
  0.3f, 0.3f, 0.3f, 0.9f
};

static void
draw_selections (ArrangerWidget * self, GtkSnapshot * snapshot)
{
  double offset_x, offset_y;
  offset_x =
    self->start_x + self->last_offset_x > 0
      ? self->last_offset_x
      : 1 - self->start_x;
  offset_y =
    self->start_y + self->last_offset_y > 0
      ? self->last_offset_y
      : 1 - self->start_y;

  GskRoundedRect  rounded_rect;
  graphene_rect_t graphene_rect = GRAPHENE_RECT_INIT (
    (float) self->start_x, (float) self->start_y,
    (float) (offset_x), (float) (offset_y));
  gsk_rounded_rect_init_from_rect (
    &rounded_rect, &graphene_rect, 0);
  const float border_width = 2.f;
  GdkRGBA     border_color = { 0.9f, 0.9f, 0.9f, 0.9f };
  float       border_widths[] = {
          border_width, border_width, border_width, border_width
  };
  GdkRGBA border_colors[] = {
    border_color, border_color, border_color, border_color
  };
  GdkRGBA inside_color = {
    border_color.red / 3.f,
    border_color.green / 3.f,
    border_color.blue / 3.f,
    border_color.alpha / 3.f,
  };

  /* if action is selecting and not selecting range
   * (in the case of timeline */
  switch (self->action)
    {
    case UI_OVERLAY_ACTION_SELECTING:
    case UI_OVERLAY_ACTION_DELETE_SELECTING:
      {
        gtk_snapshot_append_color (
          snapshot, &inside_color, &graphene_rect);
        gtk_snapshot_append_border (
          snapshot, &rounded_rect, border_widths,
          border_colors);
      }
      break;
    case UI_OVERLAY_ACTION_RAMPING:
      {
        cairo_t * cr = gtk_snapshot_append_cairo (
          snapshot, &graphene_rect);
        gdk_cairo_set_source_rgba (cr, &border_color);
        cairo_set_line_width (cr, 2.0);
        cairo_move_to (cr, self->start_x, self->start_y);
        cairo_line_to (
          cr, self->start_x + offset_x,
          self->start_y + offset_y);
        cairo_stroke (cr);
        cairo_destroy (cr);
      }
      break;
    default:
      break;
    }
}

static void
draw_highlight (
  ArrangerWidget * self,
  GtkSnapshot *    snapshot,
  GdkRectangle *   rect)
{
  if (!self->is_highlighted)
    {
      return;
    }

  /* this wasn't tested since porting to gtk4 */
  gtk_snapshot_append_color (
    snapshot, &UI_COLORS->bright_orange,
    &GRAPHENE_RECT_INIT (
      (float) ((self->highlight_rect.x + 1)),
      (float) ((self->highlight_rect.y + 1)),
      (float) (self->highlight_rect.width - 1),
      (float) (self->highlight_rect.height - 1)));
}

/**
 * @param rect Arranger's rectangle.
 */
static void
draw_arranger_object (
  ArrangerWidget * self,
  ArrangerObject * obj,
  GtkSnapshot *    snapshot,
  GdkRectangle *   rect)
{
  /* loop once or twice (2nd time for transient) */
  for (
    int i = 0;
    i
    < 1 + (arranger_object_should_orig_be_visible (obj, self) && arranger_object_is_selected (obj));
    i++)
    {
      /* if looping 2nd time (transient) */
      if (i == 1)
        {
          g_return_if_fail (obj->transient);
          obj = obj->transient;
        }

      arranger_object_set_full_rectangle (obj, self);

      /* only draw if the object's rectangle is
       * hit by the drawable region (for regions,
       * the logic is handled inside region_draw()
       * so the check is skipped) */
      bool rect_hit_or_region =
        ui_rectangle_overlap (&obj->full_rect, rect)
        || obj->type == ARRANGER_OBJECT_TYPE_REGION;

      Track * track = arranger_object_get_track (obj);
      bool    should_be_visible =
        (track->visible
         && self->is_pinned == track_is_pinned (track))
        || self->type != ARRANGER_WIDGET_TYPE_TIMELINE;

      if (rect_hit_or_region && should_be_visible)
        {
          arranger_object_draw (obj, self, snapshot, rect);
        }
    }
}

/**
 * @param rect Arranger draw rectangle.
 */
static void
draw_playhead (
  ArrangerWidget * self,
  GtkSnapshot *    snapshot,
  GdkRectangle *   rect)
{
  int cur_playhead_px = arranger_widget_get_playhead_px (self);
  int px = cur_playhead_px;
  /*int px = self->queued_playhead_px;*/
  /*g_message ("drawing %d", px);*/

  int height = rect->height;

  if (px >= rect->x && px <= rect->x + rect->width)
    {
      gtk_snapshot_append_color (
        snapshot, &UI_COLORS->prefader_send,
        &GRAPHENE_RECT_INIT (
          (float) px - 1.f, (float) rect->y, 2.f,
          (float) height));
      self->last_playhead_px = px;
    }

  /* draw faded playhead if in audition mode */
  if (P_TOOL == TOOL_AUDITION)
    {
      bool   hovered = false;
      double hover_x = 0.f;
      switch (self->type)
        {
        case ARRANGER_WIDGET_TYPE_TIMELINE:
          if (MW_TIMELINE->hovered)
            {
              hovered = true;
              hover_x = MW_TIMELINE->hover_x;
            }
          if (MW_PINNED_TIMELINE->hovered)
            {
              hovered = true;
              hover_x = MW_PINNED_TIMELINE->hover_x;
            }
          break;
        case ARRANGER_WIDGET_TYPE_MIDI:
        case ARRANGER_WIDGET_TYPE_MIDI_MODIFIER:
          if (MW_MIDI_MODIFIER_ARRANGER->hovered)
            {
              hovered = true;
              hover_x = MW_MIDI_MODIFIER_ARRANGER->hover_x;
            }
          if (MW_MIDI_ARRANGER->hovered)
            {
              hovered = true;
              hover_x = MW_MIDI_ARRANGER->hover_x;
            }
          break;
        default:
          hovered = self->hovered;
          hover_x = self->hover_x;
          break;
        }

      if (hovered)
        {
          GdkRGBA color = UI_COLORS->prefader_send;
          color.alpha = 0.6f;
          gtk_snapshot_append_color (
            snapshot, &color,
            &GRAPHENE_RECT_INIT (
              (float) hover_x - 1, (float) rect->y, 2.f,
              (float) height));
        }
    }
}

static void
draw_debug_text (
  ArrangerWidget * self,
  GtkSnapshot *    snapshot,
  GdkRectangle *   rect)
{
  gtk_snapshot_append_color (
    snapshot, &Z_GDK_RGBA_INIT (0, 0, 0, 0.4),
    &GRAPHENE_RECT_INIT (
      (float) rect->x, (float) rect->y, 400.f, 400.f));
  GtkStyleContext * style_ctx =
    gtk_widget_get_style_context (GTK_WIDGET (self));
  char                   debug_txt[6000];
  const EditorSettings * settings =
    arranger_widget_get_editor_settings (self);
  sprintf (
    debug_txt,
    "Hover: (%.0f, %.0f)\nNormalized hover: (%.0f, %.0f)\nAction: %s",
    self->hover_x, self->hover_y,
    self->hover_x - settings->scroll_start_x,
    self->hover_y - settings->scroll_start_y,
    ui_get_overlay_action_string (self->action));
  pango_layout_set_markup (self->debug_layout, debug_txt, -1);
  gtk_snapshot_render_layout (
    snapshot, style_ctx, rect->x + 4, rect->y + 4,
    self->debug_layout);
}

static void
draw_timeline_bg (
  ArrangerWidget * self,
  GtkSnapshot *    snapshot,
  GdkRectangle *   rect)
{
  /* handle horizontal drawing for tracks */
  GtkWidget * tw_widget;
  int         line_y, i, j;
  for (i = 0; i < TRACKLIST->num_tracks; i++)
    {
      Track * const track = TRACKLIST->tracks[i];

      /* skip tracks in the other timeline (pinned/
       * non-pinned) or invisible tracks */
      if (
        !track_get_should_be_visible (track)
        || (!self->is_pinned && track_is_pinned (track))
        || (self->is_pinned && !track_is_pinned (track)))
        continue;

      /* draw line below track */
      TrackWidget * tw = track->widget;
      if (!GTK_IS_WIDGET (tw))
        continue;
      tw_widget = (GtkWidget *) tw;

      double full_track_height =
        track_get_full_visible_height (track);

      double track_start_offset;
      gtk_widget_translate_coordinates (
        tw_widget,
        GTK_WIDGET (
          self->is_pinned
            ? MW_TRACKLIST->pinned_box
            : MW_TRACKLIST->unpinned_box),
        0, 0, NULL, &track_start_offset);

      line_y =
        (int) track_start_offset + (int) full_track_height;

      if (line_y >= rect->y && line_y < rect->y + rect->height)
        {
          gtk_snapshot_append_color (
            snapshot, &thick_grid_line_color,
            &GRAPHENE_RECT_INIT (
              (float) rect->x, (float) (line_y - 1),
              (float) rect->width, 2.f));
        }

      /* draw selection tint */
      if (track_is_selected (track))
        {
          gtk_snapshot_append_color (
            snapshot, &Z_GDK_RGBA_INIT (1, 1, 1, 0.04f),
            &GRAPHENE_RECT_INIT (
              (float) rect->x, (float) track_start_offset,
              (float) rect->width, (float) full_track_height));
        }

      double total_height = track->main_height;

#define OFFSET_PLUS_TOTAL_HEIGHT \
  (track_start_offset + total_height)

      /* --- draw lanes --- */

      if (track->lanes_visible)
        {
          for (j = 0; j < track->num_lanes; j++)
            {
              TrackLane * lane = track->lanes[j];

              /* horizontal line above lane */
              if (
                OFFSET_PLUS_TOTAL_HEIGHT > rect->y
                && OFFSET_PLUS_TOTAL_HEIGHT
                     < rect->y + rect->height)
                {
                  gtk_snapshot_append_color (
                    snapshot,
                    &Z_GDK_RGBA_INIT (0.7f, 0.7f, 0.7f, 0.4f),
                    &GRAPHENE_RECT_INIT (
                      (float) rect->x,
                      (float) OFFSET_PLUS_TOTAL_HEIGHT,
                      (float) rect->width, 1.f));
                }

              total_height += (double) lane->height;
            }
        }

      /* --- draw automation --- */

      /* skip tracks without visible automation */
      if (!track->automation_visible)
        continue;

      AutomationTracklist * atl =
        track_get_automation_tracklist (track);
      if (atl)
        {
          AutomationTrack * at;
          for (j = 0; j < atl->num_ats; j++)
            {
              at = atl->ats[j];

              if (!at->created || !at->visible)
                continue;

              /* horizontal line above automation
               * track */
              if (
                OFFSET_PLUS_TOTAL_HEIGHT > rect->y
                && OFFSET_PLUS_TOTAL_HEIGHT
                     < rect->y + rect->height)
                {
                  gtk_snapshot_append_color (
                    snapshot,
                    &Z_GDK_RGBA_INIT (0.7f, 0.7f, 0.7f, 0.2f),
                    &GRAPHENE_RECT_INIT (
                      (float) rect->x,
                      (float) OFFSET_PLUS_TOTAL_HEIGHT,
                      (float) rect->width, 1.f));
                }

              float normalized_val =
                automation_track_get_val_at_pos (
                  at, PLAYHEAD, true, true);
              Port * port =
                port_find_from_identifier (&at->port_id);
              AutomationPoint * ap =
                automation_track_get_ap_before_pos (
                  at, PLAYHEAD, true);
              if (!ap)
                {
                  normalized_val =
                    control_port_real_val_to_normalized (
                      port, control_port_get_val (port));
                }

              int y_px =
                automation_track_get_y_px_from_normalized_val (
                  at, normalized_val);

              /* line at current val */
              gtk_snapshot_append_color (
                snapshot,
                &Z_GDK_RGBA_INIT (
                  track->color.red, track->color.green,
                  track->color.blue, 0.3f),
                &GRAPHENE_RECT_INIT (
                  (float) rect->x,
                  (float) (OFFSET_PLUS_TOTAL_HEIGHT + y_px),
                  (float) rect->width, 1.f));

              total_height += (double) at->height;
            }
        }
    }
}

static void
draw_midi_bg (
  ArrangerWidget * self,
  GtkSnapshot *    snapshot,
  GdkRectangle *   rect)
{
  /* px per key adjusted for border width */
  double adj_px_per_key = MW_PIANO_ROLL_KEYS->px_per_key + 1.0;
  /*double adj_total_key_px =*/
  /*MW_PIANO_ROLL->total_key_px + 126;*/

  /*handle horizontal drawing*/
  double y_offset;
  for (int i = 0; i < 128; i++)
    {
      y_offset = adj_px_per_key * i;
      /* if key is visible */
      if (
        y_offset > rect->y
        && y_offset < (rect->y + rect->height))
        {
          gtk_snapshot_append_color (
            snapshot,
            &Z_GDK_RGBA_INIT (0.7f, 0.7f, 0.7f, 0.5f),
            &GRAPHENE_RECT_INIT (
              (float) rect->x, (float) y_offset,
              (float) rect->width, 1.f));
          if (piano_roll_is_key_black (
                PIANO_ROLL->piano_descriptors[i]->value))
            {
              gtk_snapshot_append_color (
                snapshot, &Z_GDK_RGBA_INIT (0, 0, 0, 0.2f),
                &GRAPHENE_RECT_INIT (
                  (float) rect->x,
                  /* + 1 since the border is
                   * bottom */
                  (float) (y_offset + 1), (float) rect->width,
                  (float) adj_px_per_key));
            }
        }
      bool drum_mode =
        arranger_widget_get_drum_mode_enabled (self);
      if ((drum_mode
           && PIANO_ROLL->drum_descriptors[i]->value
           == MW_MIDI_ARRANGER->hovered_note)
          ||
          (!drum_mode
           && PIANO_ROLL->piano_descriptors[i]->value
           == MW_MIDI_ARRANGER->hovered_note))
        {
          gtk_snapshot_append_color (
            snapshot, &Z_GDK_RGBA_INIT (1, 1, 1, 0.06f),
            &GRAPHENE_RECT_INIT (
              (float) rect->x,
              /* + 1 since the border is
               * bottom */
              (float) (y_offset + 1), (float) rect->width,
              (float) adj_px_per_key));
        }
    }
}

static void
draw_velocity_bg (
  ArrangerWidget * self,
  GtkSnapshot *    snapshot,
  GdkRectangle *   rect)
{
  for (int i = 1; i < 4; i++)
    {
      float y_offset =
        (float) rect->height * ((float) i / 4.f);
      gtk_snapshot_append_color (
        snapshot, &Z_GDK_RGBA_INIT (1, 1, 1, 0.2f),
        &GRAPHENE_RECT_INIT (
          (float) rect->x, (float) y_offset,
          (float) rect->width, 1.f));
    }
}

static void
draw_audio_bg (
  ArrangerWidget * self,
  GtkSnapshot *    snapshot,
  GdkRectangle *   rect)
{
  ZRegion * ar = clip_editor_get_region (CLIP_EDITOR);
  if (!ar)
    {
      g_message ("audio region not found, skipping draw");
      return;
    }
  if (ar->stretching)
    {
      return;
    }
  ArrangerObject * obj = (ArrangerObject *) ar;
  TrackLane *      lane = region_get_lane (ar);
  Track *          track = track_lane_get_track (lane);
  g_return_if_fail (lane);

  AudioClip * clip = AUDIO_POOL->clips[ar->pool_id];

  double local_start_x = (double) rect->x;
  double local_end_x = local_start_x + (double) rect->width;

  /* frames in the clip to start drawing from */
  signed_frame_t prev_frames = MAX (
    ui_px_to_frames_editor (local_start_x, 1) - obj->pos.frames,
    0);

  UiDetail detail = ui_get_detail_level ();
  double   increment = 1;
  double   width = 1;

  switch (detail)
    {
    case UI_DETAIL_HIGH:
      increment = 0.5;
      width = 1;
      break;
    case UI_DETAIL_NORMAL:
      increment = 1;
      width = 1;
      break;
    case UI_DETAIL_LOW:
      increment = 2;
      width = 2;
      break;
    case UI_DETAIL_ULTRA_LOW:
      increment = 4;
      width = 4;
      break;
    }

  /* draw fades */
  signed_frame_t obj_length_frames =
    arranger_object_get_length_in_frames (obj);
  GdkRGBA base_color = {
    .red = 0.3f, .green = 0.3f, .blue = 0.3f, .alpha = 0.f
  };
  GdkRGBA fade_color;
  color_morph (&base_color, &track->color, 0.5, &fade_color);
  fade_color.alpha = 0.5f;
  for (double i = local_start_x; i < local_end_x;
       i += increment)
    {
      signed_frame_t curr_frames =
        ui_px_to_frames_editor (i, 1) - obj->pos.frames;
      if (curr_frames < 0 || curr_frames >= obj_length_frames)
        continue;

      double max;
      if (curr_frames < obj->fade_in_pos.frames)
        {
          z_return_if_fail_cmp (obj->fade_in_pos.frames, >, 0);
          max = fade_get_y_normalized (
            (double) curr_frames
              / (double) obj->fade_in_pos.frames,
            &obj->fade_in_opts, 1);
        }
      else if (curr_frames >= obj->fade_out_pos.frames)
        {
          z_return_if_fail_cmp (
            obj->end_pos.frames
              - (obj->fade_out_pos.frames + obj->pos.frames),
            >, 0);
          max =
            fade_get_y_normalized (
              (double)
              (curr_frames -
               obj->fade_out_pos.frames) /
              (double)
              (obj->end_pos.frames -
                (obj->fade_out_pos.frames +
                 obj->pos.frames)),
              &obj->fade_out_opts, 0);
        }
      else
        continue;

      /* invert because gtk draws the other
       * way around */
      max = 1.0 - max;

      double from_y = -rect->y;
      double draw_height =
        (MIN (
           (double) max * (double) rect->height,
           (double) rect->height)
         - rect->y)
        - from_y;

      gtk_snapshot_append_color (
        snapshot, &fade_color,
        &GRAPHENE_RECT_INIT (
          (float) i, (float) from_y, (float) width,
          (float) draw_height));

      if (curr_frames >= (signed_frame_t) clip->num_frames)
        break;
    }

  /* draw audio part */
  GdkRGBA * color = &track->color;
  GdkRGBA   audio_lines_color = {
      color->red + 0.3f, color->green + 0.3f,
      color->blue + 0.3f, 0.9f
  };
  for (double i = local_start_x; i < local_end_x;
       i += increment)
    {
      signed_frame_t curr_frames =
        ui_px_to_frames_editor (i, 1) - obj->pos.frames;
      if (curr_frames < 0)
        continue;

      float min = 0.f, max = 0.f;
      for (signed_frame_t j = prev_frames; j < curr_frames; j++)
        {
          if (j >= (signed_frame_t) clip->num_frames)
            break;
          for (unsigned int k = 0; k < clip->channels; k++)
            {
              signed_frame_t index =
                j * (signed_frame_t) clip->channels
                + (signed_frame_t) k;
              g_return_if_fail (
                index >= 0 &&
                index <
                  (signed_frame_t)
                  (clip->num_frames *
                     clip->channels));
              float val = clip->frames[index];
              if (val > max)
                {
                  max = val;
                }
              if (val < min)
                {
                  min = val;
                }
            }
        }
#define DRAW_VLINE(x, from_y, _height) \
  gtk_snapshot_append_color ( \
    snapshot, &audio_lines_color, \
    &GRAPHENE_RECT_INIT ( \
      (float) (x), (float) (from_y), (float) width, \
      (float) (_height)))

      min = (min + 1.f) / 2.f; /* normallize */
      max = (max + 1.f) / 2.f; /* normalize */
      double from_y =
        MAX ((double) min * (double) rect->height, 0.0)
        - rect->y;
      double draw_height =
        (MIN (
           (double) max * (double) rect->height,
           (double) rect->height)
         - rect->y)
        - from_y;
      DRAW_VLINE (
        /* x */
        i,
        /* from y */
        from_y,
        /* to y */
        draw_height);

      if (curr_frames >= (signed_frame_t) clip->num_frames)
        break;

      prev_frames = curr_frames;
    }
#undef DRAW_VLINE

  /* draw gain line */
  float gain_fader_val =
    math_get_fader_val_from_amp (ar->gain);
  int gain_line_start_x =
    ui_pos_to_px_editor (&obj->pos, F_PADDING);
  int gain_line_end_x =
    ui_pos_to_px_editor (&obj->end_pos, F_PADDING);
  gtk_snapshot_append_color (
    snapshot, &UI_COLORS->bright_orange,
    &GRAPHENE_RECT_INIT (
      /* need 1 pixel extra for some reason */
      1.f + (float) gain_line_start_x,
      /* invert because gtk draws the opposite
       * way */
      (float) rect->height * (1.f - gain_fader_val),
      (float) (gain_line_end_x - gain_line_start_x), 2.f));

  /* draw gain text */
  double gain_db = math_amp_to_dbfs (ar->gain);
  char   gain_txt[50];
  sprintf (gain_txt, "%.1fdB", gain_db);
  int gain_txt_padding = 3;
  pango_layout_set_markup (self->audio_layout, gain_txt, -1);
  GtkStyleContext * style_ctx =
    gtk_widget_get_style_context (GTK_WIDGET (self));
  gtk_snapshot_render_layout (
    snapshot, style_ctx, gain_txt_padding + gain_line_start_x,
    gain_txt_padding
      + (int) ((double) rect->height * (1.0 - gain_fader_val)),
    self->audio_layout);
}

static void
draw_automation_bg (
  ArrangerWidget * self,
  GtkSnapshot *    snapshot,
  const int        width,
  const int        height,
  GdkRectangle *   rect)
{
  /* draws horizontal lines at the cut-off points
   * where automation can be drawn */
#if 0
  gtk_snapshot_append_color (
    snapshot, &thick_grid_line_color,
    &GRAPHENE_RECT_INIT (
      0, (float) AUTOMATION_ARRANGER_VPADDING,
      width, 1));
  gtk_snapshot_append_color (
    snapshot, &thick_grid_line_color,
    &GRAPHENE_RECT_INIT (
      0, (float) (height - AUTOMATION_ARRANGER_VPADDING),
      width, 1));
#endif
}

static void
draw_vertical_lines (
  ArrangerWidget * self,
  RulerWidget *    ruler,
  GtkSnapshot *    snapshot,
  GdkRectangle *   rect)
{
  const GdkRGBA thick_color = thick_grid_line_color;
  const GdkRGBA thinner_color = { 0.25f, 0.25f, 0.25f, 0.5f };
  const GdkRGBA thinnest_color = { 0.2f, 0.2f, 0.2f, 0.5f };

  const int thick_width = 1;
  const int thinner_width = 1;
  const int thinnest_width = 1;

  int height = rect->height;

  /* if time display */
  if (self->ruler_display == TRANSPORT_DISPLAY_TIME)
    {
      /* get sec interval */
      int sec_interval = ruler_widget_get_sec_interval (ruler);

      /* get 10 sec interval */
      int ten_sec_interval =
        ruler_widget_get_10sec_interval (ruler);

      /* get the interval for mins */
      int min_interval = (int) MAX (
        (RW_PX_TO_HIDE_BEATS) / (double) ruler->px_per_min,
        1.0);

      int    i = 0;
      double curr_px;
      while (
        (curr_px =
           ruler->px_per_min * (i += min_interval)
           + SPACE_BEFORE_START)
        < rect->x + rect->width)
        {
          if (curr_px < rect->x)
            continue;

          gtk_snapshot_append_color (
            snapshot, &thick_color,
            &GRAPHENE_RECT_INIT (
              (float) curr_px, (float) rect->y,
              (float) thick_width, (float) height));
        }
      i = 0;
      if (ten_sec_interval > 0)
        {
          while (
            (curr_px =
               ruler->px_per_10sec * (i += ten_sec_interval)
               + SPACE_BEFORE_START)
            < rect->x + rect->width)
            {
              if (curr_px < rect->x)
                continue;

              gtk_snapshot_append_color (
                snapshot, &thinner_color,
                &GRAPHENE_RECT_INIT (
                  (float) curr_px, (float) rect->y,
                  (float) thinner_width, (float) height));
            }
        }
      i = 0;
      if (sec_interval > 0)
        {
          while (
            (curr_px =
               ruler->px_per_sec * (i += sec_interval)
               + SPACE_BEFORE_START)
            < rect->x + rect->width)
            {
              if (curr_px < rect->x)
                continue;

              gtk_snapshot_append_color (
                snapshot, &thinnest_color,
                &GRAPHENE_RECT_INIT (
                  (float) curr_px, (float) rect->y,
                  (float) thinnest_width, (float) height));
            }
        }
    }
  /* else if BBT display */
  else
    {
      /* get sixteenth interval */
      int sixteenth_interval =
        ruler_widget_get_sixteenth_interval (ruler);

      /* get the beat interval */
      int beat_interval =
        ruler_widget_get_beat_interval (ruler);

      /* get the interval for bars */
      int bar_interval = (int) MAX (
        (RW_PX_TO_HIDE_BEATS) / (double) ruler->px_per_bar,
        1.0);

      int    i = 0;
      double curr_px;
      while (
        (curr_px =
           ruler->px_per_bar * (i += bar_interval)
           + SPACE_BEFORE_START)
        < rect->x + rect->width)
        {
          if (curr_px < rect->x)
            continue;

          gtk_snapshot_append_color (
            snapshot, &thick_color,
            &GRAPHENE_RECT_INIT (
              (float) curr_px, (float) rect->y,
              (float) thick_width, (float) height));
        }
      i = 0;
      if (beat_interval > 0)
        {
          while (
            (curr_px =
               ruler->px_per_beat * (i += beat_interval)
               + SPACE_BEFORE_START)
            < rect->x + rect->width)
            {
              if (curr_px < rect->x)
                continue;

              gtk_snapshot_append_color (
                snapshot, &thinner_color,
                &GRAPHENE_RECT_INIT (
                  (float) curr_px, (float) rect->y,
                  (float) thinner_width, (float) height));
            }
        }
      i = 0;
      if (sixteenth_interval > 0)
        {
          while (
            (curr_px =
               ruler->px_per_sixteenth
                 * (i += sixteenth_interval)
               + SPACE_BEFORE_START)
            < rect->x + rect->width)
            {
              if (curr_px < rect->x)
                continue;

              gtk_snapshot_append_color (
                snapshot, &thinnest_color,
                &GRAPHENE_RECT_INIT (
                  (float) curr_px, (float) rect->y,
                  (float) thinnest_width, (float) height));
            }
        }
    }
}

static void
draw_range (
  ArrangerWidget * self,
  GtkSnapshot *    snapshot,
  int              range_first_px,
  int              range_second_px,
  GdkRectangle *   rect)
{
  /* draw range */
  gtk_snapshot_append_color (
    snapshot, &Z_GDK_RGBA_INIT (0.3f, 0.3f, 0.3f, 0.3f),
    &GRAPHENE_RECT_INIT (
      (float) MAX (0, range_first_px), (float) rect->y,
      (float) (range_second_px - range_first_px),
      (float) rect->height));

  /* draw start and end lines */
  const float   line_width = 2.f;
  const GdkRGBA color = { 0.8f, 0.8f, 0.8f, 0.4f };
  gtk_snapshot_append_color (
    snapshot, &color,
    &GRAPHENE_RECT_INIT (
      (float) range_first_px + 1.f, (float) rect->y,
      line_width, (float) rect->height));
  gtk_snapshot_append_color (
    snapshot, &color,
    &GRAPHENE_RECT_INIT (
      (float) range_second_px + 1.f, (float) rect->y,
      line_width, (float) rect->height));
}

void
arranger_snapshot (GtkWidget * widget, GtkSnapshot * snapshot)
{
  ArrangerWidget * self = Z_ARRANGER_WIDGET (widget);

  GdkRectangle visible_rect_gdk;
  arranger_widget_get_visible_rect (self, &visible_rect_gdk);
  graphene_rect_t visible_rect;
  z_gdk_rectangle_to_graphene_rect_t (
    &visible_rect, &visible_rect_gdk);

  if (self->first_draw)
    {
      self->first_draw = false;
    }

  gint64 start_time = g_get_monotonic_time ();

  int width =
    gtk_widget_get_allocated_width (GTK_WIDGET (self));
  int height =
    gtk_widget_get_allocated_height (GTK_WIDGET (self));

  RulerWidget * ruler = arranger_widget_get_ruler (self);
  if (ruler->px_per_bar < 2.0)
    return;

  /* skip drawing if rectangle too large */
  if (
    visible_rect.size.width > 10000
    || visible_rect.size.height > 10000)
    {
      g_warning ("skipping draw - rectangle too large");
      return;
    }

  /*g_message (*/
  /*"redrawing arranger in rect: "*/
  /*"(%d, %d) width: %d height %d)",*/
  /*rect.x, rect.y, rect.width, rect.height);*/
  self->last_rect = visible_rect;

  GtkStyleContext * context =
    gtk_widget_get_style_context (widget);

#if 0
  cairo_antialias_t antialias =
    cairo_get_antialias (self->cached_cr);
  double tolerance =
    cairo_get_tolerance (self->cached_cr);
  cairo_set_antialias (
    self->cached_cr, CAIRO_ANTIALIAS_FAST);
  cairo_set_tolerance (self->cached_cr, 1.5);
#endif

  gtk_snapshot_render_background (
    snapshot, context, 0, 0, width, height);

  /* pretend we're drawing from 0, 0 */
  gtk_snapshot_save (snapshot);
  gtk_snapshot_translate (
    snapshot,
    &GRAPHENE_POINT_INIT (
      -visible_rect.origin.x, -visible_rect.origin.y));

  /* draw loop background */
  if (TRANSPORT->loop)
    {
      double start_px = 0, end_px = 0;
      if (self->type == TYPE (TIMELINE))
        {
          start_px = ui_pos_to_px_timeline (
            &TRANSPORT->loop_start_pos, 1);
          end_px = ui_pos_to_px_timeline (
            &TRANSPORT->loop_end_pos, 1);
        }
      else
        {
          start_px = ui_pos_to_px_editor (
            &TRANSPORT->loop_start_pos, 1);
          end_px =
            ui_pos_to_px_editor (&TRANSPORT->loop_end_pos, 1);
        }
      GdkRGBA loop_color = { 0, 0.9f, 0.7f, 0.08f };

      /* draw the loop start line */
      gtk_snapshot_append_color (
        snapshot, &loop_color,
        &GRAPHENE_RECT_INIT (
          (float) start_px + 1.f, 0.f, 2.f, (float) height));

      /* draw loop end line */
      gtk_snapshot_append_color (
        snapshot, &loop_color,
        &GRAPHENE_RECT_INIT (
          (float) end_px + 1.f, 0.f, 2.f, (float) height));

      /* draw transport loop area */
      double loop_start_x = MAX (0, start_px);
      gtk_snapshot_append_color (
        snapshot, &Z_GDK_RGBA_INIT (0, 0.9f, 0.7f, 0.02f),
        &GRAPHENE_RECT_INIT (
          (float) loop_start_x, 0.f,
          (float) (end_px - start_px), (float) height));
    }

  /* --- handle vertical drawing --- */

  draw_vertical_lines (
    self, ruler, snapshot, &visible_rect_gdk);

  /* draw range */
  int  range_first_px, range_second_px;
  bool have_range = false;
  if (self->type == TYPE (AUDIO) && AUDIO_SELECTIONS->has_selection)
    {
      Position *range_first_pos, *range_second_pos;
      if (position_is_before_or_equal (
            &TRANSPORT->range_1, &TRANSPORT->range_2))
        {
          range_first_pos = &AUDIO_SELECTIONS->sel_start;
          range_second_pos = &AUDIO_SELECTIONS->sel_end;
        }
      else
        {
          range_first_pos = &AUDIO_SELECTIONS->sel_end;
          range_second_pos = &AUDIO_SELECTIONS->sel_start;
        }

      range_first_px =
        ui_pos_to_px_editor (range_first_pos, 1);
      range_second_px =
        ui_pos_to_px_editor (range_second_pos, 1);
      have_range = true;
    }
  else if (self->type == TYPE (TIMELINE) && TRANSPORT->has_range)
    {
      /* in order they appear */
      Position *range_first_pos, *range_second_pos;
      if (position_is_before_or_equal (
            &TRANSPORT->range_1, &TRANSPORT->range_2))
        {
          range_first_pos = &TRANSPORT->range_1;
          range_second_pos = &TRANSPORT->range_2;
        }
      else
        {
          range_first_pos = &TRANSPORT->range_2;
          range_second_pos = &TRANSPORT->range_1;
        }

      range_first_px =
        ui_pos_to_px_timeline (range_first_pos, 1);
      range_second_px =
        ui_pos_to_px_timeline (range_second_pos, 1);
      have_range = true;
    }

  if (have_range)
    {
      draw_range (
        self, snapshot, range_first_px, range_second_px,
        &visible_rect_gdk);
    }

  if (self->type == TYPE (TIMELINE))
    {
      draw_timeline_bg (self, snapshot, &visible_rect_gdk);
    }
  else if (self->type == TYPE (MIDI))
    {
      draw_midi_bg (self, snapshot, &visible_rect_gdk);
    }
  else if (self->type == TYPE (MIDI_MODIFIER))
    {
      draw_velocity_bg (self, snapshot, &visible_rect_gdk);
    }
  else if (self->type == TYPE (AUDIO))
    {
      draw_audio_bg (self, snapshot, &visible_rect_gdk);
    }
  else if (self->type == TYPE (AUTOMATION))
    {
      draw_automation_bg (
        self, snapshot, width, height, &visible_rect_gdk);
    }

  /* draw each arranger object */
  if (!self->hit_objs_to_draw)
    {
      self->hit_objs_to_draw =
        g_ptr_array_new_full (200, NULL);
    }
  g_ptr_array_remove_range (
    self->hit_objs_to_draw, 0, self->hit_objs_to_draw->len);
  arranger_widget_get_hit_objects_in_rect (
    self, ARRANGER_OBJECT_TYPE_ALL, &visible_rect_gdk,
    self->hit_objs_to_draw);

  /*g_message (*/
  /*"objects found: %d (is pinned %d)",*/
  /*num_objs, self->is_pinned);*/
  /* note: these are only project objects */
  for (size_t j = 0; j < self->hit_objs_to_draw->len; j++)
    {
      ArrangerObject * obj = (ArrangerObject *)
        g_ptr_array_index (self->hit_objs_to_draw, j);
      draw_arranger_object (
        self, obj, snapshot, &visible_rect_gdk);
    }

  /* draw dnd highlight */
  draw_highlight (self, snapshot, &visible_rect_gdk);

  /* draw selections */
  draw_selections (self, snapshot);

  draw_playhead (self, snapshot, &visible_rect_gdk);

#if 0
  cairo_set_antialias (
    self->cached_cr, antialias);
  cairo_set_tolerance (self->cached_cr, tolerance);

  cairo_set_source_surface (
    cr, self->cached_surface,
    visible_rect_gdk.x, visible_rect_gdk.y);
  cairo_paint (cr);
#endif

  if (DEBUGGING)
    {
      draw_debug_text (self, snapshot, &visible_rect_gdk);
    }

  gtk_snapshot_restore (snapshot);

  gint64 end_time = g_get_monotonic_time ();

  (void) start_time;
  (void) end_time;
#if 0
  g_debug ("finished drawing in %ld microseconds, "
    "rect x:%d y:%d w:%d h:%d for %s "
    "arranger",
    end_time - start_time,
    rect.x, rect.y, rect.width, rect.height,
    arranger_widget_get_type_str (self));
#endif
}
