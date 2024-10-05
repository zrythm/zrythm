// SPDX-FileCopyrightText: © 2018-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "common/dsp/arranger_object.h"
#include "common/dsp/automation_region.h"
#include "common/dsp/chord_track.h"
#include "common/dsp/marker_track.h"
#include "common/dsp/tracklist.h"
#include "common/utils/cairo.h"
#include "common/utils/flags.h"
#include "common/utils/gtk.h"
#include "common/utils/math.h"
#include "common/utils/ui.h"
#include "gui/cpp/backend/project.h"
#include "gui/cpp/gtk_widgets/arranger.h"
#include "gui/cpp/gtk_widgets/arranger_object.h"
#include "gui/cpp/gtk_widgets/automation_arranger.h"
#include "gui/cpp/gtk_widgets/automation_point.h"
#include "gui/cpp/gtk_widgets/bot_dock_edge.h"
#include "gui/cpp/gtk_widgets/center_dock.h"
#include "gui/cpp/gtk_widgets/chord_editor_space.h"
#include "gui/cpp/gtk_widgets/chord_object.h"
#include "gui/cpp/gtk_widgets/chord_region.h"
#include "gui/cpp/gtk_widgets/clip_editor.h"
#include "gui/cpp/gtk_widgets/clip_editor_inner.h"
#include "gui/cpp/gtk_widgets/main_notebook.h"
#include "gui/cpp/gtk_widgets/marker.h"
#include "gui/cpp/gtk_widgets/midi_arranger.h"
#include "gui/cpp/gtk_widgets/midi_editor_space.h"
#include "gui/cpp/gtk_widgets/midi_modifier_arranger.h"
#include "gui/cpp/gtk_widgets/midi_note.h"
#include "gui/cpp/gtk_widgets/piano_roll_keys.h"
#include "gui/cpp/gtk_widgets/region.h"
#include "gui/cpp/gtk_widgets/scale_object.h"
#include "gui/cpp/gtk_widgets/timeline_arranger.h"
#include "gui/cpp/gtk_widgets/timeline_panel.h"
#include "gui/cpp/gtk_widgets/track.h"
#include "gui/cpp/gtk_widgets/tracklist.h"
#include "gui/cpp/gtk_widgets/velocity.h"
#include "gui/cpp/gtk_widgets/zrythm_app.h"

#include <glib/gi18n-lib.h>

#define TYPE(x) ArrangerObject::Type::TYPE_##x

/**
 * Returns if the current position is for resizing
 * L.
 *
 * @param x X in local coordinates.
 */
bool
arranger_object_is_resize_l (const ArrangerObject * self, const int x)
{
  if (!self->has_length ())
    return false;

  if (self->full_rect_.width < UI_RESIZE_CURSOR_SPACE * 3)
    return false;

  if (x < UI_RESIZE_CURSOR_SPACE)
    {
      return true;
    }
  return false;
}

/**
 * Returns if the current position is for resizing
 * R.
 *
 * @param x X in local coordinates.
 */
bool
arranger_object_is_resize_r (const ArrangerObject * self, const int x)
{
  if (!self->has_length ())
    return false;

  if (self->full_rect_.width < UI_RESIZE_CURSOR_SPACE * 3)
    return false;

  auto lo = dynamic_cast<const LengthableObject *> (self);

  signed_frame_t size_frames = lo->end_pos_.frames_ - lo->pos_.frames_;
  Position       pos{ size_frames };
  int width_px = arranger_widget_pos_to_px (self->get_arranger (), pos, 0);

  if (x > width_px - UI_RESIZE_CURSOR_SPACE)
    {
      return true;
    }
  return false;
}

bool
arranger_object_is_fade (
  const ArrangerObject * self,
  bool                   in,
  const int              x,
  int                    y,
  bool                   only_handle,
  bool                   only_outer,
  bool                   check_lane)
{
  if (!self->can_fade ())
    return false;

  auto         fo = dynamic_cast<const FadeableObject *> (self);
  const int    fade_in_px = ui_pos_to_px_timeline (fo->fade_in_pos_, 0);
  const int    fade_out_px = ui_pos_to_px_timeline (fo->fade_out_pos_, 0);
  const int    fade_pt_halfwidth = ARRANGER_OBJECT_FADE_POINT_HALFWIDTH;
  GdkRectangle full_rect;
  auto         track = dynamic_cast<LanedTrack *> (self->get_track ());
  if (check_lane && track && track->lanes_visible_)
    {
      auto r = dynamic_cast<const Region *> (self);
      region_get_lane_full_rect (r, &full_rect);
      y -= full_rect.y - self->full_rect_.y;
    }
  else
    {
      full_rect = self->full_rect_;
    }

  bool ret = false;
  if (only_handle)
    {
      if (in)
        {
          ret =
            x >= fade_in_px - fade_pt_halfwidth
            && x <= fade_in_px + fade_pt_halfwidth && y <= fade_pt_halfwidth;
        }
      else
        {
          ret =
            x >= fade_out_px - fade_pt_halfwidth
            && x <= fade_out_px + fade_pt_halfwidth && y <= fade_pt_halfwidth;
        }
    }
  else if (only_outer)
    {
      if (in)
        {
          ret =
            x <= fade_in_px && fade_in_px > 0
            && (double) y
                 <= full_rect.height
                      * (1.0 - fade_get_y_normalized (fo->fade_in_opts_, (double) x / fade_in_px, true));
        }
      else
        {
          ret = x >= fade_out_px && full_rect.width - fade_out_px > 0 && (double) y <= full_rect.height * (1.0 - fade_get_y_normalized (fo->fade_out_opts_, (double) (x - fade_out_px) / (full_rect.width - fade_out_px), false));
        }
    }
  else
    {
      if (in)
        {
          ret = x <= fade_in_px;
        }
      else
        {
          ret = x >= fade_out_px;
        }
    }

  return ret;
}

/**
 * Returns if the current position is for resizing
 * up (eg, Velocity).
 *
 * @param x X in local coordinates.
 * @param y Y in local coordinates.
 */
bool
arranger_object_is_resize_up (
  const ArrangerObject * self,
  const int              x,
  const int              y)
{
  if (self->type_ == ArrangerObject::Type::Velocity)
    {
      if (y < VELOCITY_RESIZE_THRESHOLD)
        return 1;
    }
  else if (self->type_ == ArrangerObject::Type::AutomationPoint)
    {
      auto ap = dynamic_cast<const AutomationPoint *> (self);
      auto curve_up = ap->curves_up ();
      if (curve_up)
        {
          if (
            x > AP_WIDGET_POINT_SIZE
            || self->full_rect_.height - y > AP_WIDGET_POINT_SIZE)
            return 1;
        }
      else
        {
          if (x > AP_WIDGET_POINT_SIZE || y > AP_WIDGET_POINT_SIZE)
            return 1;
        }
    }
  return 0;
}

bool
arranger_object_is_resize_loop (
  const ArrangerObject * self,
  const int              y,
  bool                   ctrl_pressed)
{
  if (!self->has_length () || !self->can_loop ())
    return false;

  if (self->is_region ())
    {
      auto r = dynamic_cast<const Region *> (self);
      if (r->is_audio () && !ctrl_pressed)
        {
          return true;
        }

      if (r->is_looped ())
        {
          return true;
        }

      /* TODO */
      int height_px = 60;
      /*gtk_widget_get_height (*/
      /*GTK_WIDGET (self));*/

      if (y > height_px / 2)
        {
          return 1;
        }
      return 0;
    }

  return 0;
}

/**
 * Returns if the current position is for renaming
 * the object.
 *
 * @param x X in local coordinates.
 * @param y Y in local coordinates.
 */
bool
arranger_object_is_rename (const ArrangerObject * self, const int x, const int y)
{
  /* disable for now */
  return false;

  if (!self->is_region ())
    return false;

  GdkRectangle rect = self->last_name_rect_;
  /* make the clickable height area a little smaller */
  rect.height = MAX (1, rect.height - 4);
  if (ui_is_point_in_rect_hit (&rect, true, true, x, y, 0, 0))
    {
      return true;
    }

  return false;
}

bool
arranger_object_should_show_cut_lines (
  const ArrangerObject * self,
  bool                   alt_pressed)
{
  if (!self->has_length ())
    return 0;

  switch (P_TOOL)
    {
    case Tool::Select:
      if (alt_pressed)
        return 1;
      else
        return 0;
      break;
    case Tool::Cut:
      return 1;
      break;
    default:
      return 0;
      break;
    }
  z_return_val_if_reached (-1);
}

/**
 * Returns Y in pixels from the value based on the
 * allocation of the arranger.
 */
static int
get_automation_point_y (const AutomationPoint * ap, ArrangerWidget * arranger)
{
  /* ratio of current value in the range */
  float ap_ratio = ap->normalized_val_;

  int allocated_h = gtk_widget_get_height (GTK_WIDGET (arranger));
  allocated_h -= AUTOMATION_ARRANGER_VPADDING * 2;
  int point = allocated_h - (int) (ap_ratio * (float) allocated_h);
  return point + AUTOMATION_ARRANGER_VPADDING;
}

/**
 * Gets the full rectangle for a linked object.
 */
int
arranger_object_get_full_rect_x_for_region_child (
  ArrangerObject * self,
  Region          &region,
  GdkRectangle *   full_rect)
{
  /* use absolute position */
  return ui_pos_to_px_editor (
    Position{ region.pos_.ticks_ + self->pos_.ticks_ }, true);
}

void
arranger_object_set_full_rectangle (
  ArrangerObject * self,
  ArrangerWidget * arranger)
{
  z_return_if_fail (Z_IS_ARRANGER_WIDGET (arranger) && self);

  auto warn_if_has_negative_dimensions = [&self] () {
    if (self->full_rect_.x < 0)
      {
        int diff = -self->full_rect_.x;
        self->full_rect_.x = 0;
        self->full_rect_.width -= diff;
        if (self->full_rect_.width < 1)
          {
            self->full_rect_.width = 1;
          }
      }
    z_return_if_fail (
      self->full_rect_.x >= 0 && self->full_rect_.y >= 0
      && self->full_rect_.width >= 0 && self->full_rect_.height >= 0);
  };

  switch (self->type_)
    {
    case ArrangerObject::Type::ChordObject:
      {
        auto * co = dynamic_cast<ChordObject *> (self);
        auto * descr = co->get_chord_descriptor ();

        auto * region = co->get_region ();

        double   region_start_ticks = region->pos_.ticks_;
        Position tmp;
        int      adj_px_per_key =
          chord_editor_space_widget_get_chord_height (MW_CHORD_EDITOR_SPACE) + 1;

        /* use absolute position */
        tmp.from_ticks (region_start_ticks + self->pos_.ticks_);
        self->full_rect_.x = ui_pos_to_px_editor (tmp, true);
        self->full_rect_.y = adj_px_per_key * co->chord_index_;

        std::string chord_str = descr->to_string ();

        chord_object_recreate_pango_layouts (co);
        self->full_rect_.width =
          self->textw_ + CHORD_OBJECT_WIDGET_TRIANGLE_W
          + Z_CAIRO_TEXT_PADDING * 2;

        self->full_rect_.height = adj_px_per_key;

        warn_if_has_negative_dimensions ();
      }
      break;
    case ArrangerObject::Type::AutomationPoint:
      {
        auto * ap = dynamic_cast<AutomationPoint *> (self);
        auto * region = ap->get_region ();

        /* use absolute position */
        double   region_start_ticks = region->pos_.ticks_;
        Position tmp;
        tmp.from_ticks (region_start_ticks + self->pos_.ticks_);
        self->full_rect_.x =
          ui_pos_to_px_editor (tmp, true) - AP_WIDGET_POINT_SIZE / 2;

        auto * next_ap = region->get_next_ap (
          *ap, true, arranger->action == UiOverlayAction::MovingCopy);

        if (next_ap)
          {
            /* get relative position from the start AP to the next ap. */
            tmp.from_ticks (next_ap->pos_.ticks_ - self->pos_.ticks_);

            /* width is the relative position in px plus half an
             * AP_WIDGET_POINT_SIZE for each side */
            self->full_rect_.width =
              AP_WIDGET_POINT_SIZE + ui_pos_to_px_editor (tmp, false);

            z_warn_if_fail (self->full_rect_.width >= 0);

            int cur_y = get_automation_point_y (ap, arranger);
            int next_y = get_automation_point_y (next_ap, arranger);

            self->full_rect_.y = std::max (
              (cur_y > next_y ? next_y : cur_y) - AP_WIDGET_POINT_SIZE / 2, 0);

            /* make sure y is not negative */

            /* height is the relative relative diff in px between the two points
             * plus half an AP_WIDGET_POINT_SIZE for each side */
            self->full_rect_.height =
              (cur_y > next_y ? cur_y - next_y : next_y - cur_y)
              + AP_WIDGET_POINT_SIZE;
            warn_if_has_negative_dimensions ();
          }
        else
          {
            self->full_rect_.y = std::max (
              get_automation_point_y (ap, arranger) - AP_WIDGET_POINT_SIZE / 2,
              0);

            self->full_rect_.width = AP_WIDGET_POINT_SIZE;
            self->full_rect_.height = AP_WIDGET_POINT_SIZE;

            warn_if_has_negative_dimensions ();
          }
      }
      break;
    case ArrangerObject::Type::Region:
      {
        auto * region = dynamic_cast<Region *> (self);
        auto * track = self->get_track ();

        self->full_rect_.x = ui_pos_to_px_timeline (self->pos_, true);
        Position tmp;
        tmp.from_ticks (region->end_pos_.ticks_ - self->pos_.ticks_);
        self->full_rect_.width = ui_pos_to_px_timeline (tmp, false) - 1;
        if (self->full_rect_.width < 1)
          {
            self->full_rect_.width = 1;
          }

        graphene_point_t wpt = Z_GRAPHENE_POINT_INIT (0.f, 0.f);
        if (track->widget_)
          {
            graphene_point_t tmp_pt = Z_GRAPHENE_POINT_INIT (0, 0);
            bool             success = gtk_widget_compute_point (
              GTK_WIDGET (track->widget_),
              arranger->is_pinned
                            ? GTK_WIDGET (arranger)
                            : GTK_WIDGET (MW_TRACKLIST->unpinned_box),
              &tmp_pt, &wpt);
            z_return_if_fail (success);
            /* for some reason it returns a few negatives at first */
            if (wpt.y < 0.f)
              wpt.y = 0.f;
          }

        if (region->is_chord ())
          {
            auto chord_region = dynamic_cast<ChordRegion *> (region);
            chord_region_recreate_pango_layouts (chord_region);

            self->full_rect_.y = static_cast<int> (wpt.y);
            /* full height minus the space the scales would require, plus some
             * padding */
            self->full_rect_.height =
              static_cast<int> (track->main_height_)
              - (self->texth_ + Z_CAIRO_TEXT_PADDING * 4);

            warn_if_has_negative_dimensions ();
          }
        else if (region->is_automation ())
          {
            auto automation_region = dynamic_cast<AutomationRegion *> (region);
            auto * at = automation_region->get_automation_track ();
            z_return_if_fail (at);
            auto automatable_track = at->get_track ();
            if (!at->created_ || !automatable_track->automation_visible_)
              return;

            self->full_rect_.y = static_cast<int> (wpt.y) + at->y_;
            self->full_rect_.height = static_cast<int> (at->height_);

            warn_if_has_negative_dimensions ();
          }
        else
          {
            self->full_rect_.y = static_cast<int> (wpt.y);
            self->full_rect_.height = static_cast<int> (track->main_height_);

            warn_if_has_negative_dimensions ();
          }
        /* leave some space for the line below the region */
        self->full_rect_.height--;
      }
      break;
    case ArrangerObject::Type::MidiNote:
      {
        auto * mn = dynamic_cast<MidiNote *> (self);
        auto * region = mn->get_region ();
        z_return_if_fail (region);
        auto * track = dynamic_cast<PianoRollTrack *> (mn->get_track ());
        z_return_if_fail (track);

        double   region_start_ticks = region->pos_.ticks_;
        Position tmp;
        double   adj_px_per_key = MW_PIANO_ROLL_KEYS->px_per_key + 1.0;

        /* use absolute position */
        tmp.from_ticks (region_start_ticks + self->pos_.ticks_);
        self->full_rect_.x = ui_pos_to_px_editor (tmp, true);
        const auto * descr = PIANO_ROLL->find_midi_note_descriptor_by_val (
          track->drum_mode_, mn->val_);
        self->full_rect_.y =
          static_cast<int> (adj_px_per_key * (127 - descr->value_));

        self->full_rect_.height = static_cast<int> (adj_px_per_key);
        if (track->drum_mode_)
          {
            self->full_rect_.width = self->full_rect_.height;
            self->full_rect_.x -= self->full_rect_.width / 2;

            /*WARN_IF_HAS_NEGATIVE_DIMENSIONS;*/
          }
        else
          {
            /* use absolute position */
            tmp.from_ticks (region_start_ticks + mn->end_pos_.ticks_);
            self->full_rect_.width =
              ui_pos_to_px_editor (tmp, true) - self->full_rect_.x;

            warn_if_has_negative_dimensions ();
          }
      }
      break;
    case ArrangerObject::Type::ScaleObject:
      {
        auto * track = P_CHORD_TRACK;
        auto   scale_object = dynamic_cast<ScaleObject *> (self);

        graphene_point_t wpt = Z_GRAPHENE_POINT_INIT (0.f, 0.f);
        if (track->widget_)
          {
            graphene_point_t tmp_pt = Z_GRAPHENE_POINT_INIT (0, 0);
            bool             success = gtk_widget_compute_point (
              GTK_WIDGET (track->widget_), GTK_WIDGET (arranger), &tmp_pt, &wpt);
            z_return_if_fail (success);
            /* for some reason it returns a few negatives at first */
            if (wpt.y < 0.f)
              wpt.y = 0.f;
          }

        self->full_rect_.x = ui_pos_to_px_timeline (self->pos_, true);
        scale_object_recreate_pango_layouts (scale_object);
        self->full_rect_.width =
          self->textw_ + SCALE_OBJECT_WIDGET_TRIANGLE_W
          + Z_CAIRO_TEXT_PADDING * 2;

        int obj_height = self->texth_ + Z_CAIRO_TEXT_PADDING * 2;
        self->full_rect_.y =
          (static_cast<int> (wpt.y) + static_cast<int> (track->main_height_))
          - obj_height;
        self->full_rect_.height = obj_height;

        warn_if_has_negative_dimensions ();
      }
      break;
    case ArrangerObject::Type::Marker:
      {
        auto * track = P_MARKER_TRACK;
        auto * marker = dynamic_cast<Marker *> (self);

        graphene_point_t wpt = Z_GRAPHENE_POINT_INIT (0.f, 0.f);
        if (track->widget_)
          {
            graphene_point_t tmp_pt = Z_GRAPHENE_POINT_INIT (0, 0);
            bool             success = gtk_widget_compute_point (
              GTK_WIDGET (track->widget_), GTK_WIDGET (arranger), &tmp_pt, &wpt);
            z_return_if_fail (success);
            /* for some reason it returns a few
             * negatives at first */
            if (wpt.y < 0.f)
              wpt.y = 0.f;
          }

        self->full_rect_.x = ui_pos_to_px_timeline (self->pos_, true);
        marker_recreate_pango_layouts (marker);
        self->full_rect_.width =
          self->textw_ + MARKER_WIDGET_TRIANGLE_W + Z_CAIRO_TEXT_PADDING * 2;

        int global_y_start =
          static_cast<int> (wpt.y) + static_cast<int> (track->main_height_);
        int obj_height = std::min (
          static_cast<int> (track->main_height_),
          self->texth_ + Z_CAIRO_TEXT_PADDING * 2);
        self->full_rect_.y = global_y_start - obj_height;
        self->full_rect_.height = obj_height;

        warn_if_has_negative_dimensions ();
      }
      break;
    case ArrangerObject::Type::Velocity:
      {
        /* use transient or non transient note depending on which is visible */
        auto * vel = dynamic_cast<Velocity *> (self);
        auto * mn = vel->get_midi_note ();
        z_return_if_fail (mn);
        auto * region = mn->get_region ();
        z_return_if_fail (region);

        /* use absolute position */
        double   region_start_ticks = region->pos_.ticks_;
        Position tmp;
        tmp.from_ticks (region_start_ticks + mn->pos_.ticks_);
        self->full_rect_.x = ui_pos_to_px_editor (tmp, true);

        /* adjust x to start before the MIDI note so that velocity appears
         * centered at start of MIDI note */
        self->full_rect_.x -= VELOCITY_WIDTH / 2;

        int height = gtk_widget_get_height (GTK_WIDGET (arranger));

        int vel_px = static_cast<int> (
          static_cast<float> (height)
          * (static_cast<float> (vel->vel_) / 127.f));
        self->full_rect_.y = height - vel_px;

        self->full_rect_.width = VELOCITY_WIDTH;
        self->full_rect_.height = vel_px;

        warn_if_has_negative_dimensions ();

        /* adjust for circle radius */
        self->full_rect_.height += VELOCITY_WIDTH / 2;
        self->full_rect_.y -= VELOCITY_WIDTH / 2;
      }
      break;
    default:
      z_warn_if_reached ();
      break;
    }

  bool drum_mode = false;
  if (
    self->type_ == ArrangerObject::Type::MidiNote
    || self->type_ == ArrangerObject::Type::Velocity)
    {
      auto * track = dynamic_cast<PianoRollTrack *> (self->get_track ());
      z_return_if_fail (track);
      drum_mode = track->drum_mode_;
    }

  if (
    (self->full_rect_.x < 0 && self->type_ != ArrangerObject::Type::MidiNote
     && !drum_mode)
    || (self->full_rect_.y < 0 && self->type_ != ArrangerObject::Type::Velocity)
    || (self->full_rect_.y < -VELOCITY_WIDTH / 2 && self->type_ == ArrangerObject::Type::Velocity)
    || self->full_rect_.width < 0 || self->full_rect_.height < 0)
    {
      z_debug ("Object:");
      self->print ();
      z_warning (
        "The full rectangle of widget %p has negative dimensions: (%d,%d) w: %d h: %d. This should not happen. A rendering error is expected to occur.",
        fmt::ptr (self), self->full_rect_.x, self->full_rect_.y,
        self->full_rect_.width, self->full_rect_.height);
    }

  /* make sure width/height are > 0 */
  if (self->full_rect_.width < 1)
    {
      self->full_rect_.width = 1;
    }
  if (self->full_rect_.height < 1)
    {
      self->full_rect_.height = 1;
    }
}

int
arranger_object_get_draw_rectangle (
  ArrangerObject * self,
  GdkRectangle *   parent_rect,
  GdkRectangle *   full_rect,
  GdkRectangle *   draw_rect)
{
  z_return_val_if_fail (full_rect->width > 0, false);

  if (!ui_rectangle_overlap (parent_rect, full_rect))
    return 0;

  draw_rect->x = MAX (full_rect->x, parent_rect->x);
  draw_rect->width = MIN (
    (parent_rect->x + parent_rect->width) - draw_rect->x,
    (full_rect->x + full_rect->width) - draw_rect->x);
  z_warn_if_fail (draw_rect->width >= 0);
  draw_rect->y = MAX (full_rect->y, parent_rect->y);
  draw_rect->height = MIN (
    (parent_rect->y + parent_rect->height) - draw_rect->y,
    (full_rect->y + full_rect->height) - draw_rect->y);
  z_warn_if_fail (draw_rect->height >= 0);

  return 1;
  /*z_info ("full rect: (%d, {}) w: {} h: {}",*/
  /*full_rect->x, full_rect->y,*/
  /*full_rect->width, full_rect->height);*/
  /*z_info ("draw rect: (%d, {}) w: {} h: {}",*/
  /*draw_rect->x, draw_rect->y,*/
  /*draw_rect->width, draw_rect->height);*/
}

/**
 * Draws the given object.
 *
 * To be called from the arranger's draw callback.
 *
 * @param rect Rectangle in the arranger.
 */
void
arranger_object_draw (
  ArrangerObject * self,
  ArrangerWidget * arranger,
  GtkSnapshot *    snapshot,
  GdkRectangle *   rect)
{
#if 0
  if (!ui_rectangle_overlap (
        &self->full_rect, rect))
    {
      z_debug (
        "%s: object not visible, skipping",
        __func__);
    }
#endif

  class ArrangerObjectDrawVisitor
  {
  public:
    ArrangerObjectDrawVisitor (
      ArrangerWidget * arranger,
      GtkSnapshot *    snapshot,
      GdkRectangle *   rect)
        : arranger_ (arranger), snapshot_ (snapshot), rect_ (rect)
    {
    }
    void operator() (AutomationPoint * ap)
    {
      automation_point_draw (ap, snapshot_, rect_, arranger_->ap_layout.get ());
    }
    void operator() (Region * region)
    {
      region_draw (region, snapshot_, rect_);
    }
    void operator() (MidiNote * note) { midi_note_draw (note, snapshot_); }
    void operator() (Marker * marker) { marker_draw (marker, snapshot_); }
    void operator() (ScaleObject * scale)
    {
      scale_object_draw (scale, snapshot_);
    }
    void operator() (ChordObject * chord)
    {
      chord_object_draw (chord, snapshot_);
    }
    void operator() (Velocity * velocity)
    {
      velocity_draw (velocity, snapshot_);
    }

  private:
    ArrangerWidget * arranger_;
    GtkSnapshot *    snapshot_;
    GdkRectangle *   rect_;
  };
  auto variant = convert_to_variant<ArrangerObjectPtrVariant> (self);
  std::visit (ArrangerObjectDrawVisitor (arranger, snapshot, rect), variant);
}

bool
arranger_object_should_orig_be_visible (
  const ArrangerObject  &self,
  const ArrangerWidget * arranger)
{
  if (!ZRYTHM_HAVE_UI)
    {
      return false;
    }

  if (!arranger)
    {
      arranger = self.get_arranger ();
      z_return_val_if_fail (arranger, false);
    }

  /* check trans/non-trans visibility */
  auto action = arranger->action;
  if (
    action == UiOverlayAction::MOVING
    || action == UiOverlayAction::CREATING_MOVING)
    {
      return false;
    }
  else if (
    action == UiOverlayAction::MovingCopy
    || action == UiOverlayAction::MOVING_LINK)
    {
      return true;
    }
  else
    {
      return false;
    }
}

bool
arranger_object_is_hovered (
  const ArrangerObject * self,
  const ArrangerWidget * arranger)
{
  if (!arranger)
    {
      arranger = self->get_arranger ();
    }
  if (auto obj = arranger->hovered_object.lock ())
    {
      if (obj.get () == self)
        return true;
    }
  return false;
}

bool
arranger_object_is_hovered_or_start_object (
  const ArrangerObject * self,
  const ArrangerWidget * arranger)
{
  if (!arranger)
    {
      arranger = self->get_arranger ();
    }
  if (auto obj = arranger->hovered_object.lock ())
    {
      if (obj.get () == self)
        return true;
    }
  return arranger->start_object.get () == self;
}
