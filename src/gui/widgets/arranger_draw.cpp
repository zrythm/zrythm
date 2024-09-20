// SPDX-FileCopyrightText: © 2018-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/arranger_object.h"
#include "dsp/audio_region.h"
#include "dsp/automatable_track.h"
#include "dsp/laned_track.h"
#include "dsp/track_lane.h"
#include "dsp/tracklist.h"
#include "gui/widgets/arranger_draw.h"
#include "gui/widgets/arranger_object.h"
#include "gui/widgets/arranger_wrapper.h"
#include "gui/widgets/automation_arranger.h"
#include "gui/widgets/bot_bar.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/clip_editor.h"
#include "gui/widgets/clip_editor_inner.h"
#include "gui/widgets/main_notebook.h"
#include "gui/widgets/midi_arranger.h"
#include "gui/widgets/midi_editor_space.h"
#include "gui/widgets/midi_modifier_arranger.h"
#include "gui/widgets/piano_roll_keys.h"
#include "gui/widgets/ruler.h"
#include "gui/widgets/timeline_arranger.h"
#include "gui/widgets/timeline_panel.h"
#include "gui/widgets/track.h"
#include "gui/widgets/tracklist.h"
#include "project.h"
#include "utils/color.h"
#include "utils/debug.h"
#include "utils/dsp.h"
#include "utils/flags.h"
#include "utils/gtk.h"
#include "utils/math.h"
#include "utils/objects.h"
#include "zrythm_app.h"

/*#include <valgrind/callgrind.h>*/

#define TYPE(x) ArrangerWidgetType::x

static const GdkRGBA thick_grid_line_color = { 0.3f, 0.3f, 0.3f, 0.9f };

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
  graphene_rect_t graphene_rect = Z_GRAPHENE_RECT_INIT (
    (float) self->start_x, (float) self->start_y, (float) (offset_x),
    (float) (offset_y));
  gsk_rounded_rect_init_from_rect (&rounded_rect, &graphene_rect, 0);
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
    case UiOverlayAction::SELECTING:
    case UiOverlayAction::DELETE_SELECTING:
      {
        gtk_snapshot_append_color (snapshot, &inside_color, &graphene_rect);
        gtk_snapshot_append_border (
          snapshot, &rounded_rect, border_widths, border_colors);
      }
      break;
    case UiOverlayAction::RAMPING:
      {
        cairo_t * cr = gtk_snapshot_append_cairo (snapshot, &graphene_rect);
        gdk_cairo_set_source_rgba (cr, &border_color);
        cairo_set_line_width (cr, 2.0);
        cairo_move_to (cr, self->start_x, self->start_y);
        cairo_line_to (cr, self->start_x + offset_x, self->start_y + offset_y);
        cairo_stroke (cr);
        cairo_destroy (cr);
      }
      break;
    default:
      break;
    }
}

static void
draw_highlight (ArrangerWidget * self, GtkSnapshot * snapshot, GdkRectangle * rect)
{
  if (!self->is_highlighted)
    {
      return;
    }

  /* this wasn't tested since porting to gtk4 */
  graphene_rect_t tmp = Z_GRAPHENE_RECT_INIT (
    (float) ((self->highlight_rect.x + 1)),
    (float) ((self->highlight_rect.y + 1)),
    (float) (self->highlight_rect.width - 1),
    (float) (self->highlight_rect.height - 1));
  auto color = UI_COLORS->bright_orange.to_gdk_rgba ();
  gtk_snapshot_append_color (snapshot, &color, &tmp);
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
    < 1 + (arranger_object_should_orig_be_visible (*obj, self) && obj->is_selected ());
    i++)
    {
      /* if looping 2nd time (transient) */
      if (i == 1)
        {
          z_return_if_fail (obj->transient_);
          obj = obj->get_transient ().get ();
        }

      arranger_object_set_full_rectangle (obj, self);

      /* only draw if the object's rectangle is hit by the drawable region (for
       * regions, the logic is handled inside region_draw() so the check is
       * skipped) */
      bool rect_hit_or_region =
        ui_rectangle_overlap (&obj->full_rect_, rect) || obj->is_region ();

      Track * track = obj->get_track ();
      bool    should_be_visible =
        (track->visible_ && self->is_pinned == track->is_pinned ())
        || self->type != ArrangerWidgetType::Timeline;

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
draw_playhead (ArrangerWidget * self, GtkSnapshot * snapshot, GdkRectangle * rect)
{
  int cur_playhead_px = arranger_widget_get_playhead_px (self);
  int px = cur_playhead_px;
  /*int px = self->queued_playhead_px;*/
  /*z_info ("drawing {}", px);*/

  int height = rect->height;

  if (px >= rect->x && px <= rect->x + rect->width)
    {
      graphene_rect_t tmp = Z_GRAPHENE_RECT_INIT (
        (float) px - 1.f, (float) rect->y, 2.f, (float) height);
      auto color = UI_COLORS->prefader_send.to_gdk_rgba ();
      gtk_snapshot_append_color (snapshot, &color, &tmp);
      self->last_playhead_px = px;
    }

  /* draw faded playhead if in audition mode */
  if (P_TOOL == Tool::Audition)
    {
      bool   hovered = false;
      double hover_x = 0.f;
      switch (self->type)
        {
        case ArrangerWidgetType::Timeline:
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
        case ArrangerWidgetType::Midi:
        case ArrangerWidgetType::MidiModifier:
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
          GdkRGBA color = UI_COLORS->prefader_send.to_gdk_rgba ();
          color.alpha = 0.6f;
          graphene_rect_t tmp = Z_GRAPHENE_RECT_INIT (
            (float) hover_x - 1, (float) rect->y, 2.f, (float) height);
          gtk_snapshot_append_color (snapshot, &color, &tmp);
        }
    }
}

static void
draw_debug_text (
  ArrangerWidget * self,
  GtkSnapshot *    snapshot,
  GdkRectangle *   rect)
{
  graphene_rect_t tmp =
    Z_GRAPHENE_RECT_INIT ((float) rect->x, (float) rect->y, 400.f, 400.f);
  GdkRGBA color = Z_GDK_RGBA_INIT (0, 0, 0, 0.4);
  gtk_snapshot_append_color (snapshot, &color, &tmp);
  auto settings = arranger_widget_get_editor_settings (self);
  std::visit (
    [&] (auto &&st) {
      auto debug_txt = fmt::format (
        "Hover: (%.0f, %.0f)\nNormalized hover: (%.0f, %.0f)\nAction: {}",
        self->hover_x, self->hover_y, self->hover_x - st->scroll_start_x_,
        self->hover_y - st->scroll_start_y_,
        UiOverlayAction_to_string (self->action));
      pango_layout_set_markup (
        self->debug_layout.get (), debug_txt.c_str (), -1);
      gtk_snapshot_save (snapshot);
      graphene_point_t point =
        Z_GRAPHENE_POINT_INIT ((float) (rect->x + 4), (float) (rect->y + 4));
      gtk_snapshot_translate (snapshot, &point);
      GdkRGBA tmp_color = Z_GDK_RGBA_INIT (1, 1, 1, 1);
      gtk_snapshot_append_layout (
        snapshot, self->debug_layout.get (), &tmp_color);
      gtk_snapshot_restore (snapshot);
    },
    settings);
}

static void
draw_timeline_bg (
  ArrangerWidget * self,
  GtkSnapshot *    snapshot,
  GdkRectangle *   rect)
{
  /* handle horizontal drawing for tracks */
  GtkWidget * tw_widget;
  int         line_y;
  for (auto const &track : TRACKLIST->tracks_)
    {
      /* skip tracks in the other timeline (pinned/ non-pinned) or invisible
       * tracks */
      if (
        !track->should_be_visible () || (!self->is_pinned && track->is_pinned ())
        || (self->is_pinned && !track->is_pinned ()))
        continue;

      /* draw line below track */
      auto tw = track->widget_;
      if (!GTK_IS_WIDGET (tw))
        continue;
      tw_widget = (GtkWidget *) tw;

      double full_track_height = track->get_full_visible_height ();

      graphene_point_t track_start_offset_pt;
      graphene_point_t tmp = Z_GRAPHENE_POINT_INIT (0.f, 0.f);
      bool             success = gtk_widget_compute_point (
        tw_widget,
        GTK_WIDGET (
          self->is_pinned ? MW_TRACKLIST->pinned_box : MW_TRACKLIST->unpinned_box),
        &tmp, &track_start_offset_pt);
      z_return_if_fail (success);
      double track_start_offset = (double) track_start_offset_pt.y;

      line_y = (int) track_start_offset + (int) full_track_height;

      if (line_y >= rect->y && line_y < rect->y + rect->height)
        {
          graphene_rect_t tmp_rect = Z_GRAPHENE_RECT_INIT (
            (float) rect->x, (float) (line_y - 1), (float) rect->width, 2.f);
          gtk_snapshot_append_color (
            snapshot, &thick_grid_line_color, &tmp_rect);
        }

      /* draw selection tint */
      if (track->is_selected ())
        {
          graphene_rect_t tmp_rect = Z_GRAPHENE_RECT_INIT (
            (float) rect->x, (float) track_start_offset, (float) rect->width,
            (float) full_track_height);
          GdkRGBA color = Z_GDK_RGBA_INIT (1, 1, 1, 0.04f);
          gtk_snapshot_append_color (snapshot, &color, &tmp_rect);
        }

      double total_height = track->main_height_;

#define OFFSET_PLUS_TOTAL_HEIGHT (track_start_offset + total_height)

      /* --- draw lanes --- */

      if (auto laned_track = dynamic_cast<LanedTrack *> (track.get ()))
        {
          auto variant = convert_to_variant<LanedTrackPtrVariant> (laned_track);
          std::visit (
            [&] (auto &tr) {
              if (tr->lanes_visible_)
                {
                  for (auto &lane : tr->lanes_)
                    {
                      /* horizontal line above lane */
                      if (
                        OFFSET_PLUS_TOTAL_HEIGHT > rect->y
                        && OFFSET_PLUS_TOTAL_HEIGHT < rect->y + rect->height)
                        {
                          GdkRGBA color =
                            Z_GDK_RGBA_INIT (0.7f, 0.7f, 0.7f, 0.4f);
                          graphene_rect_t tmp_rect = Z_GRAPHENE_RECT_INIT (
                            (float) rect->x, (float) OFFSET_PLUS_TOTAL_HEIGHT,
                            (float) rect->width, 1.f);
                          gtk_snapshot_append_color (
                            snapshot, &color, &tmp_rect);
                        }

                      total_height += (double) lane->height_;
                    }
                }
            },
            variant);
        }

      /* --- draw automation --- */

      /* skip tracks without visible automation */
      if (
        auto automatable_track = dynamic_cast<AutomatableTrack *> (track.get ());
        automatable_track && automatable_track->automation_visible_)
        {
          auto &atl = automatable_track->get_automation_tracklist ();
          for (auto at : atl.visible_ats_)
            {
              if (!at->created_)
                continue;

              /* horizontal line above automation track */
              if (
                OFFSET_PLUS_TOTAL_HEIGHT > rect->y
                && OFFSET_PLUS_TOTAL_HEIGHT < rect->y + rect->height)
                {
                  GdkRGBA color = Z_GDK_RGBA_INIT (0.7f, 0.7f, 0.7f, 0.2f);
                  graphene_rect_t tmp_rect = Z_GRAPHENE_RECT_INIT (
                    (float) rect->x, (float) OFFSET_PLUS_TOTAL_HEIGHT,
                    (float) rect->width, 1.f);
                  gtk_snapshot_append_color (snapshot, &color, &tmp_rect);
                }

              float normalized_val =
                at->get_val_at_pos (PLAYHEAD, true, true, true);
              auto port = Port::find_from_identifier<ControlPort> (at->port_id_);
              auto ap = at->get_ap_before_pos (PLAYHEAD, true, true);
              if (!ap)
                {
                  normalized_val =
                    port->real_val_to_normalized (port->get_val ());
                }

              int y_px = at->get_y_px_from_normalized_val (normalized_val);

              /* line at current val */
              GdkRGBA color = track->color_.to_gdk_rgba_with_alpha (0.3f);
              {
                graphene_rect_t tmp_r = Z_GRAPHENE_RECT_INIT (
                  (float) rect->x, (float) (OFFSET_PLUS_TOTAL_HEIGHT + y_px),
                  (float) rect->width, 1.f);
                gtk_snapshot_append_color (snapshot, &color, &tmp_r);
              }

              total_height += (double) at->height_;
            }
        }
    }
}

static void
draw_midi_bg (ArrangerWidget * self, GtkSnapshot * snapshot, GdkRectangle * rect)
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
      if (y_offset > rect->y && y_offset < (rect->y + rect->height))
        {
          GdkRGBA color = Z_GDK_RGBA_INIT (0.7f, 0.7f, 0.7f, 0.5f);
          {
            graphene_rect_t tmp_r = Z_GRAPHENE_RECT_INIT (
              (float) rect->x, (float) y_offset, (float) rect->width, 1.f);
            gtk_snapshot_append_color (snapshot, &color, &tmp_r);
          }
          if (
            PIANO_ROLL->is_key_black (PIANO_ROLL->piano_descriptors_[i].value_))
            {
              color = Z_GDK_RGBA_INIT (0, 0, 0, 0.2f);
              /* rect  y: + 1 since the border is bottom */
              graphene_rect_t tmp_r = Z_GRAPHENE_RECT_INIT (
                (float) rect->x, (float) (y_offset + 1), (float) rect->width,
                (float) adj_px_per_key);
              gtk_snapshot_append_color (snapshot, &color, &tmp_r);
            }
        }
      bool drum_mode = arranger_widget_get_drum_mode_enabled (self);
      if ((drum_mode && PIANO_ROLL->drum_descriptors_[i].value_ == MW_MIDI_ARRANGER->hovered_note) || (!drum_mode && PIANO_ROLL->piano_descriptors_[i].value_ == MW_MIDI_ARRANGER->hovered_note))
        {
          GdkRGBA tmp_color = Z_GDK_RGBA_INIT (1, 1, 1, 0.06f);
          /* rect  y: + 1 since the border is bottom */
          graphene_rect_t rect_r = Z_GRAPHENE_RECT_INIT (
            (float) rect->x, (float) (y_offset + 1), (float) rect->width,
            (float) adj_px_per_key);
          gtk_snapshot_append_color (snapshot, &tmp_color, &rect_r);
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
      float   y_offset = (float) rect->height * ((float) i / 4.f);
      GdkRGBA color = Z_GDK_RGBA_INIT (1, 1, 1, 0.2f);
      {
        graphene_rect_t tmp_r = Z_GRAPHENE_RECT_INIT (
          (float) rect->x, (float) y_offset, (float) rect->width, 1.f);
        gtk_snapshot_append_color (snapshot, &color, &tmp_r);
      }
    }
}

static void
draw_audio_bg (ArrangerWidget * self, GtkSnapshot * snapshot, GdkRectangle * rect)
{
  auto ar = CLIP_EDITOR->get_region<AudioRegion> ();
  if (!ar)
    {
      z_debug ("audio region not found, skipping draw");
      return;
    }
  if (ar->stretching_)
    {
      return;
    }
  auto obj = ar;
  auto lane = ar->get_lane ();
  z_return_if_fail (lane);
  auto track = lane->get_track ();

  auto &clip = AUDIO_POOL->clips_[ar->pool_id_];

  double local_start_x = (double) rect->x;
  double local_end_x = local_start_x + (double) rect->width;

  /* frames in the clip to start drawing from */
  signed_frame_t prev_frames =
    MAX (ui_px_to_frames_editor (local_start_x, 1) - obj->pos_.frames_, 0);

  UiDetail detail = ui_get_detail_level ();
  double   increment = 1;
  double   width = 1;

  switch (detail)
    {
    case UiDetail::High:
      /* snapshot does not work with midpoints */
      /*increment = 0.5;*/
      increment = 1;
      width = 1;
      break;
    case UiDetail::Normal:
      increment = 1;
      width = 1;
      break;
    case UiDetail::Low:
      increment = 2;
      width = 2;
      break;
    case UiDetail::UltraLow:
      increment = 4;
      width = 4;
      break;
    }

  /* draw fades */
  /* TODO draw top line like in the timeline */
  signed_frame_t obj_length_frames = obj->get_length_in_frames ();
  Color          base_color{ 0.3f, 0.3f, 0.3f, 0.f };
  GdkRGBA fade_color = base_color.morph (track->color_, 0.5f).to_gdk_rgba ();
  fade_color.alpha = 0.5f;
  for (double i = local_start_x; i < local_end_x; i += increment)
    {
      signed_frame_t curr_frames =
        ui_px_to_frames_editor (i, true) - obj->pos_.frames_;
      if (curr_frames < 0 || curr_frames >= obj_length_frames)
        continue;

      double max;
      if (curr_frames < obj->fade_in_pos_.frames_)
        {
          z_return_if_fail_cmp (obj->fade_in_pos_.frames_, >, 0);
          max = fade_get_y_normalized (
            obj->fade_in_opts_,
            (double) curr_frames / (double) obj->fade_in_pos_.frames_, true);
        }
      else if (curr_frames >= obj->fade_out_pos_.frames_)
        {
          z_return_if_fail_cmp (
            obj->end_pos_.frames_
              - (obj->fade_out_pos_.frames_ + obj->pos_.frames_),
            >, 0);
          max = fade_get_y_normalized (
            obj->fade_out_opts_,
            (double) (curr_frames - obj->fade_out_pos_.frames_)
              / (double) (obj->end_pos_.frames_
                          - (obj->fade_out_pos_.frames_ + obj->pos_.frames_)),
            false);
        }
      else
        continue;

      /* invert because gtk draws the other
       * way around */
      max = 1.0 - max;

      double from_y = -rect->y;
      double draw_height =
        (MIN ((double) max * (double) rect->height, (double) rect->height)
         - rect->y)
        - from_y;

      {
        graphene_rect_t tmp_r = Z_GRAPHENE_RECT_INIT (
          (float) i, (float) from_y, (float) width, (float) draw_height);
        gtk_snapshot_append_color (snapshot, &fade_color, &tmp_r);
      }

      if (curr_frames >= obj_length_frames)
        break;
    }

  /* draw audio part */
  GdkRGBA color = track->color_.to_gdk_rgba ();
  GdkRGBA audio_lines_color = {
    color.red + 0.3f, color.green + 0.3f, color.blue + 0.3f, 0.9f
  };
  auto    num_channels = clip->channels_;
  float * prev_min = object_new_n (num_channels, float);
  float * prev_max = object_new_n (num_channels, float);
  dsp_fill (prev_min, 0.5f, num_channels);
  dsp_fill (prev_max, 0.5f, num_channels);
  float * ch_min = object_new_n (num_channels, float);
  float * ch_max = object_new_n (num_channels, float);
  dsp_fill (ch_min, 0.5f, num_channels);
  dsp_fill (ch_max, 0.5f, num_channels);
  const double height_per_ch = (double) rect->height / (double) num_channels;
  for (double i = local_start_x; i < local_end_x; i += increment)
    {
      signed_frame_t curr_frames =
        ui_px_to_frames_editor (i, 1) - obj->pos_.frames_;
      if (curr_frames <= 0)
        continue;

      if (prev_frames >= (signed_frame_t) clip->num_frames_)
        {
          prev_frames = curr_frames;
          continue;
        }

      size_t from = (size_t) (MAX (0, prev_frames));
      size_t to =
        (size_t) (MIN (clip->num_frames_, (unsigned_frame_t) curr_frames));
      size_t frames_to_check = to - from;
      if (from + frames_to_check > clip->num_frames_)
        frames_to_check = clip->num_frames_ - from;
      z_return_if_fail_cmp (from, <, clip->num_frames_);
      z_return_if_fail_cmp (from + frames_to_check, <=, clip->num_frames_);
      if (frames_to_check > 0)
        {
          for (unsigned int k = 0; k < clip->channels_; k++)
            {
              ch_min[k] = dsp_min (
                clip->ch_frames_.getReadPointer (k, from),
                (size_t) frames_to_check);
              ch_max[k] = dsp_max (
                clip->ch_frames_.getReadPointer (k, from),
                (size_t) frames_to_check);

              /* normalize */
              ch_min[k] = (ch_min[k] + 1.f) / 2.f;
              ch_max[k] = (ch_max[k] + 1.f) / 2.f;
            }
        }
      else
        {
          dsp_copy (prev_min, ch_min, clip->channels_);
          dsp_copy (prev_max, ch_max, clip->channels_);
        }

      for (unsigned int k = 0; k < clip->channels_; k++)
        {
          /* adjust to draw from the middle so it draws bars instead of single
           * points when zoomed */
          if (ch_max[k] > 0.5f && ch_min[k] > 0.5f)
            {
              ch_min[k] = 0.5f;
            }
          if (ch_min[k] < 0.5f && ch_max[k] < 0.5f)
            {
              ch_max[k] = 0.5f;
            }

          double min_y = MAX ((double) ch_min[k] * (double) height_per_ch, 0.0);
          double max_y = MIN (
            (double) ch_max[k] * (double) height_per_ch, (double) height_per_ch);

          /* only draw if non-silent */
          float       avg = (ch_max[k] + ch_min[k]) / 2.f;
          const float epsilon = 0.001f;
          if (
            !math_floats_equal_epsilon (avg, 0.5f, epsilon)
            || ch_max[k] - ch_min[k] > epsilon)
            {
              gtk_snapshot_save (snapshot);
              {
                graphene_point_t tmp_pt =
                  Z_GRAPHENE_POINT_INIT (0, (float) k * (float) height_per_ch);
                gtk_snapshot_translate (snapshot, &tmp_pt);
              }
              {
                graphene_rect_t tmp_r = Z_GRAPHENE_RECT_INIT (
                  (float) i, (float) min_y, (float) MAX (width, 1),
                  (float) MAX (max_y - min_y, 1));
                gtk_snapshot_append_color (snapshot, &audio_lines_color, &tmp_r);
              }
              gtk_snapshot_restore (snapshot);
            }

          prev_min[k] = ch_min[k];
          prev_max[k] = ch_max[k];
        } /* endforeach channel k */

      prev_frames = curr_frames;
    }
  free (ch_min);
  free (ch_max);
  free (prev_min);
  free (prev_max);

  /* draw gain line */
  float gain_fader_val = math_get_fader_val_from_amp (ar->gain_);
  int   gain_line_start_x = ui_pos_to_px_editor (obj->pos_, F_PADDING);
  int   gain_line_end_x = ui_pos_to_px_editor (obj->end_pos_, F_PADDING);
  /* rect x:  need 1 pixel extra for some reason */
  /* rect y:  invert because gtk draws the opposite way */
  graphene_rect_t tmp_r = Z_GRAPHENE_RECT_INIT (
    1.f + (float) gain_line_start_x,
    (float) rect->height * (1.f - gain_fader_val),
    (float) (gain_line_end_x - gain_line_start_x), 2.f);

  {
    auto tmp_color = UI_COLORS->bright_orange.to_gdk_rgba ();
    gtk_snapshot_append_color (snapshot, &tmp_color, &tmp_r);
  }

  /* draw gain text */
  double gain_db = math_amp_to_dbfs (ar->gain_);
  char   gain_txt[50];
  sprintf (gain_txt, "%.1fdB", gain_db);
  int gain_txt_padding = 3;
  pango_layout_set_markup (self->audio_layout.get (), gain_txt, -1);
  gtk_snapshot_save (snapshot);
  {
    graphene_point_t tmp_pt = Z_GRAPHENE_POINT_INIT (
      (float) (gain_txt_padding + gain_line_start_x),
      (float) (gain_txt_padding
               + (int) ((double) rect->height * (1.0 - gain_fader_val))));
    gtk_snapshot_translate (snapshot, &tmp_pt);
  }
  GdkRGBA tmp_color = Z_GDK_RGBA_INIT (1, 1, 1, 1);
  gtk_snapshot_append_layout (snapshot, self->audio_layout.get (), &tmp_color);
  gtk_snapshot_restore (snapshot);
}

static void
draw_automation_bg (
  ArrangerWidget * self,
  GtkSnapshot *    snapshot,
  const int        width,
  const int        height,
  GdkRectangle *   rect)
{
  /* draws horizontal lines at the cut-off points where automation can be drawn */
#if 0
  gtk_snapshot_append_color (
    snapshot, &thick_grid_line_color,
    &Z_Z_GRAPHENE_RECT_INIT (
      0, (float) AUTOMATION_ARRANGER_VPADDING,
      width, 1));
  gtk_snapshot_append_color (
    snapshot, &thick_grid_line_color,
    &Z_Z_GRAPHENE_RECT_INIT (
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
  if (self->ruler_display == Transport::Display::Time)
    {
      /* get sec interval */
      int sec_interval = ruler_widget_get_sec_interval (ruler);

      /* get 10 sec interval */
      int ten_sec_interval = ruler_widget_get_10sec_interval (ruler);

      /* get the interval for mins */
      int min_interval =
        (int) MAX ((RW_PX_TO_HIDE_BEATS) / (double) ruler->px_per_min, 1.0);

      int    i = 0;
      double curr_px;
      while (
        (curr_px = ruler->px_per_min * (i += min_interval) + SPACE_BEFORE_START)
        < rect->x + rect->width)
        {
          if (curr_px < rect->x)
            continue;

          {
            graphene_rect_t tmp_r = Z_GRAPHENE_RECT_INIT (
              (float) curr_px, (float) rect->y, (float) thick_width,
              (float) height);
            gtk_snapshot_append_color (snapshot, &thick_color, &tmp_r);
          }
        }
      i = 0;
      if (ten_sec_interval > 0)
        {
          while (
            (curr_px =
               ruler->px_per_10sec * (i += ten_sec_interval) + SPACE_BEFORE_START)
            < rect->x + rect->width)
            {
              if (curr_px < rect->x)
                continue;

              {
                graphene_rect_t tmp_r = Z_GRAPHENE_RECT_INIT (
                  (float) curr_px, (float) rect->y, (float) thinner_width,
                  (float) height);
                gtk_snapshot_append_color (snapshot, &thinner_color, &tmp_r);
              }
            }
        }
      i = 0;
      if (sec_interval > 0)
        {
          while (
            (curr_px =
               ruler->px_per_sec * (i += sec_interval) + SPACE_BEFORE_START)
            < rect->x + rect->width)
            {
              if (curr_px < rect->x)
                continue;

              {
                graphene_rect_t tmp_r = Z_GRAPHENE_RECT_INIT (
                  (float) curr_px, (float) rect->y, (float) thinnest_width,
                  (float) height);
                gtk_snapshot_append_color (snapshot, &thinnest_color, &tmp_r);
              }
            }
        }
    }
  /* else if BBT display */
  else
    {
      /* get sixteenth interval */
      int sixteenth_interval = ruler_widget_get_sixteenth_interval (ruler);

      /* get the beat interval */
      int beat_interval = ruler_widget_get_beat_interval (ruler);

      /* get the interval for bars */
      int bar_interval =
        (int) MAX ((RW_PX_TO_HIDE_BEATS) / (double) ruler->px_per_bar, 1.0);

      int    i = 0;
      double curr_px;
      while (
        (curr_px = ruler->px_per_bar * (i += bar_interval) + SPACE_BEFORE_START)
        < rect->x + rect->width)
        {
          if (curr_px < rect->x)
            continue;

          {
            graphene_rect_t tmp_r = Z_GRAPHENE_RECT_INIT (
              (float) curr_px, (float) rect->y, (float) thick_width,
              (float) height);
            gtk_snapshot_append_color (snapshot, &thick_color, &tmp_r);
          }
        }
      i = 0;
      if (beat_interval > 0)
        {
          while (
            (curr_px =
               ruler->px_per_beat * (i += beat_interval) + SPACE_BEFORE_START)
            < rect->x + rect->width)
            {
              if (curr_px < rect->x)
                continue;

              {
                graphene_rect_t tmp_r = Z_GRAPHENE_RECT_INIT (
                  (float) curr_px, (float) rect->y, (float) thinner_width,
                  (float) height);
                gtk_snapshot_append_color (snapshot, &thinner_color, &tmp_r);
              }
            }
        }
      i = 0;
      if (sixteenth_interval > 0)
        {
          while (
            (curr_px =
               ruler->px_per_sixteenth * (i += sixteenth_interval)
               + SPACE_BEFORE_START)
            < rect->x + rect->width)
            {
              if (curr_px < rect->x)
                continue;

              {
                graphene_rect_t tmp_r = Z_GRAPHENE_RECT_INIT (
                  (float) curr_px, (float) rect->y, (float) thinnest_width,
                  (float) height);
                gtk_snapshot_append_color (snapshot, &thinnest_color, &tmp_r);
              }
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
  GdkRGBA color = Z_GDK_RGBA_INIT (0.3f, 0.3f, 0.3f, 0.3f);
  {
    graphene_rect_t tmp_r = Z_GRAPHENE_RECT_INIT (
      (float) MAX (0, range_first_px), (float) rect->y,
      (float) (range_second_px - range_first_px), (float) rect->height);
    gtk_snapshot_append_color (snapshot, &color, &tmp_r);
  }

  /* draw start and end lines */
  const float   line_width = 2.f;
  const GdkRGBA tmp_color = { 0.8f, 0.8f, 0.8f, 0.4f };
  {
    graphene_rect_t tmp_r = Z_GRAPHENE_RECT_INIT (
      (float) range_first_px + 1.f, (float) rect->y, line_width,
      (float) rect->height);
    gtk_snapshot_append_color (snapshot, &tmp_color, &tmp_r);
  }
  {
    graphene_rect_t tmp_r = Z_GRAPHENE_RECT_INIT (
      (float) range_second_px + 1.f, (float) rect->y, line_width,
      (float) rect->height);
    gtk_snapshot_append_color (snapshot, &color, &tmp_r);
  }
}

void
arranger_snapshot (GtkWidget * widget, GtkSnapshot * snapshot)
{
  ArrangerWidget * self = Z_ARRANGER_WIDGET (widget);

  GdkRectangle visible_rect_gdk;
  arranger_widget_get_visible_rect (self, &visible_rect_gdk);
  graphene_rect_t visible_rect;
  z_gdk_rectangle_to_graphene_rect_t (&visible_rect, &visible_rect_gdk);

  if (self->first_draw)
    {
      self->first_draw = false;
    }

  gint64 start_time = g_get_monotonic_time ();

  int width = gtk_widget_get_width (GTK_WIDGET (self));
  int height = gtk_widget_get_height (GTK_WIDGET (self));

  RulerWidget * ruler = arranger_widget_get_ruler (self);
  if (ruler->px_per_bar < 2.0)
    return;

  /* skip drawing if rectangle too large */
  if (visible_rect.size.width > 10000 || visible_rect.size.height > 10000)
    {
      z_warning ("skipping draw - rectangle too large");
      return;
    }

  /*z_info (*/
  /*"redrawing arranger in rect: "*/
  /*"(%d, %d) width: %d height %d)",*/
  /*rect.x, rect.y, rect.width, rect.height);*/
  self->last_rect = visible_rect;

#if 0
  cairo_antialias_t antialias =
    cairo_get_antialias (self->cached_cr);
  double tolerance =
    cairo_get_tolerance (self->cached_cr);
  cairo_set_antialias (
    self->cached_cr, CAIRO_ANTIALIAS_FAST);
  cairo_set_tolerance (self->cached_cr, 1.5);
#endif

  /* pretend we're drawing from 0, 0 */
  gtk_snapshot_save (snapshot);
  {
    graphene_point_t tmp_pt =
      Z_GRAPHENE_POINT_INIT (-visible_rect.origin.x, -visible_rect.origin.y);
    gtk_snapshot_translate (snapshot, &tmp_pt);
  }

  /* draw loop background */
  if (TRANSPORT->loop_)
    {
      double start_px = 0, end_px = 0;
      if (self->type == ArrangerWidgetType::Timeline)
        {
          start_px = ui_pos_to_px_timeline (TRANSPORT->loop_start_pos_, 1);
          end_px = ui_pos_to_px_timeline (TRANSPORT->loop_end_pos_, 1);
        }
      else
        {
          start_px = ui_pos_to_px_editor (TRANSPORT->loop_start_pos_, 1);
          end_px = ui_pos_to_px_editor (TRANSPORT->loop_end_pos_, 1);
        }
      GdkRGBA loop_color = { 0, 0.9f, 0.7f, 0.08f };

      /* draw the loop start line */
      {
        graphene_rect_t tmp_r = Z_GRAPHENE_RECT_INIT (
          (float) start_px + 1.f, visible_rect.origin.y, 2.f,
          visible_rect.size.height);
        gtk_snapshot_append_color (snapshot, &loop_color, &tmp_r);
      }

      /* draw loop end line */
      {
        graphene_rect_t tmp_r = Z_GRAPHENE_RECT_INIT (
          (float) end_px + 1.f, visible_rect.origin.y, 2.f,
          visible_rect.size.height);
        gtk_snapshot_append_color (snapshot, &loop_color, &tmp_r);
      }

      /* draw transport loop area */
      double  loop_start_x = MAX (0, start_px);
      GdkRGBA tmp_color = Z_GDK_RGBA_INIT (0, 0.9f, 0.7f, 0.02f);
      {
        graphene_rect_t tmp_r = Z_GRAPHENE_RECT_INIT (
          (float) loop_start_x, visible_rect.origin.y,
          (float) (end_px - start_px), visible_rect.size.height);
        gtk_snapshot_append_color (snapshot, &tmp_color, &tmp_r);
      }
    }

  /* --- handle vertical drawing --- */

  draw_vertical_lines (self, ruler, snapshot, &visible_rect_gdk);

  /* draw range */
  int  range_first_px, range_second_px;
  bool have_range = false;
  if (
    self->type == ArrangerWidgetType::Audio && AUDIO_SELECTIONS->has_selection_)
    {
      Position *range_first_pos, *range_second_pos;
      if (TRANSPORT->range_1_ <= TRANSPORT->range_2_)
        {
          range_first_pos = &AUDIO_SELECTIONS->sel_start_;
          range_second_pos = &AUDIO_SELECTIONS->sel_end_;
        }
      else
        {
          range_first_pos = &AUDIO_SELECTIONS->sel_end_;
          range_second_pos = &AUDIO_SELECTIONS->sel_start_;
        }

      range_first_px = ui_pos_to_px_editor (*range_first_pos, 1);
      range_second_px = ui_pos_to_px_editor (*range_second_pos, 1);
      have_range = true;
    }
  else if (self->type == ArrangerWidgetType::Timeline && TRANSPORT->has_range_)
    {
      /* in order they appear */
      Position *range_first_pos, *range_second_pos;
      if (TRANSPORT->range_1_ <= TRANSPORT->range_2_)
        {
          range_first_pos = &TRANSPORT->range_1_;
          range_second_pos = &TRANSPORT->range_2_;
        }
      else
        {
          range_first_pos = &TRANSPORT->range_2_;
          range_second_pos = &TRANSPORT->range_1_;
        }

      range_first_px = ui_pos_to_px_timeline (*range_first_pos, 1);
      range_second_px = ui_pos_to_px_timeline (*range_second_pos, 1);
      have_range = true;
    }

  if (have_range)
    {
      draw_range (
        self, snapshot, range_first_px, range_second_px, &visible_rect_gdk);
    }

  if (self->type == ArrangerWidgetType::Timeline)
    {
      draw_timeline_bg (self, snapshot, &visible_rect_gdk);
    }
  else if (self->type == ArrangerWidgetType::Midi)
    {
      draw_midi_bg (self, snapshot, &visible_rect_gdk);
    }
  else if (self->type == ArrangerWidgetType::MidiModifier)
    {
      draw_velocity_bg (self, snapshot, &visible_rect_gdk);
    }
  else if (self->type == ArrangerWidgetType::Audio)
    {
      draw_audio_bg (self, snapshot, &visible_rect_gdk);
    }
  else if (self->type == ArrangerWidgetType::Automation)
    {
      draw_automation_bg (self, snapshot, width, height, &visible_rect_gdk);
    }

  /* draw each arranger object */
  std::vector<ArrangerObject *> hit_objs_to_draw;
  hit_objs_to_draw.reserve (200);
  arranger_widget_get_hit_objects_in_rect (
    self, ArrangerObject::Type::All, &visible_rect_gdk, hit_objs_to_draw);

  for (auto obj : hit_objs_to_draw)
    {
      draw_arranger_object (self, obj, snapshot, &visible_rect_gdk);
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
  z_debug ("finished drawing in {} microseconds, "
    "rect x:%d y:%d w:%d h:%d for %s "
    "arranger",
    end_time - start_time,
    rect.x, rect.y, rect.width, rect.height,
    arranger_widget_get_type_str (self));
#endif
}
