// SPDX-FileCopyrightText: © 2018-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "common/dsp/control_port.h"
#include "common/dsp/instrument_track.h"
#include "common/dsp/track.h"
#include "common/utils/color.h"
#include "common/utils/gtk.h"
#include "common/utils/objects.h"
#include "common/utils/ui.h"
#include "gui/cpp/backend/zrythm.h"
#include "gui/cpp/gtk_widgets/automation_mode.h"
#include "gui/cpp/gtk_widgets/custom_button.h"
#include "gui/cpp/gtk_widgets/track.h"
#include "gui/cpp/gtk_widgets/track_canvas.h"
#include "gui/cpp/gtk_widgets/zrythm_app.h"

#include <glib/gi18n.h>

G_DEFINE_TYPE (TrackCanvasWidget, track_canvas_widget, GTK_TYPE_WIDGET)

#ifdef _WIN32
constexpr auto AUTOMATABLE_NAME_FONT = "8.5";
constexpr auto AUTOMATABLE_VAL_FONT = "7";
#else
constexpr auto AUTOMATABLE_NAME_FONT = "9.5";
constexpr auto AUTOMATABLE_VAL_FONT = "8";
#endif

/** Width of the "label" to show the automation
 * value. */
#define AUTOMATION_VALUE_WIDTH 32

#define icon_texture_size 16

static void
recreate_icon_texture (TrackCanvasWidget * self)
{
  object_free_w_func_and_null (g_object_unref, self->track_icon);
  self->track_icon = z_gdk_texture_new_from_icon_name (
    self->parent->track->icon_name_.c_str (), icon_texture_size,
    icon_texture_size, 1);
  self->last_track_icon_name = self->parent->track->icon_name_;
}

static void
update_pango_layouts (TrackCanvasWidget * self, int w, int h)
{
  if (!self->layout)
    {
      self->layout = gtk_widget_create_pango_layout (GTK_WIDGET (self), nullptr);
      pango_layout_set_ellipsize (self->layout, PANGO_ELLIPSIZE_END);
    }

  if (!self->automation_value_layout)
    {
      self->automation_value_layout =
        gtk_widget_create_pango_layout (GTK_WIDGET (self), nullptr);
      PangoFontDescription * desc =
        pango_font_description_from_string (AUTOMATABLE_VAL_FONT);
      pango_layout_set_font_description (self->automation_value_layout, desc);
      pango_font_description_free (desc);
    }

  if (!self->lane_layout)
    {
      self->lane_layout =
        gtk_widget_create_pango_layout (GTK_WIDGET (self), nullptr);
      pango_layout_set_ellipsize (self->lane_layout, PANGO_ELLIPSIZE_END);
    }

  pango_layout_set_width (self->layout, pango_units_from_double (w));
  pango_layout_set_width (
    self->automation_value_layout, pango_units_from_double (w));
  pango_layout_set_width (self->lane_layout, pango_units_from_double (w));
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
  auto bg_color = tw->track->color_;
  if (!track->is_enabled ())
    {
      bg_color.red_ = 0.5;
      bg_color.green_ = 0.5;
      bg_color.blue_ = 0.5;
    }
  if (tw->color_area_hovered)
    {
      bg_color.brighten_default ();
    }
  {
    graphene_rect_t tmp_r =
      Z_GRAPHENE_RECT_INIT (0.f, 0.f, TRACK_COLOR_AREA_WIDTH, (float) height);
    z_gtk_snapshot_append_color (snapshot, bg_color, &tmp_r);
  }

  /* TODO */
  auto c2 = track->color_.get_contrast_color ();
  auto c3 = c2.get_contrast_color ();

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
      c2.brighten_default ();
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

  if (!self->track_icon || self->last_track_icon_name != track->icon_name_)
    {
      recreate_icon_texture (self);
    }

  /* TODO figure out how to draw dark colors */
  graphene_matrix_t color_matrix;
  const float       float_arr[16] = {
    1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 0, 0, 0, c2.alpha_
  };
  graphene_matrix_init_from_float (&color_matrix, float_arr);
  graphene_vec4_t color_offset;
  graphene_vec4_init (&color_offset, c2.red_, c2.green_, c2.blue_, 0);

  gtk_snapshot_push_color_matrix (snapshot, &color_matrix, &color_offset);

  {
    graphene_rect_t tmp_r =
      Z_GRAPHENE_RECT_INIT (0, 0, icon_texture_size, icon_texture_size);
    gtk_snapshot_append_texture (snapshot, self->track_icon, &tmp_r);
  }

  gtk_snapshot_pop (snapshot);
}

static void
draw_name (TrackCanvasWidget * self, GtkSnapshot * snapshot, int width)
{
  TrackWidget * tw = self->parent;
  Track *       track = tw->track;

  std::string name = track->name_;
  if (DEBUGGING)
    {
      int size = 0;
      if (auto foldable_tr = dynamic_cast<FoldableTrack *> (track))
        {
          size = foldable_tr->size_;
        }
      name = fmt::format (
        "{}[{} - {}] {}", track->is_selected () ? "* " : "", track->pos_, size,
        track->name_);
    }

  int first_button_x =
    width - (TRACK_BUTTON_SIZE + TRACK_BUTTON_PADDING) * tw->top_buttons.size ();

  PangoLayout * layout = self->layout;
  pango_layout_set_text (layout, name.c_str (), -1);
  pango_layout_set_width (layout, pango_units_from_double (first_button_x - 22));

  gtk_snapshot_save (snapshot);
  {
    graphene_point_t tmp_pt = Z_GRAPHENE_POINT_INIT (22.f, 2.f);
    gtk_snapshot_translate (snapshot, &tmp_pt);
  }
  GdkRGBA white = Z_GDK_RGBA_INIT (1, 1, 1, 1);
  gtk_snapshot_append_layout (snapshot, layout, &white);
  gtk_snapshot_restore (snapshot);
}

/**
 * @param top 1 to draw top, 0 to draw bottom.
 * @param width Track width.
 */
static void
draw_buttons (TrackCanvasWidget * self, GtkSnapshot * snapshot, int top, int width)
{
  TrackWidget * tw = self->parent;

  CustomButtonWidget * hovered_cb = tw->last_hovered_btn;
  auto                &buttons = top ? tw->top_buttons : tw->bot_buttons;
  Track *              track = tw->track;
  for (size_t i = 0; i < buttons.size (); ++i)
    {
      auto &cb = buttons[i];

      if (top)
        {
          cb->x =
            width
            - (TRACK_BUTTON_SIZE + TRACK_BUTTON_PADDING) * (buttons.size () - i);
          cb->y = TRACK_BUTTON_PADDING_FROM_EDGE;
        }
      else
        {
          cb->x =
            width
            - (TRACK_BUTTON_SIZE + TRACK_BUTTON_PADDING)
                * (tw->bot_buttons.size () - i);
          cb->y =
            track->main_height_
            - (TRACK_BUTTON_PADDING_FROM_EDGE + TRACK_BUTTON_SIZE);
        }

      auto state = CustomButtonWidget::State::NORMAL;

      bool is_solo = TRACK_CB_ICON_IS (SOLO);

      auto ch_track = dynamic_cast<ChannelTrack *> (track);
      if (cb.get () == tw->clicked_button)
        {
          /* currently clicked button */
          state = CustomButtonWidget::State::ACTIVE;
        }
      else if (is_solo && track->get_soloed ())
        {
          state = CustomButtonWidget::State::TOGGLED;
        }
      else if (is_solo && track->get_implied_soloed ())
        {
          state = CustomButtonWidget::State::SEMI_TOGGLED;
        }
      else if (
        TRACK_CB_ICON_IS (SHOW_UI)
        && dynamic_cast<InstrumentTrack *> (track)->is_plugin_visible ())
        {
          state = CustomButtonWidget::State::TOGGLED;
        }
      else if (TRACK_CB_ICON_IS (MUTE) && track->get_muted ())
        {
          state = CustomButtonWidget::State::TOGGLED;
        }
      else if (TRACK_CB_ICON_IS (LISTEN) && track->get_listened ())
        {
          state = CustomButtonWidget::State::TOGGLED;
        }
      else if (
        TRACK_CB_ICON_IS (MONITOR_AUDIO)
        && dynamic_cast<ProcessableTrack *> (track)->get_monitor_audio ())
        {
          state = CustomButtonWidget::State::TOGGLED;
        }
      else if (TRACK_CB_ICON_IS (FREEZE) && track->frozen_)
        {
          state = CustomButtonWidget::State::TOGGLED;
        }
      else if (
        TRACK_CB_ICON_IS (MONO_COMPAT)
        && ch_track->channel_->get_mono_compat_enabled ())
        {
          state = CustomButtonWidget::State::TOGGLED;
        }
      else if (
        TRACK_CB_ICON_IS (SWAP_PHASE) && ch_track->channel_->get_swap_phase ())
        {
          state = CustomButtonWidget::State::TOGGLED;
        }
      else if (
        TRACK_CB_ICON_IS (RECORD)
        && dynamic_cast<RecordableTrack *> (track)->get_recording ())
        {
          state = CustomButtonWidget::State::TOGGLED;
        }
      else if (
        TRACK_CB_ICON_IS (SHOW_TRACK_LANES)
        && dynamic_cast<LanedTrack *> (track)->lanes_visible_)
        {
          state = CustomButtonWidget::State::TOGGLED;
        }
      else if (
        TRACK_CB_ICON_IS (SHOW_AUTOMATION_LANES)
        && dynamic_cast<AutomatableTrack *> (track)->automation_visible_)
        {
          state = CustomButtonWidget::State::TOGGLED;
        }
      else if (TRACK_CB_ICON_IS (FOLD_OPEN))
        {
          state = CustomButtonWidget::State::TOGGLED;
        }
      else if (hovered_cb == cb.get ())
        {
          state = CustomButtonWidget::State::HOVERED;
        }

      cb->draw (snapshot, cb->x, cb->y, state);
    }
}

static void
draw_lanes (TrackCanvasWidget * self, GtkSnapshot * snapshot, int width)
{
  TrackWidget * tw = self->parent;
  auto          track = dynamic_cast<LanedTrack *> (tw->track);

  if (!track || !track->lanes_visible_)
    return;

  int  total_height = (int) track->main_height_;
  auto variant = convert_to_variant<LanedTrackPtrVariant> (track);
  std::visit (
    [&] (auto &&t) {
      for (auto &lane : t->lanes_)
        {
          /* remember y */
          lane->y_ = total_height;

          /* draw separator */
          GdkRGBA tmp_color = Z_GDK_RGBA_INIT (1, 1, 1, 0.3f);
          {
            graphene_rect_t tmp_r = Z_GRAPHENE_RECT_INIT (
              TRACK_COLOR_AREA_WIDTH, (float) total_height,
              (float) width - TRACK_COLOR_AREA_WIDTH, 1.f);
            gtk_snapshot_append_color (snapshot, &tmp_color, &tmp_r);
          }

          /* draw text */
          gtk_snapshot_save (snapshot);
          {
            graphene_point_t tmp_pt = Z_GRAPHENE_POINT_INIT (
              TRACK_COLOR_AREA_WIDTH + TRACK_BUTTON_PADDING_FROM_EDGE,
              (float) total_height + TRACK_BUTTON_PADDING_FROM_EDGE);
            gtk_snapshot_translate (snapshot, &tmp_pt);
          }
          PangoLayout * layout = self->lane_layout;
          pango_layout_set_text (layout, lane->name_.c_str (), -1);
          GdkRGBA white = Z_GDK_RGBA_INIT (1, 1, 1, 1);
          gtk_snapshot_append_layout (snapshot, layout, &white);
          gtk_snapshot_restore (snapshot);

          /* create buttons if necessary */
          if (lane->buttons_.empty ())
            {
              auto cb = std::make_unique<CustomButtonWidget> (
                TRACK_ICON_NAME_SOLO, TRACK_BUTTON_SIZE);
              cb->owner_type = CustomButtonWidget::Owner::LANE;
              cb->owner = lane.get ();
              cb->toggled_color = UI_COLORS->solo_checked;
              cb->held_color = UI_COLORS->solo_active;
              lane->buttons_.emplace_back (std::move (cb));
              cb = std::make_unique<CustomButtonWidget> (
                TRACK_ICON_NAME_MUTE, TRACK_BUTTON_SIZE);
              cb->owner_type = CustomButtonWidget::Owner::LANE;
              cb->owner = lane.get ();
              lane->buttons_.emplace_back (std::move (cb));
            }

          /* draw buttons */
          CustomButtonWidget * hovered_cb = track_widget_get_hovered_button (
            tw, (int) tw->last_x, (int) tw->last_y);
          for (size_t j = 0; j < lane->buttons_.size (); ++j)
            {
              auto &cb = lane->buttons_[j];

              cb->x =
                width
                - (TRACK_BUTTON_SIZE + TRACK_BUTTON_PADDING)
                    * (lane->buttons_.size () - j);
              cb->y = total_height + TRACK_BUTTON_PADDING_FROM_EDGE;

              auto state = CustomButtonWidget::State::NORMAL;

              if (cb.get () == tw->clicked_button)
                {
                  /* currently clicked button */
                  state = CustomButtonWidget::State::ACTIVE;
                }
              else if (TRACK_CB_ICON_IS (SOLO) && lane->solo_)
                {
                  state = CustomButtonWidget::State::TOGGLED;
                }
              else if (TRACK_CB_ICON_IS (MUTE) && lane->mute_)
                {
                  state = CustomButtonWidget::State::TOGGLED;
                }
              else if (hovered_cb == cb.get ())
                {
                  state = CustomButtonWidget::State::HOVERED;
                }

              cb->draw (snapshot, cb->x, cb->y, state);
            }

          total_height += (int) lane->height_;
        }
    },
    variant);
}

static void
draw_automation (TrackCanvasWidget * self, GtkSnapshot * snapshot, int width)
{
  TrackWidget * tw = self->parent;
  auto          track = dynamic_cast<AutomatableTrack *> (tw->track);

  if (!track || !track->automation_visible_)
    return;

  auto  &atl = track->get_automation_tracklist ();
  double total_height = track->main_height_;

  if (const auto * laned_track = dynamic_cast<LanedTrack *> (track))
    {
      total_height += std::visit (
        [] (const auto &laned_t) {
          return laned_t->get_visible_lane_heights ();
        },
        convert_to_variant<LanedTrackPtrVariant> (laned_track));
    }

  for (size_t i = 0; i < atl.visible_ats_.size (); i++)
    {
      auto at = atl.visible_ats_[i];

      if (!at->created_)
        continue;

      /* remember y */
      at->y_ = static_cast<int> (total_height);

      /* draw separator above at */
      GdkRGBA sep_color = Z_GDK_RGBA_INIT (1, 1, 1, 0.3);
      {
        graphene_rect_t tmp_r = Z_GRAPHENE_RECT_INIT (
          TRACK_COLOR_AREA_WIDTH, (float) total_height,
          (float) width - TRACK_COLOR_AREA_WIDTH, 1.f);
        gtk_snapshot_append_color (snapshot, &sep_color, &tmp_r);
      }

      /* create buttons if necessary */
      if (at->top_left_buttons_.empty ())
        {
          auto cb = std::make_unique<CustomButtonWidget> (
            TRACK_ICON_NAME_SHOW_AUTOMATION_LANES, TRACK_BUTTON_SIZE);
          cb->owner_type = CustomButtonWidget::Owner::AT;
          cb->owner = at;
          /*char text[500];*/
          /*sprintf (*/
          /*text, "%d - %s",*/
          /*at->index, at->automatable->label);*/
          cb->set_text (
            self->layout, at->port_id_.label_.c_str (), AUTOMATABLE_NAME_FONT);
          pango_layout_set_ellipsize (cb->layout, PANGO_ELLIPSIZE_END);
          at->top_left_buttons_.emplace_back (std::move (cb));
        }
      if (at->top_right_buttons_.empty ())
        {
#if 0
          at->top_right_buttons[0] =
            custom_button_widget_new (
              TRACK_ICON_NAME_MUTE, TRACK_BUTTON_SIZE);
          at->top_right_buttons[0]->owner_type =
            CustomButtonWidget::Owner::AT;
          at->top_right_buttons[0]->owner = at;
          at->num_top_right_buttons = 1;
#endif
        }
      if (at->bot_left_buttons_.empty ())
        {
        }
      if (!at->am_widget_)
        {
          at->am_widget_ = std::make_unique<AutomationModeWidget> (
            TRACK_BUTTON_SIZE, self->layout, at);
        }
      if (at->bot_right_buttons_.empty ())
        {
          auto cb = std::make_unique<CustomButtonWidget> (
            TRACK_ICON_NAME_MINUS, TRACK_BUTTON_SIZE);
          cb->owner_type = CustomButtonWidget::Owner::AT;
          cb->owner = at;
          at->bot_right_buttons_.emplace_back (std::move (cb));

          cb = std::make_unique<CustomButtonWidget> (
            TRACK_ICON_NAME_PLUS, TRACK_BUTTON_SIZE);
          cb->owner_type = CustomButtonWidget::Owner::AT;
          cb->owner = at;
          at->bot_right_buttons_.emplace_back (std::move (cb));
        }

      /* draw top left buttons */
      CustomButtonWidget *   hovered_cb = tw->last_hovered_btn;
      AutomationModeWidget * hovered_am = track_widget_get_hovered_am_widget (
        tw, (int) tw->last_x, (int) tw->last_y);
      for (auto &cb : at->top_left_buttons_)
        {
          cb->x = TRACK_BUTTON_PADDING_FROM_EDGE + TRACK_COLOR_AREA_WIDTH;
          cb->y = total_height + TRACK_BUTTON_PADDING_FROM_EDGE;

          auto state = CustomButtonWidget::State::NORMAL;

          if (cb.get () == tw->clicked_button)
            {
              /* currently clicked button */
              state = CustomButtonWidget::State::ACTIVE;
            }
          else if (hovered_cb == cb.get ())
            {
              state = CustomButtonWidget::State::HOVERED;
            }

          cb->draw_with_text (
            snapshot, cb->x, cb->y,
            width
              - (TRACK_COLOR_AREA_WIDTH + TRACK_BUTTON_PADDING_FROM_EDGE * 2 + at->top_right_buttons_.size() * (TRACK_BUTTON_SIZE + TRACK_BUTTON_PADDING) + AUTOMATION_VALUE_WIDTH + TRACK_BUTTON_PADDING),
            state);
        }

      /* draw automation value */
      PangoLayout * layout = self->automation_value_layout;
      auto  port = Port::find_from_identifier<ControlPort> (at->port_id_);
      auto  str = fmt::format ("{:.2f}", port->get_val ());
      auto &cb = at->top_left_buttons_[0];
      pango_layout_set_text (layout, str.c_str (), -1);
      PangoRectangle pangorect;
      pango_layout_get_pixel_extents (layout, nullptr, &pangorect);

      float orig_start_x = (float) (cb->x + cb->width + TRACK_BUTTON_PADDING);
      float orig_start_y = (float) total_height + TRACK_BUTTON_PADDING_FROM_EDGE;
      float remaining_x =
        ((float) width - orig_start_x) - (float) pangorect.width;
      float remaining_y = (float) cb->size - (float) pangorect.height;

      gtk_snapshot_save (snapshot);
      {
        graphene_point_t tmp_pt = Z_GRAPHENE_POINT_INIT (
          orig_start_x + remaining_x / 2.f, orig_start_y + remaining_y / 2.f);
        gtk_snapshot_translate (snapshot, &tmp_pt);
      }
      GdkRGBA white = Z_GDK_RGBA_INIT (1, 1, 1, 1);
      gtk_snapshot_append_layout (snapshot, layout, &white);
      gtk_snapshot_restore (snapshot);

      /* draw top right buttons */
      for (size_t j = 0; j < at->top_right_buttons_.size (); ++j)
        {
          auto &cur_cb = at->top_right_buttons_[j];

          cur_cb->x =
            width
            - (TRACK_BUTTON_SIZE + TRACK_BUTTON_PADDING)
                * (at->top_right_buttons_.size () - j);
          cur_cb->y = total_height + TRACK_BUTTON_PADDING_FROM_EDGE;

          auto state = CustomButtonWidget::State::NORMAL;

          if (cur_cb.get () == tw->clicked_button)
            {
              /* currently clicked button */
              state = CustomButtonWidget::State::ACTIVE;
            }
          else if (hovered_cb == cur_cb.get ())
            {
              state = CustomButtonWidget::State::HOVERED;
            }

          cur_cb->draw (snapshot, cur_cb->x, cur_cb->y, state);
        }

      if (TRACK_BOT_BUTTONS_SHOULD_BE_VISIBLE (at->height_))
        {
          /* automation mode */
          auto &am = at->am_widget_;

          am->x_ = TRACK_BUTTON_PADDING_FROM_EDGE + TRACK_COLOR_AREA_WIDTH;
          am->y_ =
            (total_height + at->height_)
            - (TRACK_BUTTON_PADDING_FROM_EDGE + TRACK_BUTTON_SIZE);

          auto state = CustomButtonWidget::State::NORMAL;
          if (tw->clicked_am == am.get ())
            {
              /* currently clicked button */
              state = CustomButtonWidget::State::ACTIVE;
            }
          else if (hovered_am == am.get ())
            {
              state = CustomButtonWidget::State::HOVERED;
            }

          am->draw (snapshot, am->x_, am->y_, tw->last_x, state);

          for (size_t j = 0; j < at->bot_right_buttons_.size (); ++j)
            {
              auto &cur_cb = at->bot_right_buttons_[j];

              cur_cb->x =
                width
                - (TRACK_BUTTON_SIZE + TRACK_BUTTON_PADDING)
                    * (at->bot_right_buttons_.size () - j);
              cur_cb->y =
                (total_height + at->height_)
                - (TRACK_BUTTON_PADDING_FROM_EDGE + TRACK_BUTTON_SIZE);

              state = CustomButtonWidget::State::NORMAL;

              if (cur_cb.get () == tw->clicked_button)
                {
                  /* currently clicked button */
                  state = CustomButtonWidget::State::ACTIVE;
                }
              else if (hovered_cb == cur_cb.get ())
                {
                  state = CustomButtonWidget::State::HOVERED;
                }

              cur_cb->draw (snapshot, cur_cb->x, cur_cb->y, state);
            }
        }
      total_height += (int) at->height_;
    }
}

static void
track_canvas_snapshot (GtkWidget * widget, GtkSnapshot * snapshot)
{
  TrackCanvasWidget * self = Z_TRACK_CANVAS_WIDGET (widget);

  int width = gtk_widget_get_width (widget);
  int height = gtk_widget_get_height (widget);

  if (self->last_width != width || self->last_height != height)
    {
      update_pango_layouts (self, width, height);
    }

  TrackWidget * tw = self->parent;
  Track *       track = tw->track;

  GdkRectangle rect = { .x = 0, .y = 0, .width = width, .height = height };

  const int   line_w = MIN (128, width);
  const int   line_h = 4;
  const float line_x = (float) (width / 2 - line_w / 2);
  switch (tw->highlight_loc)
    {
    case TrackWidgetHighlight::TRACK_WIDGET_HIGHLIGHT_NONE:
      break;
    case TrackWidgetHighlight::TRACK_WIDGET_HIGHLIGHT_TOP:
      {
        graphene_rect_t tmp_r =
          Z_GRAPHENE_RECT_INIT (line_x, 0.f, (float) line_w, (float) line_h);
        z_gtk_snapshot_append_color (snapshot, UI_COLORS->bright_orange, &tmp_r);
      }
      break;
    case TrackWidgetHighlight::TRACK_WIDGET_HIGHLIGHT_INSIDE:
      {
        graphene_rect_t tmp_r = Z_GRAPHENE_RECT_INIT (
          line_x, (float) (height / 2 - line_h / 2), (float) line_w,
          (float) line_h);
        z_gtk_snapshot_append_color (snapshot, UI_COLORS->bright_orange, &tmp_r);
      }
      break;
    case TrackWidgetHighlight::TRACK_WIDGET_HIGHLIGHT_BOTTOM:
      {
        graphene_rect_t tmp_r = Z_GRAPHENE_RECT_INIT (
          line_x, (float) (height - line_h), (float) line_w, (float) line_h);
        z_gtk_snapshot_append_color (snapshot, UI_COLORS->bright_orange, &tmp_r);
      }
      break;
    }

  /* tint background */
  GdkRGBA tint_color = track->color_.to_gdk_rgba_with_alpha (0.15f);
  {
    graphene_rect_t tmp_r =
      Z_GRAPHENE_RECT_INIT (0.f, 0.f, (float) width, (float) height);
    gtk_snapshot_append_color (snapshot, &tint_color, &tmp_r);
  }

  if (track->is_chord ())
    {
      /* show where scales start */
      PangoLayout * layout = self->automation_value_layout;
      char          scales_txt[100];
      sprintf (scales_txt, "%s", _ ("Scales"));
      pango_layout_set_text (layout, scales_txt, -1);
      PangoRectangle pangorect;
      pango_layout_get_pixel_extents (layout, nullptr, &pangorect);
      gtk_snapshot_save (snapshot);
      {
        graphene_point_t tmp_pt =
          Z_GRAPHENE_POINT_INIT (22.f, (float) (height - pangorect.height - 2));
        gtk_snapshot_translate (snapshot, &tmp_pt);
      }
      GdkRGBA white = Z_GDK_RGBA_INIT (1, 1, 1, 1);
      gtk_snapshot_append_layout (snapshot, layout, &white);
      gtk_snapshot_restore (snapshot);
    }

  if (tw->bg_hovered)
    {
      GdkRGBA tmp_color = Z_GDK_RGBA_INIT (1, 1, 1, 0.1);
      {
        graphene_rect_t tmp_r =
          Z_GRAPHENE_RECT_INIT (0.f, 0.f, (float) width, (float) height);
        gtk_snapshot_append_color (snapshot, &tmp_color, &tmp_r);
      }
    }
  else if (track->is_selected ())
    {
      GdkRGBA tmp_color = Z_GDK_RGBA_INIT (1, 1, 1, 0.07);
      {
        graphene_rect_t tmp_r =
          Z_GRAPHENE_RECT_INIT (0.f, 0.f, (float) width, (float) height);
        gtk_snapshot_append_color (snapshot, &tmp_color, &tmp_r);
      }
    }

  draw_color_area (self, snapshot, &rect, height);

  draw_name (self, snapshot, width);

  draw_buttons (self, snapshot, 1, width);

  /* only show bot buttons if enough space */
  if (TRACK_BOT_BUTTONS_SHOULD_BE_VISIBLE (track->main_height_))
    {
      draw_buttons (self, snapshot, 0, width);
    }

  draw_lanes (self, snapshot, width);

  draw_automation (self, snapshot, width);

  self->last_width = width;
  self->last_height = height;
}

void
track_canvas_widget_setup (TrackCanvasWidget * self, TrackWidget * parent)
{
  self->parent = parent;
}

static void
finalize (TrackCanvasWidget * self)
{
  std::destroy_at (&self->last_track_icon_name);

  object_free_w_func_and_null (g_object_unref, self->track_icon);
  object_free_w_func_and_null (g_object_unref, self->layout);
  object_free_w_func_and_null (g_object_unref, self->automation_value_layout);
  object_free_w_func_and_null (g_object_unref, self->lane_layout);

  G_OBJECT_CLASS (track_canvas_widget_parent_class)->finalize (G_OBJECT (self));
}

static gboolean
tick_cb (GtkWidget * widget, GdkFrameClock * frame_clock, gpointer user_data)
{
  TrackCanvasWidget * self = Z_TRACK_CANVAS_WIDGET (user_data);

  TrackWidget * tw = self->parent;
  Track *       track = tw->track;
  if (!track)
    return G_SOURCE_CONTINUE;

  if (track->is_selected ())
    {
      gtk_widget_add_css_class (widget, "caption-heading");
      gtk_widget_remove_css_class (widget, "caption");
    }
  else
    {
      gtk_widget_add_css_class (widget, "caption");
      gtk_widget_remove_css_class (widget, "caption-heading");
    }

  return G_SOURCE_CONTINUE;
}

static void
track_canvas_widget_init (TrackCanvasWidget * self)
{
  std::construct_at (&self->last_track_icon_name);

  gtk_widget_add_tick_callback (GTK_WIDGET (self), tick_cb, self, nullptr);
}

static void
track_canvas_widget_class_init (TrackCanvasWidgetClass * klass)
{
  GtkWidgetClass * wklass = GTK_WIDGET_CLASS (klass);
  wklass->snapshot = track_canvas_snapshot;
  gtk_widget_class_set_css_name (wklass, "track-canvas");

  GObjectClass * oklass = G_OBJECT_CLASS (klass);
  oklass->finalize = (GObjectFinalizeFunc) finalize;
}
