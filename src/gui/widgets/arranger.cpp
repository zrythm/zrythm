// SPDX-FileCopyrightText: © 2018-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-FileCopyrightText: © 2024 Miró Allard <miro.allard@pm.me>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense
/*
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 * ---
 *
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * Modified by the GTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GTK+ at ftp://ftp.gtk.org/pub/gtk/.
 */

#include "actions/arranger_selections.h"
#include "dsp/automation_region.h"
#include "dsp/automation_track.h"
#include "dsp/channel.h"
#include "dsp/chord_region.h"
#include "dsp/chord_track.h"
#include "dsp/control_port.h"
#include "dsp/marker.h"
#include "dsp/marker_track.h"
#include "dsp/midi_region.h"
#include "dsp/piano_roll_track.h"
#include "dsp/track.h"
#include "dsp/tracklist.h"
#include "dsp/transport.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/arranger_draw.h"
#include "gui/widgets/arranger_wrapper.h"
#include "gui/widgets/audio_arranger.h"
#include "gui/widgets/audio_editor_space.h"
#include "gui/widgets/automation_arranger.h"
#include "gui/widgets/automation_editor_space.h"
#include "gui/widgets/automation_point.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/chord_arranger.h"
#include "gui/widgets/chord_editor_space.h"
#include "gui/widgets/clip_editor.h"
#include "gui/widgets/clip_editor_inner.h"
#include "gui/widgets/color_area.h"
#include "gui/widgets/dialogs/string_entry_dialog.h"
#include "gui/widgets/editor_ruler.h"
#include "gui/widgets/main_notebook.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/midi_arranger.h"
#include "gui/widgets/midi_editor_space.h"
#include "gui/widgets/midi_modifier_arranger.h"
#include "gui/widgets/piano_roll_keys.h"
#include "gui/widgets/region.h"
#include "gui/widgets/ruler.h"
#include "gui/widgets/scale_selector_window.h"
#include "gui/widgets/timeline_arranger.h"
#include "gui/widgets/timeline_bg.h"
#include "gui/widgets/timeline_panel.h"
#include "gui/widgets/timeline_ruler.h"
#include "gui/widgets/track.h"
#include "gui/widgets/tracklist.h"
#include "project.h"
#include "settings/g_settings_manager.h"
#include "utils/cairo.h"
#include "utils/flags.h"
#include "utils/gtk.h"
#include "utils/math.h"
#include "utils/objects.h"
#include "utils/rt_thread_id.h"
#include "utils/ui.h"
#include "zrythm.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>

#include "gtk_wrapper.h"

G_DEFINE_TYPE (ArrangerWidget, arranger_widget, GTK_TYPE_WIDGET)

#define TYPE_IS(x) (self->type == ArrangerWidgetType::x)

constexpr int SCROLL_PADDING = 8;

std::pair<ArrangerObject::ResizeType, bool>
arranger_widget_get_resize_type_and_direction_from_action (
  UiOverlayAction action)
{
  switch (action)
    {
    case UiOverlayAction::ResizingL:
      return std::make_pair (ArrangerObject::ResizeType::Normal, true);
    case UiOverlayAction::CreatingResizingR:
    case UiOverlayAction::ResizingR:
      return std::make_pair (ArrangerObject::ResizeType::Normal, false);
    case UiOverlayAction::ResizingLLoop:
      return std::make_pair (ArrangerObject::ResizeType::Loop, true);
    case UiOverlayAction::ResizingRLoop:
      return std::make_pair (ArrangerObject::ResizeType::Loop, false);
    case UiOverlayAction::ResizingLFade:
      return std::make_pair (ArrangerObject::ResizeType::Fade, true);
    case UiOverlayAction::ResizingRFade:
      return std::make_pair (ArrangerObject::ResizeType::Fade, false);
    case UiOverlayAction::StretchingL:
      return std::make_pair (ArrangerObject::ResizeType::Stretch, true);
    case UiOverlayAction::StretchingR:
      return std::make_pair (ArrangerObject::ResizeType::Stretch, false);
    default:
      z_return_val_if_reached (
        std::make_pair (ArrangerObject::ResizeType::Normal, true));
    }
}

/**
 * Snaps the region's start or end point.
 *
 * @param pos Position to snap.
 * @param dry_run Don't resize notes; just check if the resize is allowed (check
 * if invalid resizes will happen)
 *
 * @return Whether successful.
 */
template <FinalArrangerObjectSubclass ObjT>
bool
snap_obj_during_resize (
  ArrangerWidget * self,
  ObjT            &obj_to_resize,
  Position        &pos,
  bool             dry_run)
{
  auto [resize_type, left] =
    arranger_widget_get_resize_type_and_direction_from_action (self->action);

  Region * owner_region = nullptr;
  Position global_pos_to_snap = pos;
  if constexpr (std::derived_from<ObjT, RegionOwnedObject>)
    {
      owner_region = obj_to_resize.get_region ();
      global_pos_to_snap.add_ticks (owner_region->pos_.ticks_);
    }
  else
    {
      if (!pos.is_positive ())
        return false;
    }

  if (
    self->snap_grid->any_snap () && !self->shift_held
    && resize_type != ArrangerObject::ResizeType::Fade
    && global_pos_to_snap.is_positive ())
    {
      Track * track = nullptr;
      if constexpr (!RegionOwnedObjectSubclass<ObjT>)
        {
          track = obj_to_resize.get_track ();
          z_return_val_if_fail (track, false);
        }
      z_return_val_if_fail (self->earliest_obj_start_pos, false);
      global_pos_to_snap.snap (
        self->earliest_obj_start_pos.get (), track, owner_region,
        *self->snap_grid);
    }

  // convert global pos back into output pos
  if constexpr (RegionOwnedObjectSubclass<ObjT>)
    {
      global_pos_to_snap.add_ticks (-owner_region->pos_.ticks_);
    }
  pos = global_pos_to_snap;

  if (resize_type == ArrangerObject::ResizeType::Fade)
    {
      if constexpr (std::derived_from<ObjT, FadeableObject>)
        {
          if (left)
            {
              if (pos >= obj_to_resize.fade_out_pos_)
                return false;
            }
          else
            {
              Position tmp{
                obj_to_resize.end_pos_.ticks_ - obj_to_resize.pos_.ticks_
              };
              if (pos <= obj_to_resize.fade_in_pos_ || pos > tmp)
                return false;
            }
        }
      else
        {
          z_return_val_if_reached (false);
        }
    }
  else
    {
      if (left)
        {
          if (pos >= obj_to_resize.end_pos_)
            return false;
        }
      else
        {
          if (pos <= obj_to_resize.pos_)
            return false;
        }
    }

  bool   is_valid = false;
  double diff = 0.0;

  if (resize_type == ArrangerObject::ResizeType::Fade)
    {
      if constexpr (std::derived_from<ObjT, FadeableObject>)
        {
          if (left)
            {
              is_valid = obj_to_resize.is_position_valid (
                pos, ArrangerObject::PositionType::FadeIn);
              diff = pos.ticks_ - obj_to_resize.fade_in_pos_.ticks_;
            }
          else
            {
              is_valid = obj_to_resize.is_position_valid (
                pos, ArrangerObject::PositionType::FadeOut);
              diff = pos.ticks_ - obj_to_resize.fade_out_pos_.ticks_;
            }
        }
      else
        {
          z_return_val_if_reached (false);
        }
    }
  else
    {
      if (left)
        {
          is_valid = obj_to_resize.is_position_valid (
            pos, ArrangerObject::PositionType::Start);
          diff = pos.ticks_ - obj_to_resize.pos_.ticks_;
        }
      else
        {
          is_valid = obj_to_resize.is_position_valid (
            pos, ArrangerObject::PositionType::End);
          diff = pos.ticks_ - obj_to_resize.end_pos_.ticks_;
        }
    }

  if (is_valid)
    {
      if (!dry_run)
        {
          try
            {
              obj_to_resize.resize (left, resize_type, diff, true);
            }
          catch (const ZrythmException &e)
            {
              e.handle ("Failed to resize object");
              return false;
            }

          if (self->action == UiOverlayAction::CreatingResizingR)
            {
              if constexpr (std::derived_from<ObjT, LoopableObject>)
                {
                  double   full_size = obj_to_resize.get_length_in_ticks ();
                  Position tmp = obj_to_resize.loop_start_pos_;
                  tmp.add_ticks (full_size);
                  obj_to_resize.loop_end_pos_setter (&tmp);
                }
            }
          return true;
        }
      else
        {
          return true;
        }
    }
  else
    {
      return false;
    }
}

static bool
snap_selections_during_resize (ArrangerWidget * self, Position * pos, bool dry_run)
{
  auto shared_prj_obj = self->prj_start_object.lock ();
  z_return_val_if_fail (shared_prj_obj, false);

  return std::visit (
    [&] (auto &&obj) {
      using ObjT = base_type<decltype (obj)>;

      Region * owner_region = nullptr;
      if constexpr (std::derived_from<ObjT, RegionOwnedObject>)
        {
          owner_region = obj->get_region ();
          z_return_val_if_fail (owner_region, false);
        }

      auto [resize_type, left] =
        arranger_widget_get_resize_type_and_direction_from_action (self->action);
      double delta_ticks = 0.0;
      if (resize_type == ArrangerObject::ResizeType::Fade)
        {
          if constexpr (std::derived_from<ObjT, FadeableObject>)
            {
              const auto &pos_to_add =
                left ? obj->fade_in_pos_ : obj->fade_out_pos_;
              delta_ticks = pos->ticks_ - (obj->pos_.ticks_ + pos_to_add.ticks_);
            }
          else
            {
              z_return_val_if_reached (false);
            }
        }
      else
        {
          const auto &pos_to_add = left ? obj->pos_ : obj->end_pos_;
          if constexpr (std::derived_from<ObjT, RegionOwnedObject>)
            {
              delta_ticks =
                pos->ticks_ - (pos_to_add.ticks_ + owner_region->pos_.ticks_);
            }
          else
            {
              delta_ticks = pos->ticks_ - pos_to_add.ticks_;
            }
        }

      auto selections = arranger_widget_get_selections (self);

      for (auto cur_obj : selections->objects_ | type_is<ObjT> ())
        {
          Position new_pos;
          if (self->action == UiOverlayAction::ResizingLFade)
            {
              if constexpr (std::derived_from<ObjT, FadeableObject>)
                {
                  new_pos =
                    left ? cur_obj->fade_in_pos_ : cur_obj->fade_out_pos_;
                }
              else
                {
                  z_return_val_if_reached (false);
                }
            }
          else
            {
              new_pos = left ? cur_obj->pos_ : cur_obj->end_pos_;
            }
          new_pos.add_ticks (delta_ticks);

          bool successful =
            snap_obj_during_resize (self, *cur_obj, new_pos, dry_run);

          if (!successful)
            return false;
        }

      if (!dry_run)
        {
          EVENTS_PUSH (EventType::ET_ARRANGER_SELECTIONS_CHANGED, selections);
        }

      return true;
    },
    convert_to_variant<LengthableObjectPtrVariant> (shared_prj_obj.get ()));
}

/**
 * Returns if the arranger can scroll vertically.
 */
bool
arranger_widget_can_scroll_vertically (ArrangerWidget * self)
{
  if (
    (TYPE_IS (Timeline) && self->is_pinned) || TYPE_IS (MidiModifier)
    || TYPE_IS (Audio) || TYPE_IS (Automation))
    return false;

  return true;
}

std::unique_ptr<EditorSettings>
arranger_widget_get_editor_setting_values (ArrangerWidget * self)
{
  auto cur_settings = arranger_widget_get_editor_settings (self);
  auto ret = std::visit (
    [] (auto &&arg) {
      return static_cast<std::unique_ptr<EditorSettings>> (arg->clone_unique ());
    },
    cur_settings);
  if (!arranger_widget_can_scroll_vertically (self))
    {
      ret->scroll_start_y_ = 0;
    }

  return ret;
}

static void
drag_end (
  GtkGestureDrag * gesture,
  gdouble          offset_x,
  gdouble          offset_y,
  ArrangerWidget * self);

const char *
arranger_widget_get_type_str (ArrangerWidgetType type)
{
  static const char * arranger_widget_type_str[] = {
    "timeline", "midi", "midi modifier", "audio", "chord", "automation",
  };
  return arranger_widget_type_str[static_cast<int> (type)];
}

bool
arranger_widget_get_drum_mode_enabled (ArrangerWidget * self)
{
  if (self->type != ArrangerWidgetType::Midi)
    return false;

  if (!CLIP_EDITOR->has_region_)
    return false;

  auto tr = dynamic_cast<PianoRollTrack *> (CLIP_EDITOR->get_track ());
  if (!tr)
    return false;

  return tr->drum_mode_;
}

int
arranger_widget_get_playhead_px (ArrangerWidget * self)
{
  RulerWidget * ruler = arranger_widget_get_ruler (self);
  return ruler_widget_get_playhead_px (ruler, false);
}

void
arranger_widget_set_cursor (ArrangerWidget * self, ArrangerCursor cursor)
{
#define SET_X_CURSOR(x) ui_set_##x##_cursor (GTK_WIDGET (self));

#define SET_CURSOR_FROM_NAME(name) \
  ui_set_cursor_from_name (GTK_WIDGET (self), name);

  switch (cursor)
    {
    case ArrangerCursor::Select:
      SET_X_CURSOR (pointer);
      break;
    case ArrangerCursor::Edit:
      SET_X_CURSOR (pencil);
      break;
    case ArrangerCursor::Autofill:
      SET_X_CURSOR (brush);
      break;
    case ArrangerCursor::ARRANGER_CURSOR_CUT:
      SET_X_CURSOR (cut_clip);
      break;
    case ArrangerCursor::Ramp:
      SET_X_CURSOR (line);
      break;
    case ArrangerCursor::Eraser:
      SET_X_CURSOR (eraser);
      break;
    case ArrangerCursor::Audition:
      SET_X_CURSOR (speaker);
      break;
    case ArrangerCursor::Grab:
      SET_X_CURSOR (hand);
      break;
    case ArrangerCursor::Grabbing:
      SET_CURSOR_FROM_NAME ("grabbing");
      break;
    case ArrangerCursor::GrabbingCopy:
      SET_CURSOR_FROM_NAME ("copy");
      break;
    case ArrangerCursor::GrabbingLink:
      SET_CURSOR_FROM_NAME ("link");
      break;
    case ArrangerCursor::ResizingL:
    case ArrangerCursor::ResizingLFade:
      SET_X_CURSOR (left_resize);
      break;
    case ArrangerCursor::ARRANGER_CURSOR_STRETCHING_L:
      SET_X_CURSOR (left_stretch);
      break;
    case ArrangerCursor::ARRANGER_CURSOR_RESIZING_L_LOOP:
      SET_X_CURSOR (left_resize_loop);
      break;
    case ArrangerCursor::ResizingR:
    case ArrangerCursor::ARRANGER_CURSOR_RESIZING_R_FADE:
      SET_X_CURSOR (right_resize);
      break;
    case ArrangerCursor::ARRANGER_CURSOR_STRETCHING_R:
      SET_X_CURSOR (right_stretch);
      break;
    case ArrangerCursor::ARRANGER_CURSOR_RESIZING_R_LOOP:
      SET_X_CURSOR (right_resize_loop);
      break;
    case ArrangerCursor::ResizingUp:
      SET_CURSOR_FROM_NAME ("n-resize");
      break;
    case ArrangerCursor::ResizingUpFadeIn:
      SET_CURSOR_FROM_NAME ("n-resize");
      break;
    case ArrangerCursor::ResizingUpFadeOut:
      SET_CURSOR_FROM_NAME ("n-resize");
      break;
    case ArrangerCursor::Range:
      SET_X_CURSOR (time_select);
      break;
    case ArrangerCursor::FadeIn:
      SET_X_CURSOR (fade_in);
      break;
    case ArrangerCursor::FadeOut:
      SET_X_CURSOR (fade_out);
      break;
    case ArrangerCursor::Rename:
      SET_CURSOR_FROM_NAME ("text");
      break;
    case ArrangerCursor::Panning:
      SET_CURSOR_FROM_NAME ("all-scroll");
      break;
    default:
      z_warn_if_reached ();
      break;
    }
}

bool
arranger_widget_is_cursor_in_top_half (ArrangerWidget * self, double y)
{
  int height = gtk_widget_get_height (GTK_WIDGET (self));
  return y < ((double) height / 2.0);
}

/**
 * Sets whether selecting objects or range.
 */
static void
set_select_type (ArrangerWidget * self, double y)
{
  if (self->type == ArrangerWidgetType::Timeline)
    {
      timeline_arranger_widget_set_select_type (self, y);
    }
  else if (self->type == ArrangerWidgetType::Audio)
    {
      if (arranger_widget_is_cursor_in_top_half (self, y))
        {
          self->resizing_range = false;
        }
      else
        {
          self->resizing_range = true;
          self->resizing_range_start = true;
          self->action = UiOverlayAction::ResizingR;
        }
    }
}

SnapGrid *
arranger_widget_get_snap_grid (ArrangerWidget * self)
{
  if (
    self == MW_MIDI_MODIFIER_ARRANGER || self == MW_MIDI_ARRANGER
    || self == MW_AUTOMATION_ARRANGER || self == MW_AUDIO_ARRANGER
    || self == MW_CHORD_ARRANGER)
    {
      return SNAP_GRID_EDITOR.get ();
    }
  else if (self == MW_TIMELINE || self == MW_PINNED_TIMELINE)
    {
      return SNAP_GRID_TIMELINE.get ();
    }
  z_return_val_if_reached (nullptr);
}

class ObjectOverlapInfo
{
public:
  ObjectOverlapInfo (
    GdkRectangle *                 rect,
    double                         x,
    double                         y,
    std::vector<ArrangerObject *> &objects)
      : rect_ (rect), x_ (x), y_ (y), objects_ (objects)
  {
  }

  /**
   * When rect is nullptr, this is a special case for automation points. The
   * object will only be added if the cursor is on the automation point or
   * within n px from the curve.
   */
  GdkRectangle * rect_ = nullptr;

  /** X, or -1 to not check x. */
  double x_ = -1;

  /** Y, or -1 to not check y. */
  double y_ = -1;

  /** Position for x or rect->x (cached). */
  Position start_pos_;

  /**
   * Position for rect->x + rect->width.
   *
   * If rect is nullptr, this is the same as @ref start_pos.
   */
  Position end_pos_;

  /**
   * @brief ???
   *
   */
  std::vector<ArrangerObject *> &objects_;

  /**
   * @brief ???
   *
   */
  ArrangerObject * obj_ = nullptr;
};

/**
 * Adds the object to the array if it or its transient overlaps with the
 * rectangle, or with @ref x @ref y if @ref rect is NULL.
 *
 * @return Whether the object was added or not.
 */
ATTR_HOT static bool
add_object_if_overlap (ArrangerWidget * self, ObjectOverlapInfo &nfo)
{
  auto   rect = nfo.rect_;
  double x = nfo.x_;
  double y = nfo.y_;
  auto   obj = nfo.obj_;
  auto   ruler = arranger_widget_get_ruler (self);

  z_return_val_if_fail (obj, false);
  z_return_val_if_fail (
    (math_doubles_equal (x, -1) || x >= 0.0)
      && (math_doubles_equal (y, -1) || y >= 0.0),
    false);

  if (obj->deleted_temporarily_)
    {
      return false;
    }

  /* --- optimization to skip expensive calculations for most objects --- */

  bool orig_visible =
    arranger_object_should_orig_be_visible (*obj, self) && obj->transient_;

  /* skip objects that end before the rect */
  if (obj->has_length ())
    {
      auto       lobj = dynamic_cast<LengthableObject *> (obj);
      const auto lobj_trans = obj->get_transient<LengthableObject> ();
      Position   g_obj_end_pos;
      Position   g_obj_end_pos_transient;
      if (ArrangerObject::type_has_global_pos (obj->type_))
        {
          g_obj_end_pos = lobj->end_pos_;
          if (orig_visible)
            g_obj_end_pos_transient = lobj_trans->end_pos_;
        }
      else if (obj->owned_by_region ())
        {
          std::visit (
            [&] (auto &&ro_obj) {
              auto r = ro_obj->get_region ();
              z_return_if_fail (r);
              g_obj_end_pos = r->pos_;
              g_obj_end_pos.add_ticks (lobj->end_pos_.ticks_);
              if (orig_visible)
                {
                  g_obj_end_pos_transient = r->pos_;
                  g_obj_end_pos_transient.add_ticks (
                    lobj_trans->end_pos_.ticks_);
                }
            },
            convert_to_variant<RegionOwnedObjectPtrVariant> (obj));
        }
      if (g_obj_end_pos < nfo.start_pos_)
        {
          if (orig_visible)
            {
              if (g_obj_end_pos_transient < nfo.start_pos_)
                return false;
            }
          else
            return false;
        }
    }

  /* skip objects that start a few pixels after the end */
  Position g_obj_start_pos;
  Position g_obj_start_pos_transient;
  if (ArrangerObject::type_has_global_pos (obj->type_))
    {
      g_obj_start_pos = obj->pos_;
      if (orig_visible)
        g_obj_start_pos_transient = obj->transient_->pos_;
    }
  else if (obj->owned_by_region ())
    {
      std::visit (
        [&] (auto &&ro_obj) {
          auto r = ro_obj->get_region ();
          z_return_if_fail (r);
          g_obj_start_pos = r->pos_;
          g_obj_start_pos.add_ticks (obj->pos_.ticks_);
          if (orig_visible)
            {
              g_obj_start_pos_transient = r->pos_;
              g_obj_start_pos_transient.add_ticks (obj->transient_->pos_.ticks_);
            }
        },
        convert_to_variant<RegionOwnedObjectPtrVariant> (obj));
    }
  const double ticks_to_add = -12.0 / ruler->px_per_tick;
  g_obj_start_pos.add_ticks (ticks_to_add);
  if (orig_visible)
    {
      g_obj_start_pos_transient.add_ticks (ticks_to_add);
    }
  if (g_obj_start_pos > nfo.end_pos_)
    {
      if (orig_visible)
        {
          if (g_obj_start_pos_transient > nfo.end_pos_)
            return false;
        }
      else
        return false;
    }

  /* --- end optimization --- */

  bool is_same_arranger = obj->get_arranger () == self;
  if (!is_same_arranger)
    return false;

  arranger_object_set_full_rectangle (obj, self);
  bool add = false;
  if (rect)
    {
      if (
        (ui_rectangle_overlap (&obj->full_rect_, rect) ||
         /* also check original (transient) */
         (orig_visible && obj->transient_
          && ui_rectangle_overlap (&obj->transient_->full_rect_, rect))))
        {
          add = true;
        }
    }
  else if (
    (ui_is_point_in_rect_hit (
       &obj->full_rect_, x >= 0 ? true : false, y >= 0 ? true : false, x, y, 0, 0)
     ||
     /* also check original (transient) */
     (orig_visible
      && ui_is_point_in_rect_hit (
        &obj->transient_->full_rect_, x >= 0 ? true : false,
        y >= 0 ? true : false, x, y, 0, 0))))
    {
      if (obj->type_ == ArrangerObject::Type::AutomationPoint)
        {
          /* object to check for automation point curve cross (either main
           * object or transient) */
          std::shared_ptr<AutomationPoint> ap_to_check = nullptr;
          if (
            orig_visible
            && ui_is_point_in_rect_hit (
              &obj->transient_->full_rect_, x >= 0 ? true : false,
              y >= 0 ? true : false, x, y, 0, 0))
            {
              ap_to_check = obj->get_transient<AutomationPoint> ();
            }
          else
            {
              ap_to_check = obj->shared_from_this_as<AutomationPoint> ();
            }

          /* handle special case for automation points */
          if (
            ap_to_check && !automation_point_is_point_hit (*ap_to_check, x, y)
            && !automation_point_is_curve_hit (*ap_to_check, x, y, 16.0))
            {
              return false;
            }
        } /* endif automation point */

      add = true;
    }

  if (add)
    {
      nfo.objects_.push_back (obj);
    }

  return add;
}

/**
 * Fills in the given array with the ArrangerObject's of the given type that
 * appear in the given rect, or at the given coords if @ref rect is NULL.
 *
 * @param rect The rectangle to search in.
 * @param type The type of arranger objects to find, or -1 to look for all
 * objects.
 * @param x X, or -1 to not check x.
 * @param y Y, or -1 to not check y.
 */
static void
get_hit_objects (
  ArrangerWidget *               self,
  ArrangerObject::Type           type,
  GdkRectangle *                 rect,
  double                         x,
  double                         y,
  std::vector<ArrangerObject *> &arr)
{
  z_return_if_fail (self);

  if (!math_doubles_equal (x, -1) && x < 0.0)
    {
      z_error ("invalid x: {:f}", x);
      return;
    }
  if (!math_doubles_equal (y, -1) && y < 0.0)
    {
      z_error ("invalid y: {:f}", y);
      return;
    }

  // ArrangerObject * obj = NULL;

  /* skip if haven't drawn yet */
  if (self->first_draw)
    {
      return;
    }

  int start_y = rect ? rect->y : (int) y;

  /* prepare struct to pass for each object */
  ObjectOverlapInfo nfo (rect, x, y, arr);
  nfo.rect_ = rect;
  nfo.x_ = x;
  nfo.y_ = y;
  nfo.start_pos_ = arranger_widget_px_to_pos (self, rect ? rect->x : x, true);
  if (rect)
    {
      nfo.end_pos_ =
        arranger_widget_px_to_pos (self, rect->x + rect->width, true);
    }
  else
    {
      nfo.end_pos_ = nfo.start_pos_;
    }
  nfo.objects_ = arr;

  auto clip_editor_region = CLIP_EDITOR->get_region ();

  switch (self->type)
    {
    case ArrangerWidgetType::Timeline:
      if (
        type != ArrangerObject::Type::All && type != ArrangerObject::Type::Region
        && type != ArrangerObject::Type::Marker
        && type != ArrangerObject::Type::ScaleObject)
        break;

      /* add overlapping regions */
      if (
        type == ArrangerObject::Type::All
        || type == ArrangerObject::Type::Region)
        {
          /* midi and audio regions */
          for (const auto &track : TRACKLIST->tracks_)
            {
              /* skip tracks if not visible or this is timeline and pin status
               * doesn't match */
              if (
                !track->visible_
                || (TYPE_IS (Timeline) && track->is_pinned () != self->is_pinned))
                {
                  continue;
                }

              /* skip if track should not be visible */
              if (!track->should_be_visible ())
                continue;

              if (G_LIKELY (track->widget_))
                {
                  int track_y =
                    track_widget_get_local_y (track->widget_, self, start_y);

                  /* skip if track starts after the
                   * rect */
                  if (track_y + (rect ? rect->height : 0) < 0)
                    {
                      continue;
                    }

                  double full_track_height = track->get_full_visible_height ();

                  /* skip if track ends before the rect */
                  if (track_y > full_track_height)
                    {
                      continue;
                    }
                }

              if (track->has_lanes ())
                {
                  std::visit (
                    [&] (const auto laned_track) {
                      for (const auto &lane : laned_track->lanes_)
                        {
                          for (const auto &r : lane->regions_)
                            {
                              nfo.obj_ = r.get ();
                              bool ret = add_object_if_overlap (self, nfo);
                              if (!ret)
                                {
                                  /* check lanes */
                                  if (!laned_track->lanes_visible_)
                                    continue;
                                  GdkRectangle lane_rect;
                                  region_get_lane_full_rect (
                                    r.get (), &lane_rect);
                                  if (
                                    ((rect
                                      && ui_rectangle_overlap (&lane_rect, rect))
                                     || (!rect && ui_is_point_in_rect_hit (&lane_rect, true, true, x, y, 0, 0)))
                                    && r->get_arranger () == self
                                    && !r->deleted_temporarily_)
                                    {
                                      arr.push_back (r.get ());
                                    }
                                }
                            }
                        }
                    },
                    convert_to_variant<LanedTrackPtrVariant> (track.get ()));
                }

              /* chord regions */
              if (track->is_chord ())
                {
                  const auto chord_track =
                    dynamic_cast<ChordTrack *> (track.get ());
                  for (auto &cr : chord_track->regions_)
                    {
                      nfo.obj_ = cr.get ();
                      add_object_if_overlap (self, nfo);
                    }
                }

              /* automation regions */
              if (track->is_automatable ())
                {
                  auto * automatable_track =
                    dynamic_cast<AutomatableTrack *> (track.get ());
                  auto &atl = automatable_track->get_automation_tracklist ();
                  if (automatable_track->automation_visible_)
                    {
                      for (auto &at : atl.visible_ats_)
                        {
                          for (auto &atr : at->regions_)
                            {
                              nfo.obj_ = atr.get ();
                              add_object_if_overlap (self, nfo);
                            }
                        }
                    }
                }
            }
        }

      /* add overlapping scales */
      if (
        (type == ArrangerObject::Type::All
         || type == ArrangerObject::Type::ScaleObject)
        && P_CHORD_TRACK->should_be_visible ())
        {
          for (const auto &scale : P_CHORD_TRACK->scales_)
            {
              nfo.obj_ = scale.get ();
              add_object_if_overlap (self, nfo);
            }
        }

      /* add overlapping markers */
      if (
        (type == ArrangerObject::Type::All
         || type == ArrangerObject::Type::Marker)
        && P_MARKER_TRACK->should_be_visible ())
        {
          for (const auto &marker : P_MARKER_TRACK->markers_)
            {
              nfo.obj_ = marker.get ();
              add_object_if_overlap (self, nfo);
            }
        }
      break;
    case ArrangerWidgetType::Midi:
      /* add overlapping midi notes */
      if (
        type == ArrangerObject::Type::All
        || type == ArrangerObject::Type::MidiNote)
        {
          auto r = dynamic_cast<MidiRegion *> (clip_editor_region);
          if (!r)
            break;

          /* add main region notes */
          for (const auto &mn : r->midi_notes_)
            {
              nfo.obj_ = mn.get ();
              add_object_if_overlap (self, nfo);
            }

          if (g_settings_get_boolean (S_UI, "ghost-notes"))
            {
              /* add other region notes for same track (ghosted) */
              auto track = r->get_track_as<LanedTrackImpl<MidiRegion>> ();
              z_return_if_fail (track);
              for (const auto &lane : track->lanes_)
                {
                  for (const auto &cur_r : lane->regions_)
                    {
                      if (cur_r.get () == r)
                        continue;

                      for (const auto &mn : cur_r->midi_notes_)
                        {
                          nfo.obj_ = mn.get ();
                          add_object_if_overlap (self, nfo);
                        }
                    }
                }
            }
        }
      break;
    case ArrangerWidgetType::MidiModifier:
      /* add overlapping midi notes */
      if (
        type == ArrangerObject::Type::All
        || type == ArrangerObject::Type::Velocity)
        {
          auto r = dynamic_cast<MidiRegion *> (clip_editor_region);
          if (!r)
            break;

          for (auto &midi_note : r->midi_notes_)
            {
              Velocity * vel = midi_note->vel_.get ();
              nfo.obj_ = vel;
              add_object_if_overlap (self, nfo);
            }
        }
      break;
    case ArrangerWidgetType::Chord:
      /* add overlapping chord objects */
      if (
        type == ArrangerObject::Type::All
        || type == ArrangerObject::Type::ChordObject)
        {
          auto r = dynamic_cast<ChordRegion *> (clip_editor_region);
          if (!r)
            break;

          for (auto &co : r->chord_objects_)
            {
              z_return_if_fail (
                co->chord_index_ < (int) CHORD_EDITOR->chords_.size ());
              nfo.obj_ = co.get ();
              add_object_if_overlap (self, nfo);
            }
        }
      break;
    case ArrangerWidgetType::Automation:
      /* add overlapping midi notes */
      if (
        type == ArrangerObject::Type::All
        || type == ArrangerObject::Type::AutomationPoint)
        {
          auto r = dynamic_cast<AutomationRegion *> (clip_editor_region);
          if (!r)
            break;

          for (const auto &ap : r->aps_)
            {
              nfo.obj_ = ap.get ();
              add_object_if_overlap (self, nfo);
            }
        }
      break;
    case ArrangerWidgetType::Audio:
      /* no objects in audio arranger yet */
      break;
    default:
      z_warn_if_reached ();
      break;
    }
}

void
arranger_widget_get_hit_objects_in_rect (
  ArrangerWidget *               self,
  ArrangerObject::Type           type,
  GdkRectangle *                 rect,
  std::vector<ArrangerObject *> &arr)
{
  get_hit_objects (self, type, rect, 0, 0, arr);
}

/**
 * Filters out objects from frozen tracks.
 */
static void
filter_out_frozen_objects (
  ArrangerWidget *               self,
  std::vector<ArrangerObject *> &objs_arr)
{
  if (self->type != ArrangerWidgetType::Timeline)
    {
      return;
    }

  auto removed = std::erase_if (objs_arr, [=] (const ArrangerObject * obj) {
    auto track = obj->get_track ();
    z_return_val_if_fail (track, true);
    return track->frozen_;
  });

  z_debug ("removed {} frozen objects", removed);
}

void
arranger_widget_get_hit_objects_at_point (
  ArrangerWidget *               self,
  ArrangerObject::Type           type,
  double                         x,
  double                         y,
  std::vector<ArrangerObject *> &arr)
{
  get_hit_objects (self, type, nullptr, x, y, arr);
}

bool
arranger_widget_is_in_moving_operation (ArrangerWidget * self)
{
  if (
    self->action == UiOverlayAction::STARTING_MOVING
    || self->action == UiOverlayAction::STARTING_MOVING_COPY
    || self->action == UiOverlayAction::STARTING_MOVING_LINK
    || self->action == UiOverlayAction::MOVING
    || self->action == UiOverlayAction::MovingCopy
    || self->action == UiOverlayAction::MOVING_LINK)
    return true;

  return false;
}

/**
 * Moves the ArrangerSelections by the given amount of ticks.
 *
 * @param ticks_diff Ticks to move by.
 * @param copy_moving 1 if copy-moving.
 */
static void
move_items_x (ArrangerWidget * self, const double ticks_diff)
{
  ArrangerSelections * sel = arranger_widget_get_selections (self);
  z_return_if_fail (sel);

  EVENTS_PUSH_NOW (EventType::ET_ARRANGER_SELECTIONS_IN_TRANSIT, sel);

  sel->add_ticks (ticks_diff);
  /*z_debug ("adding {:f} ticks to selections", ticks_diff);*/

  if (sel->is_automation ())
    {
      /* re-sort the automation region */
      auto region = CLIP_EDITOR->get_region<AutomationRegion> ();
      z_return_if_fail (region);
      region->force_sort ();
    }

  TRANSPORT->recalculate_total_bars (sel);

  EVENTS_PUSH_NOW (EventType::ET_ARRANGER_SELECTIONS_IN_TRANSIT, sel);
}

/**
 * Gets the float value at the given Y coordinate relative to the automation
 * arranger.
 */
static float
get_fvalue_at_y (ArrangerWidget * self, double y)
{
  auto height = (float) gtk_widget_get_height (GTK_WIDGET (self));

  auto region = CLIP_EDITOR->get_region<AutomationRegion> ();
  z_return_val_if_fail (region, -1.f);
  auto at = region->get_automation_track ();

  /* get ratio from widget */
  auto  widget_value = height - (float) y;
  auto  widget_ratio = std::clamp<float> (widget_value / height, 0.f, 1.f);
  auto  port = Port::find_from_identifier<ControlPort> (at->port_id_);
  float automatable_value = port->normalized_val_to_real (widget_ratio);

  return automatable_value;
}

/**
 * Move selected arranger objects vertically.
 */
static void
move_items_y (ArrangerWidget * self, double offset_y)
{
  auto sel = arranger_widget_get_selections (self);
  z_return_if_fail (sel);

  switch (self->type)
    {
    case ArrangerWidgetType::Automation:
      if (!sel->objects_.empty ())
        {
          double offset_y_normalized =
            -offset_y
            / static_cast<double> (gtk_widget_get_height (GTK_WIDGET (self)));
          z_return_if_fail (self->sel_at_start);
          (void) get_fvalue_at_y;
          for (size_t i = 0; i < sel->objects_.size (); i++)
            {
              auto &ap = dynamic_cast<AutomationPoint &> (*sel->objects_[i]);
              auto &automation_sel =
                dynamic_cast<AutomationSelections &> (*self->sel_at_start);
              auto &start_ap =
                dynamic_cast<AutomationPoint &> (*automation_sel.objects_[i]);

              ap.set_fvalue (
                start_ap.normalized_val_
                  + static_cast<float> (offset_y_normalized),
                F_NORMALIZED, true);
            }
          z_return_if_fail (self->start_object);
        }
      break;
    case ArrangerWidgetType::Timeline:
      {
        /* old = original track
         * last = track where last change happened
         * (new) = track at hover position */
        auto track = timeline_arranger_widget_get_track_at_y (
          self, self->start_y + offset_y);
        auto old_track =
          timeline_arranger_widget_get_track_at_y (self, self->start_y);
        auto last_track = TRACKLIST->get_visible_track_after_delta (
          *old_track, self->visible_track_diff);

        auto lane = timeline_arranger_widget_get_track_lane_at_y (
          self, self->start_y + offset_y);
        auto old_lane =
          timeline_arranger_widget_get_track_lane_at_y (self, self->start_y);
        TrackLane * last_lane = nullptr;
        if (old_lane)
          {
            auto old_lane_variant =
              convert_to_variant<TrackLanePtrVariant> (old_lane);
            std::visit (
              [&self, &last_lane] (auto &&arg) {
                auto old_lane_track = arg->get_track ();
                last_lane =
                  old_lane_track->lanes_[arg->pos_ + self->lane_diff].get ();
              },
              old_lane_variant);
          }

        auto at =
          timeline_arranger_widget_get_at_at_y (self, self->start_y + offset_y);
        auto old_at = timeline_arranger_widget_get_at_at_y (self, self->start_y);
        AutomationTrack * last_at = nullptr;
        if (track && old_at)
          {
            auto automatable_track = dynamic_cast<AutomatableTrack *> (track);
            z_return_if_fail (automatable_track);
            last_at =
              automatable_track->automation_tracklist_
                ->get_visible_at_after_delta (*old_at, self->visible_at_diff);
          }

        /* if new track is equal, move lanes or automation lanes */
        if (
          track && last_track && track == last_track
          && self->visible_track_diff == 0)
          {
            if (old_lane && lane && last_lane && old_lane != lane)
              {
                int cur_diff = lane->pos_ - old_lane->pos_;
                int delta = lane->pos_ - last_lane->pos_;
                if (delta != 0)
                  {
                    bool moved =
                      TL_SELECTIONS->move_regions_to_new_lanes (delta);
                    if (moved)
                      {
                        self->lane_diff = cur_diff;
                      }
                  }
              }
            else if (at && old_at && last_at && at != old_at)
              {
                auto automatable_track =
                  dynamic_cast<AutomatableTrack *> (track);
                int cur_diff =
                  automatable_track->automation_tracklist_
                    ->get_visible_at_diff (*old_at, *at);
                int delta =
                  automatable_track->automation_tracklist_
                    ->get_visible_at_diff (*last_at, *at);
                if (delta != 0)
                  {
                    bool moved = TL_SELECTIONS->move_regions_to_new_ats (delta);
                    if (moved)
                      {
                        self->visible_at_diff = cur_diff;
                      }
                  }
              }
          }
        /* otherwise move tracks */
        else if (track && last_track && old_track && track != last_track)
          {
            int cur_diff =
              TRACKLIST->get_visible_track_diff (*old_track, *track);
            int delta = TRACKLIST->get_visible_track_diff (*last_track, *track);
            if (delta != 0)
              {
                bool moved = TL_SELECTIONS->move_regions_to_new_tracks (delta);
                if (moved)
                  {
                    self->visible_track_diff = cur_diff;
                  }
              }
          }
      }
      break;
    case ArrangerWidgetType::Midi:
      {
        int y_delta;
        /* first note selected */
        int first_note_selected =
          dynamic_cast<MidiNote &> (*self->start_object).val_;
        /* note at cursor */
        int note_at_cursor = piano_roll_keys_widget_get_key_from_y (
          MW_PIANO_ROLL_KEYS, self->start_y + offset_y);

        y_delta = note_at_cursor - first_note_selected;
        y_delta = midi_arranger_calc_deltamax_for_note_movement (y_delta);

        for (auto &obj : sel->objects_)
          {
            auto midi_note = dynamic_cast<MidiNote *> (obj.get ());
            midi_note->set_val (
              static_cast<uint8_t> (midi_note->val_ + y_delta));
          }
      }
      break;
    case ArrangerWidgetType::Chord:
      {
        int y_delta;
        /* first chord selected */
        auto &first_co = dynamic_cast<ChordObject &> (*self->start_object);
        int   first_chord_selected = first_co.chord_index_;
        /* chord at cursor */
        int chord_at_cursor =
          chord_arranger_widget_get_chord_at_y (self->start_y + offset_y);

        y_delta = chord_at_cursor - first_chord_selected;
        y_delta = chord_arranger_calc_deltamax_for_chord_movement (y_delta);

        for (auto &obj : sel->objects_)
          {
            auto co = dynamic_cast<ChordObject *> (obj.get ());
            co->chord_index_ += y_delta;
          }
      }
      break;
    default:
      break;
    }
}

void
arranger_widget_select_all (ArrangerWidget * self, bool select, bool fire_events)
{
  auto sel = arranger_widget_get_selections (self);
  z_return_if_fail (sel);

  if (select)
    {
      std::vector<ArrangerObject *> objs;
      arranger_widget_get_all_objects (self, objs);
      for (auto obj : objs)
        {
          obj->select (obj->shared_from_this (), true, true, false);
        }

      if (fire_events)
        {
          EVENTS_PUSH (EventType::ET_ARRANGER_SELECTIONS_CREATED, sel);
        }
    }
  else
    {
      z_debug ("deselecting all");
      if (sel->has_any ())
        {
          sel->clear (false);

          if (fire_events)
            {
              EVENTS_PUSH_NOW (EventType::ET_ARRANGER_SELECTIONS_REMOVED, sel);
            }
        }
    }
}

EditorSettingsPtrVariant
arranger_widget_get_editor_settings (ArrangerWidget * self)
{
  switch (self->type)
    {
    case ArrangerWidgetType::Timeline:
      return PRJ_TIMELINE.get ();
      break;
    case ArrangerWidgetType::Automation:
      return &AUTOMATION_EDITOR;
      break;
    case ArrangerWidgetType::Audio:
      return &AUDIO_CLIP_EDITOR;
      break;
    case ArrangerWidgetType::Midi:
    case ArrangerWidgetType::MidiModifier:
      return PIANO_ROLL;
      break;
    case ArrangerWidgetType::Chord:
      return CHORD_EDITOR;
      break;
    default:
      break;
    }

  z_return_val_if_reached ((Timeline *) nullptr);
}

static GMenu *
gen_context_menu_audio (ArrangerWidget * self, GMenu * menu, gdouble x, gdouble y)
{
  return g_menu_new ();
}

static GMenu *
gen_context_menu_chord (ArrangerWidget * self, GMenu * menu, gdouble x, gdouble y)
{
  return g_menu_new ();
}

static GMenu *
gen_context_menu_midi_modifier (
  ArrangerWidget * self,
  GMenu *          menu,
  gdouble          x,
  gdouble          y)
{
  return g_menu_new ();
}

/**
 * Show context menu at the given point.
 *
 * @param x X in absolute coordinates.
 * @param y Y in absolute coordinates.
 */
static void
show_context_menu (ArrangerWidget * self, gdouble x, gdouble y)
{
  GMenu *     menu = NULL;
  GMenuItem * menuitem;
  switch (self->type)
    {
    case ArrangerWidgetType::Timeline:
      menu = timeline_arranger_widget_gen_context_menu (self, x, y);
      break;
    case ArrangerWidgetType::Midi:
      menu = midi_arranger_widget_gen_context_menu (self, x, y);
      break;
    case ArrangerWidgetType::MidiModifier:
      menu = gen_context_menu_midi_modifier (self, menu, x, y);
      break;
    case ArrangerWidgetType::Chord:
      menu = gen_context_menu_chord (self, menu, x, y);
      break;
    case ArrangerWidgetType::Automation:
      menu = automation_arranger_widget_gen_context_menu (self, x, y);
      break;
    case ArrangerWidgetType::Audio:
      menu = gen_context_menu_audio (self, menu, x, y);
      break;
    }

  z_return_if_fail (menu);

  ArrangerObject * obj = arranger_widget_get_hit_arranger_object (
    self, ArrangerObject::Type::All, x, y);

  if (!obj)
    {
      GMenu * create_submenu = g_menu_new ();

      /* add "create object" menu item */
      auto action_name = fmt::sprintf (
        "app.create-arranger-obj(('%p',%f,%f))", fmt::ptr (self), x, y);
      menuitem = z_gtk_create_menu_item (
        _ ("Create Object"), nullptr, action_name.c_str ());
      g_menu_append_item (create_submenu, menuitem);

      g_menu_append_section (menu, nullptr, G_MENU_MODEL (create_submenu));
    }

  const auto &settings = arranger_widget_get_editor_setting_values (self);
  z_gtk_show_context_menu_from_g_menu (
    self->popover_menu, x - settings->scroll_start_x_,
    y - settings->scroll_start_y_, menu);
  g_object_unref (menu);
}

static void
on_right_click (
  GtkGestureClick * gesture,
  gint              n_press,
  gdouble           x,
  gdouble           y,
  ArrangerWidget *  self)
{
  z_info ("right mb released");
#if 0
  if (n_press != 1)
    return;

  MAIN_WINDOW->last_focused = GTK_WIDGET (self);

  /* if object clicked and object is unselected,
   * select it */
  ArrangerObject * obj =
    arranger_widget_get_hit_arranger_object (
      (ArrangerWidget *) self,
      ArrangerObject::Type::All, x, y);
  if (obj)
    {
      if (!arranger_object_is_selected (obj))
        {
          obj->select ( true, false,
            false);
        }
    }

  show_context_menu (self, x, y);
#endif
}

static void
auto_scroll (ArrangerWidget * self, int x, int y)
{
  /* figure out if we should scroll */
  bool scroll_h = false, scroll_v = false;
  switch (self->action)
    {
    case UiOverlayAction::MOVING:
    case UiOverlayAction::MovingCopy:
    case UiOverlayAction::MOVING_LINK:
    case UiOverlayAction::CREATING_MOVING:
    case UiOverlayAction::SELECTING:
    case UiOverlayAction::RAMPING:
      scroll_h = true;
      scroll_v = true;
      break;
    case UiOverlayAction::ResizingR:
    case UiOverlayAction::ResizingL:
    case UiOverlayAction::StretchingL:
    case UiOverlayAction::CreatingResizingR:
    case UiOverlayAction::StretchingR:
    case UiOverlayAction::AUTOFILLING:
    case UiOverlayAction::AUDITIONING:
      scroll_h = true;
      break;
    case UiOverlayAction::RESIZING_UP:
      scroll_v = true;
      break;
    default:
      break;
    }

  if (!arranger_widget_can_scroll_vertically (self))
    {
      scroll_v = false;
    }

  if (!scroll_h && !scroll_v)
    return;

  auto settings_variant = arranger_widget_get_editor_settings (self);
  std::visit (
    [&] (auto &&settings) {
      z_return_if_fail (settings);
      int h_scroll_speed = 20;
      int v_scroll_speed = 10;
      int border_distance = 5;
      int scroll_width = gtk_widget_get_width (GTK_WIDGET (self));
      int scroll_height = gtk_widget_get_height (GTK_WIDGET (self));
      int v_delta = 0;
      int h_delta = 0;
      int adj_x = settings->scroll_start_x_;
      int adj_y = settings->scroll_start_y_;
      if (y + border_distance >= adj_y + scroll_height)
        {
          v_delta = v_scroll_speed;
        }
      else if (y - border_distance <= adj_y)
        {
          v_delta = -v_scroll_speed;
        }
      if (x + border_distance >= adj_x + scroll_width)
        {
          h_delta = h_scroll_speed;
        }
      else if (x - border_distance <= adj_x)
        {
          h_delta = -h_scroll_speed;
        }

      if (!scroll_h)
        h_delta = 0;
      if (!scroll_v)
        v_delta = 0;

      if (settings->scroll_start_x_ + h_delta < 0)
        {
          h_delta -= settings->scroll_start_x_ + h_delta;
        }
      if (settings->scroll_start_y_ + v_delta < 0)
        {
          v_delta -= settings->scroll_start_y_ + v_delta;
        }
      settings->append_scroll (h_delta, v_delta, true);
      self->offset_x_from_scroll += h_delta;
      self->offset_y_from_scroll += v_delta;
    },
    settings_variant);
}

void
arranger_widget_on_key_release (
  GtkEventControllerKey * key_controller,
  guint                   keyval,
  guint                   keycode,
  GdkModifierType         state,
  ArrangerWidget *        self)
{
  self->key_is_pressed = 0;

  if (z_gtk_keyval_is_ctrl (keyval))
    {
      self->ctrl_held = 0;
    }

  if (z_gtk_keyval_is_shift (keyval))
    {
      self->shift_held = 0;
    }
  if (z_gtk_keyval_is_alt (keyval))
    {
      self->alt_held = 0;
    }

  ArrangerSelections * sel = arranger_widget_get_selections (self);

  if (self->action == UiOverlayAction::STARTING_MOVING)
    {
      if (self->alt_held && self->can_link)
        self->action = UiOverlayAction::MOVING_LINK;
      else if (self->ctrl_held && !sel->contains_unclonable_object ())
        self->action = UiOverlayAction::MovingCopy;
      else
        self->action = UiOverlayAction::MOVING;
    }
  else if (
    (self->action == UiOverlayAction::MOVING) && self->alt_held
    && self->can_link)
    {
      self->action = UiOverlayAction::MOVING_LINK;
    }
  else if (
    (self->action == UiOverlayAction::MOVING) && self->ctrl_held
    && !sel->contains_unclonable_object ())
    {
      self->action = UiOverlayAction::MovingCopy;
    }
  else if (
    (self->action == UiOverlayAction::MOVING_LINK) && !self->alt_held
    && self->can_link)
    {
      self->action =
        self->ctrl_held ? UiOverlayAction::MovingCopy : UiOverlayAction::MOVING;
    }
  else if ((self->action == UiOverlayAction::MovingCopy) && !self->ctrl_held)
    {
      self->action = UiOverlayAction::MOVING;
    }

  if (TYPE_IS (Timeline))
    {
      timeline_arranger_widget_set_cut_lines_visible (self);
    }

  /*arranger_widget_update_visibility (self);*/

  arranger_widget_refresh_cursor (self);
}

/**
 * Called from MainWindowWidget because some
 * events don't reach here.
 */
gboolean
arranger_widget_on_key_press (
  GtkEventControllerKey * key_controller,
  guint                   keyval,
  guint                   keycode,
  GdkModifierType         state,
  ArrangerWidget *        self)
{
  z_debug ("arranger widget key action");

  if (z_gtk_keyval_is_ctrl (keyval))
    {
      self->ctrl_held = 1;
    }
  if (z_gtk_keyval_is_shift (keyval))
    {
      self->shift_held = 1;
    }
  if (z_gtk_keyval_is_alt (keyval))
    {
      self->alt_held = 1;
    }

  ArrangerSelections * sel = arranger_widget_get_selections (self);
  z_return_val_if_fail (sel, false);

  if (self->action == UiOverlayAction::STARTING_MOVING)
    {
      if (self->ctrl_held && !sel->contains_unclonable_object ())
        self->action = UiOverlayAction::MovingCopy;
      else
        self->action = UiOverlayAction::MOVING;
    }
  else if (
    self->action == UiOverlayAction::MOVING && self->alt_held && self->can_link)
    {
      self->action = UiOverlayAction::MOVING_LINK;
    }
  else if (
    self->action == UiOverlayAction::MOVING && self->ctrl_held
    && !sel->contains_unclonable_object ())
    {
      self->action = UiOverlayAction::MovingCopy;
    }
  else if (self->action == UiOverlayAction::MOVING_LINK && !self->alt_held)
    {
      self->action =
        self->ctrl_held ? UiOverlayAction::MovingCopy : UiOverlayAction::MOVING;
    }
  else if (self->action == UiOverlayAction::MovingCopy && !self->ctrl_held)
    {
      self->action = UiOverlayAction::MOVING;
    }
  else if (self->action == UiOverlayAction::None)
    {
      if (sel->has_any ())
        {
          double move_ticks = 0.0;
          if (self->ctrl_held)
            {
              Position tmp;
              tmp.set_to_bar (2);
              move_ticks = tmp.ticks_;
            }
          else
            {
              SnapGrid * sg = arranger_widget_get_snap_grid (self);
              move_ticks = (double) sg->get_snap_ticks ();
            }

          /* check arrow movement */
          bool have_arrow_movement = false;
          try
            {
              if (keyval == GDK_KEY_Left)
                {
                  have_arrow_movement = true;
                  Position min_possible_pos;
                  arranger_widget_get_min_possible_position (
                    self, &min_possible_pos);

                  /* get earliest object */
                  auto [obj, first_obj_pos] =
                    sel->get_first_object_and_pos (false);

                  if (obj->pos_.ticks_ - move_ticks >= min_possible_pos.ticks_)
                    {
                      UNDO_MANAGER->perform (
                        std::make_unique<
                          ArrangerSelectionsAction::MoveByTicksAction> (
                          *sel, -move_ticks, false));

                      /* scroll left if needed */
                      arranger_widget_scroll_until_obj (
                        self, obj, 1, 0, 1, SCROLL_PADDING);
                    }
                }
              else if (keyval == GDK_KEY_Right)
                {
                  have_arrow_movement = true;
                  UNDO_MANAGER->perform (
                    std::make_unique<ArrangerSelectionsAction::MoveByTicksAction> (
                      *sel, move_ticks, false));

                  /* get latest object */
                  auto [obj, last_obj_pos] =
                    sel->get_last_object_and_pos (false, true);

                  /* scroll right if needed */
                  arranger_widget_scroll_until_obj (
                    self, obj, 1, 0, 0, SCROLL_PADDING);
                }
              else if (keyval == GDK_KEY_Down)
                {
                  have_arrow_movement = true;
                  if (
                    self == MW_MIDI_ARRANGER
                    || self == MW_MIDI_MODIFIER_ARRANGER)
                    {
                      int        pitch_delta = 0;
                      MidiNote * mn = MIDI_SELECTIONS->get_lowest_note ();
                      if (self->ctrl_held)
                        {
                          if (mn->val_ - 12 >= 0)
                            pitch_delta = -12;
                        }
                      else
                        {
                          if (mn->val_ - 1 >= 0)
                            pitch_delta = -1;
                        }

                      if (pitch_delta)
                        {
                          UNDO_MANAGER->perform (
                            std::make_unique<
                              ArrangerSelectionsAction::MoveMidiAction> (
                              dynamic_cast<MidiSelections &> (*sel), 0,
                              pitch_delta, false));

                          /* scroll down if needed */
                          arranger_widget_scroll_until_obj (
                            self, mn, 0, 0, 0, SCROLL_PADDING);
                        }
                    }
                  else if (self == MW_CHORD_ARRANGER)
                    {
                      UNDO_MANAGER->perform (
                        std::make_unique<
                          ArrangerSelectionsAction::MoveChordAction> (
                          dynamic_cast<ChordSelections &> (*sel), 0, 1, false));
                    }
                  else if (self == MW_TIMELINE)
                    {
                      /* TODO check if can be moved */
                      /*action =*/
                      /*arranger_selections_action_new_move (*/
                      /*sel, 0, -1, 0, 0, 0);*/
                      /*undo_manager_perform (*/
                      /*UNDO_MANAGER, action);*/
                    }
                }
              else if (keyval == GDK_KEY_Up)
                {
                  have_arrow_movement = true;
                  if (
                    self == MW_MIDI_ARRANGER
                    || self == MW_MIDI_MODIFIER_ARRANGER)
                    {
                      int  pitch_delta = 0;
                      auto mn = MIDI_SELECTIONS->get_highest_note ();
                      if (self->ctrl_held)
                        {
                          if (mn->val_ + 12 < 128)
                            pitch_delta = 12;
                        }
                      else
                        {
                          if (mn->val_ + 1 < 128)
                            pitch_delta = 1;
                        }

                      if (pitch_delta)
                        {
                          UNDO_MANAGER->perform (
                            std::make_unique<
                              ArrangerSelectionsAction::MoveMidiAction> (
                              dynamic_cast<MidiSelections &> (*sel), 0,
                              pitch_delta, false));

                          /* scroll up if needed */
                          arranger_widget_scroll_until_obj (
                            self, mn, 0, 1, 0, SCROLL_PADDING);
                        }
                    }
                  else if (self == MW_CHORD_ARRANGER)
                    {
                      UNDO_MANAGER->perform (
                        std::make_unique<
                          ArrangerSelectionsAction::MoveChordAction> (
                          dynamic_cast<ChordSelections &> (*sel), 0, -1, false));
                    }
                  else if (self == MW_TIMELINE)
                    {
                      /* TODO check if can be moved */
                      /*action =*/
                      /*arranger_selections_action_new_move (*/
                      /*sel, 0, 1, 0, 0, 0);*/
                      /*undo_manager_perform (*/
                      /*UNDO_MANAGER, action);*/
                    }
                } /* endif key up */
            }
          catch (const ZrythmException &e)
            {
              e.handle (_ ("Failed to move selection"));
            }

          if (have_arrow_movement && TYPE_IS (Midi))
            {
              midi_arranger_listen_notes (self, true);
              g_source_remove_and_zero (self->unlisten_notes_timeout_id);
              self->unlisten_notes_timeout_id = g_timeout_add (
                400, midi_arranger_unlisten_notes_source_func, self);
            }

        } /* arranger selections has any */
    }

  if (TYPE_IS (Timeline))
    {
      timeline_arranger_widget_set_cut_lines_visible (self);
    }

  arranger_widget_refresh_cursor (self);

  /* if space pressed, allow the shortcut func to be called */
  /* FIXME this whole function should not check
   * for keyvals and return false always */
  if (
    keyval == GDK_KEY_space || (keyval >= GDK_KEY_1 && keyval <= GDK_KEY_6)
    || keyval == GDK_KEY_A || keyval == GDK_KEY_a || keyval == GDK_KEY_M
    || keyval == GDK_KEY_m || keyval == GDK_KEY_Q || keyval == GDK_KEY_q
    || keyval == GDK_KEY_less || keyval == GDK_KEY_Delete
    || keyval == GDK_KEY_greater || keyval == GDK_KEY_F2
    || keyval == GDK_KEY_KP_4 || keyval == GDK_KEY_KP_6 || keyval == GDK_KEY_Tab
    || keyval == GDK_KEY_ISO_Left_Tab)
    {
      z_debug ("ignoring keyval used for shortcuts");
      return false;
    }
  else
    z_debug ("not ignoring keyval %x", keyval);

  /* if key was Esc, cancel any drag and adjust the undo/redo stacks */
  if (
    keyval == GDK_KEY_Escape && gtk_gesture_is_active (GTK_GESTURE (self->drag)))
    {
      UNDO_MANAGER->redo_stack_locked_ = true;
      auto last_action = UNDO_MANAGER->get_last_action ();
      gtk_gesture_set_state (
        GTK_GESTURE (self->drag), GTK_EVENT_SEQUENCE_DENIED);
      auto new_last_action = UNDO_MANAGER->get_last_action ();
      if (new_last_action != last_action)
        {
          try
            {
              UNDO_MANAGER->undo ();
            }
          catch (const ZrythmException &e)
            {
              e.handle (_ ("Failed to undo"));
            }
        }
      UNDO_MANAGER->redo_stack_locked_ = false;
    }

  return true;
}

void
arranger_widget_set_highlight_rect (ArrangerWidget * self, GdkRectangle * rect)
{
  if (rect)
    {
      self->is_highlighted = true;
      /*self->prev_highlight_rect =*/
      /*self->highlight_rect;*/
      self->highlight_rect = *rect;
    }
  else
    {
      self->is_highlighted = false;
    }

  EVENTS_PUSH (EventType::ET_ARRANGER_HIGHLIGHT_CHANGED, self);
}

/**
 * On button press.
 *
 * This merely sets the number of clicks and objects clicked on. It is always
 * called before drag_begin, so the logic is done in drag_begin.
 */
static void
click_pressed (
  GtkGestureClick * gesture,
  gint              n_press,
  gdouble           x,
  gdouble           y,
  ArrangerWidget *  self)
{
  z_debug ("arranger click pressed - npress {}", n_press);

  /* set number of presses */
  self->n_press = n_press;

  /* set modifier button states */
  GdkModifierType state = gtk_event_controller_get_current_event_state (
    GTK_EVENT_CONTROLLER (gesture));
  if (state & GDK_SHIFT_MASK)
    self->shift_held = 1;
  if (state & GDK_CONTROL_MASK)
    self->ctrl_held = 1;
  if (state & GDK_ALT_MASK)
    self->alt_held = 1;

  PROJECT->last_selection_ =
    self->type == ArrangerWidgetType::Timeline
      ? Project::SelectionType::Timeline
      : Project::SelectionType::Editor;
  EVENTS_PUSH (EventType::ET_PROJECT_SELECTION_TYPE_CHANGED, nullptr);
}

static void
click_stopped (GtkGestureClick * click, ArrangerWidget * self)
{
  z_debug ("arranger click stopped");

  self->n_press = 0;
}

void
arranger_widget_create_item (
  ArrangerWidget * self,
  double           start_x,
  double           start_y,
  bool             autofilling)
{
  /* something will be created */
  Track *           track = NULL;
  AutomationTrack * at = NULL;
  int               note, chord_index;

  /* get the position */
  Position pos = arranger_widget_px_to_pos (self, start_x, true);

  /* make sure the position is positive */
  if (pos < Position ())
    {
      pos = Position ();
    }

  /* snap it */
  if (autofilling || (!self->shift_held && self->snap_grid->any_snap ()))
    {
      Track * track_for_snap = NULL;
      if (TYPE_IS (Timeline))
        {
          track_for_snap =
            timeline_arranger_widget_get_track_at_y (self, start_y);
        }

      SnapGrid sg = *self->snap_grid;
      /* if autofilling, make sure that snapping is enabled */
      if (autofilling)
        {
          sg.snap_to_grid_ = true;
        }

      pos.snap (
        self->earliest_obj_start_pos.get (), track_for_snap, nullptr, sg);
    }

  z_debug ("creating item at {:f},{:f}", start_x, start_y);

  switch (self->type)
    {
    case ArrangerWidgetType::Timeline:
      {
        /* figure out if we are creating a region or
         * automation point */
        at = timeline_arranger_widget_get_at_at_y (self, start_y);
        track = timeline_arranger_widget_get_track_at_y (self, start_y);

        try
          {
            if (at) /* if creating automation region */
              {
                timeline_arranger_widget_create_region<AutomationRegion> (
                  self, track, nullptr, at, &pos);
              }
            else if (track) /* else if double click inside track */
              {
                TrackLane * lane =
                  timeline_arranger_widget_get_track_lane_at_y (self, start_y);
                switch (track->type_)
                  {
                  case Track::Type::Instrument:
                  case Track::Type::Midi:
                    timeline_arranger_widget_create_region<MidiRegion> (
                      self, track,
                      static_cast<TrackLaneImpl<MidiRegion> *> (lane), nullptr,
                      &pos);
                    break;
                  case Track::Type::Audio:
                    break;
                  case Track::Type::Master:
                    break;
                  case Track::Type::Chord:
                    timeline_arranger_widget_create_chord_or_scale (
                      self, track, start_y, &pos);
                    break;
                  case Track::Type::AudioBus:
                    break;
                  case Track::Type::AudioGroup:
                    break;
                  case Track::Type::Marker:
                    timeline_arranger_widget_create_marker (self, track, &pos);
                    break;
                  default:
                    /* TODO */
                    break;
                  }
              }
          }
        catch (const ZrythmException &e)
          {
            e.handle (_ ("Failed to create object"));
            return;
          }
      }
      break;
    case ArrangerWidgetType::Midi:
      {
        /* find the note and region at x,y */
        note =
          piano_roll_keys_widget_get_key_from_y (MW_PIANO_ROLL_KEYS, start_y);
        auto region = CLIP_EDITOR->get_region<MidiRegion> ();

        /* create a note */
        if (region)
          {
            midi_arranger_widget_create_note (self, pos, note, *region);
          }
      }
      break;
    case ArrangerWidgetType::MidiModifier:
    case ArrangerWidgetType::Audio:
      break;
    case ArrangerWidgetType::Chord:
      {
        /* find the chord and region at x,y */
        chord_index = chord_arranger_widget_get_chord_at_y (start_y);
        auto region = CLIP_EDITOR->get_region<ChordRegion> ();

        /* create a chord object */
        if (
          region
          && chord_index < static_cast<int> (CHORD_EDITOR->chords_.size ()))
          {
            chord_arranger_widget_create_chord (self, pos, chord_index, *region);
          }
      }
      break;
    case ArrangerWidgetType::Automation:
      {
        auto region = CLIP_EDITOR->get_region<AutomationRegion> ();

        if (region)
          {
            automation_arranger_widget_create_ap (
              self, &pos, start_y, region, autofilling);
          }
      }
      break;
    }

  if (!autofilling)
    {
      /* set the start selections */
      std::visit (
        [&] (auto &&sel) {
          z_return_if_fail (sel);
          self->sel_at_start = sel->clone_unique ();
        },
        convert_to_variant<ArrangerSelectionsPtrVariant> (
          arranger_widget_get_selections (self)));
    }
}

/**
 * Called to autofill at the given position.
 *
 * In the case of velocities, this will set the velocity wherever hit.
 *
 * In the case of automation, this will create or edit the automation point at
 * the given position.
 *
 * In other cases, this will create an object with the default length at the
 * given position, unless an object already exists there.
 */
ATTR_NONNULL static void
autofill (ArrangerWidget * self, double x, double y)
{
  /* make sure values are valid */
  x = MAX (x, 0);
  y = MAX (y, 0);

  /* start autofill if not started yet */
  if (self->action != UiOverlayAction::AUTOFILLING)
    {
      self->action = UiOverlayAction::AUTOFILLING;
      ArrangerSelections * sel = arranger_widget_get_selections (self);
      z_return_if_fail (sel);

      /* clear the actual selections to append created objects */
      sel->clear (false);

      /* also clear the selections at start so we can append the affected
       * objects */
      auto sel_variant = convert_to_variant<ArrangerSelectionsPtrVariant> (sel);
      std::visit (
        [&] (auto &&s) { self->sel_at_start = s->clone_unique (); },
        sel_variant);

      auto clip_editor_region = CLIP_EDITOR->get_region ();
      auto region_variant =
        convert_to_variant<RegionPtrVariant> (clip_editor_region);
      std::visit (
        [&] (auto &&region) {
          self->region_at_start = region ? region->clone_unique () : nullptr;
        },
        region_variant);
    }

  if (self->type == ArrangerWidgetType::MidiModifier)
    {
      midi_modifier_arranger_set_hit_velocity_vals (self, x, y, true);
    }
  else if (self->type == ArrangerWidgetType::Automation)
    {
      /* move aps or create ap */
      if (!automation_arranger_move_hit_aps (self, x, y))
        {
          arranger_widget_create_item (self, x, y, true);
        }
    }
  else
    {
      ArrangerObject * obj = arranger_widget_get_hit_arranger_object (
        self, ArrangerObject::Type::All, x, y);

      /* don't write over object */
      if (obj)
        {
          z_info (
            "object already exists at {:f},{:f}, "
            "skipping",
            x, y);
          return;
        }
      arranger_widget_create_item (self, x, y, true);
    }
}

static void
drag_cancel (
  GtkGesture *       gesture,
  GdkEventSequence * sequence,
  ArrangerWidget *   self)
{
  z_info ("drag cancelled");
}

/**
 * Sets the start pos of the earliest object and the flag whether the earliest
 * object exists.
 */
ATTR_NONNULL static void
set_earliest_obj (ArrangerWidget * self)
{
  z_debug ("setting earliest object...");

  ArrangerSelections * sel = arranger_widget_get_selections (self);
  z_return_if_fail (sel);
  self->earliest_obj_start_pos.reset ();
  if (sel->has_any ())
    {
      auto [start_obj, start_pos] = sel->get_first_object_and_pos (true);
      self->earliest_obj_start_pos = std::make_unique<Position> (start_pos);
      z_debug ("earliest object position: {}", *self->earliest_obj_start_pos);
    }
  else
    {
      z_debug ("cleared earliest object position");
    }
}

/**
 * Checks for the first object hit, sets the appropriate action and selects it.
 *
 * @return If an object was handled or not.
 */
static bool
on_drag_begin_handle_hit_object (
  ArrangerWidget * self,
  const double     x,
  const double     y)
{
  auto obj = arranger_widget_get_hit_arranger_object (
    self, ArrangerObject::Type::All, x, y);
  auto obj_variant = convert_to_variant<ArrangerObjectPtrVariant> (obj);

  return std::visit (
    [&] (auto &&o) {
      using ObjT = base_type<decltype (o)>;
      if (!o || o->is_frozen ())
        {
          return false;
        }

      /* get x,y as local to the object */
      int wx = static_cast<int> (x) - o->full_rect_.x;
      int wy = static_cast<int> (y) - o->full_rect_.y;

      /* remember object and pos */
      arranger_widget_set_start_object (
        self, o->template shared_from_this_as<ObjT> ());
      self->start_pos_px = x;

      /* get flags */
      bool is_fade_in_point =
        arranger_object_is_fade_in (o, wx, wy, true, false);
      bool is_fade_out_point =
        arranger_object_is_fade_out (o, wx, wy, true, false);
      bool is_fade_in_outer =
        arranger_object_is_fade_in (o, wx, wy, false, true);
      bool is_fade_out_outer =
        arranger_object_is_fade_out (o, wx, wy, false, true);
      bool is_resize_l = arranger_object_is_resize_l (o, wx);
      bool is_resize_r = arranger_object_is_resize_r (o, wx);
      bool is_resize_up = arranger_object_is_resize_up (o, wx, wy);
      bool is_resize_loop =
        arranger_object_is_resize_loop (o, wy, self->ctrl_held);
      bool show_cut_lines =
        arranger_object_should_show_cut_lines (o, self->alt_held);
      bool is_rename = arranger_object_is_rename (o, wx, wy);
      bool is_selected = o->is_selected ();
      self->start_object_was_selected = is_selected;

      /* select object if unselected */
      switch (P_TOOL)
        {
        case Tool::Select:
        case Tool::Edit:
          if (!is_selected)
            {
              if (self->ctrl_held)
                {
                  /* append to selections */
                  o->select (true, true, true);
                }
              else
                {
                  /* make it the only selection */
                  o->select (true, false, true);
                  z_info ("making only selection");
                }
            }
          break;
        case Tool::Cut:
          /* only select this object */
          o->select (true, false, true);
          break;
        default:
          break;
        }

      /* set editor region and show editor if double click */
      if constexpr (std::derived_from<ObjT, Region>)
        {
          if (self->drag_start_btn == GDK_BUTTON_PRIMARY)
            {
              CLIP_EDITOR->set_region (o, true);

              /* if double click bring up piano roll */
              if (self->n_press == 2 && !self->ctrl_held)
                {
                  z_debug ("double clicked on region - showing piano roll");
                  EVENTS_PUSH (EventType::ET_REGION_ACTIVATED, nullptr);
                }
            }
        }
      /* if midi note from a ghosted region set the clip editor region */
      else if constexpr (std::is_same_v<ObjT, MidiNote>)
        {
          auto cur_r = CLIP_EDITOR->get_region ();
          auto r = o->get_region ();
          if (r != cur_r)
            {
              CLIP_EDITOR->set_region (r, true);
            }
        }
      /* if open marker dialog if double click on marker */
      else if constexpr (std::is_same_v<ObjT, Marker>)
        {
          if (self->n_press == 2 && !self->ctrl_held)
            {
              auto dialog = string_entry_dialog_widget_new (
                _ ("Marker name"),
                bind_member_function (*o, &NameableObject::get_name),
                bind_member_function (*o, &NameableObject::set_name_with_action));
              gtk_window_present (GTK_WINDOW (dialog));
              self->action = UiOverlayAction::None;
              return true;
            }
        }
      /* if double click on scale, open scale selector */
      else if constexpr (std::is_same_v<ObjT, ScaleObject>)
        {
          if (self->n_press == 2 && !self->ctrl_held)
            {
              auto scale_selector = scale_selector_window_widget_new (
                dynamic_cast<ScaleObject *> (o));
              gtk_window_present (GTK_WINDOW (scale_selector));
              self->action = UiOverlayAction::None;
              return true;
            }
        }
      /* if double click on automation point, ask for value */
      else if constexpr (std::is_same_v<ObjT, AutomationPoint>)
        {
          if (self->n_press == 2 && !self->ctrl_held)
            {
              auto dialog = string_entry_dialog_widget_new (
                _ ("Automation value"),
                bind_member_function (*o, &AutomationPoint::get_fvalue_as_string),
                bind_member_function (
                  *o, &AutomationPoint::set_fvalue_with_action));
              gtk_window_present (GTK_WINDOW (dialog));
              self->action = UiOverlayAction::None;
              return true;
            }
        }

      /* check if all selected objects are fadeable or resizable */
      if (is_resize_l || is_resize_r)
        {
          auto sel = arranger_widget_get_selections (self);

          bool have_unresizable = sel->contains_object_with_property (
            ArrangerSelections::Property::HasLength, false);
          if (have_unresizable)
            {
              ui_show_message_literal (
                _ ("Cannot Resize"),
                _ ("Cannot resize because the "
                   "selection contains objects "
                   "without length"));
              return false;
            }

          bool have_looped = sel->contains_looped ();
          if ((is_resize_l || is_resize_r) && !is_resize_loop && have_looped)
            {
              bool have_unloopable = sel->contains_object_with_property (
                ArrangerSelections::Property::CanLoop, false);
              if (have_unloopable)
                {
                  /* cancel resize since we have a looped object mixed with
                   * unloopable objects in the selection */
                  ui_show_message_literal (
                    _ ("Cannot Resize"),
                    _ ("Cannot resize because the selection contains a mix of looped and unloopable objects"));
                  return false;
                }
              else
                {
                  /* loop-resize since we have a loopable object in the
                   * selection and all other objects are loopable */
                  z_debug (
                    "convert resize to resize-loop - have looped object in the selection");
                  is_resize_loop = true;
                }
            }
        }
      if (
        is_fade_in_point || is_fade_in_outer || is_fade_out_point
        || is_fade_out_outer)
        {
          auto sel = arranger_widget_get_selections (self);
          bool have_unfadeable = sel->contains_object_with_property (
            ArrangerSelections::Property::CanFade, false);
          if (have_unfadeable)
            {
              /* don't fade */
              is_fade_in_point = false;
              is_fade_in_outer = false;
              is_fade_out_point = false;
              is_fade_out_outer = false;
            }
        }

#define SET_ACTION(x) self->action = UiOverlayAction::x

      z_debug ("action before");
      arranger_widget_print_action (self);

      bool drum_mode = arranger_widget_get_drum_mode_enabled (self);

      /* update arranger action */
      switch (o->type_)
        {
        case ArrangerObject::Type::Region:
          switch (P_TOOL)
            {
            case Tool::Eraser:
              SET_ACTION (STARTING_ERASING);
              break;
            case Tool::Audition:
              SET_ACTION (AUDITIONING);
              break;
            case Tool::Select:
              if (show_cut_lines)
                SET_ACTION (CUTTING);
              else if (is_fade_in_point)
                SET_ACTION (ResizingLFade);
              else if (is_fade_out_point)
                SET_ACTION (ResizingRFade);
              else if (is_resize_l && is_resize_loop)
                SET_ACTION (ResizingLLoop);
              else if (is_resize_l)
                {
                  self->action =
                    self->ctrl_held
                      ? UiOverlayAction::StretchingL
                      : UiOverlayAction::ResizingL;
                }
              else if (is_resize_r && is_resize_loop)
                SET_ACTION (ResizingRLoop);
              else if (is_resize_r)
                {
                  self->action =
                    self->ctrl_held
                      ? UiOverlayAction::StretchingR
                      : UiOverlayAction::ResizingR;
                }
              else if (is_rename)
                SET_ACTION (RENAMING);
              else if (is_fade_in_outer)
                SET_ACTION (RESIZING_UP_FADE_IN);
              else if (is_fade_out_outer)
                SET_ACTION (RESIZING_UP_FADE_OUT);
              else
                SET_ACTION (STARTING_MOVING);
              break;
            case Tool::Edit:
              if (is_resize_l)
                SET_ACTION (ResizingL);
              else if (is_resize_r)
                SET_ACTION (ResizingR);
              else
                SET_ACTION (STARTING_MOVING);
              break;
            case Tool::Cut:
              SET_ACTION (CUTTING);
              break;
            case Tool::Ramp:
              /* TODO */
              break;
            }
          break;
        case ArrangerObject::Type::MidiNote:
          switch (P_TOOL)
            {
            case Tool::Eraser:
              SET_ACTION (STARTING_ERASING);
              break;
            case Tool::Audition:
              SET_ACTION (AUDITIONING);
              break;
            case Tool::Select:
            case Tool::Edit:
              if ((is_resize_l) && !drum_mode)
                SET_ACTION (ResizingL);
              else if (is_resize_r && !drum_mode)
                SET_ACTION (ResizingR);
              else
                SET_ACTION (STARTING_MOVING);
              break;
            case Tool::Cut:
              /* TODO */
              break;
            case Tool::Ramp:
              break;
            }
          break;
        case ArrangerObject::Type::AutomationPoint:
          switch (P_TOOL)
            {
            case Tool::Select:
            case Tool::Edit:
              if (is_resize_up)
                SET_ACTION (RESIZING_UP);
              else
                SET_ACTION (STARTING_MOVING);
              break;
            default:
              break;
            }
          break;
        case ArrangerObject::Type::Velocity:
          switch (P_TOOL)
            {
            case Tool::Select:
            case Tool::Edit:
            case Tool::Ramp:
              SET_ACTION (STARTING_MOVING);
              if (is_resize_up)
                SET_ACTION (RESIZING_UP);
              else
                SET_ACTION (None);
              break;
            default:
              break;
            }
          break;
        case ArrangerObject::Type::ChordObject:
        case ArrangerObject::Type::ScaleObject:
        case ArrangerObject::Type::Marker:
          switch (P_TOOL)
            {
            case Tool::Select:
            case Tool::Edit:
              SET_ACTION (STARTING_MOVING);
              break;
            default:
              break;
            }
          break;
        default:
          z_return_val_if_reached (false);
        }

      z_debug ("action after");
      arranger_widget_print_action (self);

#undef SET_ACTION

      auto orig_selections = arranger_widget_get_selections (self);
      auto orig_selections_variant =
        convert_to_variant<ArrangerSelectionsPtrVariant> (orig_selections);
      std::visit (
        [&] (auto &&orig_sel) {
          constexpr bool is_timeline =
            std::is_same_v<base_type<decltype (orig_sel)>, TimelineSelections *>;

          /* set index in prev lane for selected objects if timeline */
          if constexpr (is_timeline)
            {
              TL_SELECTIONS->set_index_in_prev_lane ();
            }

          /* clone the arranger selections at this point */
          self->sel_at_start = orig_sel->clone_unique ();

          /* if the action is stretching, set the "before_length" on each region
           */
          if constexpr (is_timeline)
            {
              if (self->action == UiOverlayAction::StretchingR)
                {
                  TRANSPORT->prepare_audio_regions_for_stretch (orig_sel);
                }
            }
        },
        orig_selections_variant);

      return true;
    },
    obj_variant);
}

static void
drag_begin (
  GtkGestureDrag * gesture,
  gdouble          start_x,
  gdouble          start_y,
  ArrangerWidget * self)
{
  z_debug ("arranger drag begin starting...");

  self->offset_x_from_scroll = 0;
  self->offset_y_from_scroll = 0;

  const auto settings = arranger_widget_get_editor_setting_values (self);
  start_x += settings->scroll_start_x_;
  start_y += settings->scroll_start_y_;

  self->start_x = start_x;
  self->hover_x = start_x;
  self->start_pos = arranger_widget_px_to_pos (self, start_x, true);
  self->start_y = start_y;
  self->hover_y = start_y;
  ;
  self->drag_update_started = false;

  /* set last project selection type */
  if (self->type == ArrangerWidgetType::Timeline)
    {
      PROJECT->last_selection_ = Project::SelectionType::Timeline;
    }
  else
    {
      PROJECT->last_selection_ = Project::SelectionType::Editor;
    }

  self->drag_start_btn =
    gtk_gesture_single_get_current_button (GTK_GESTURE_SINGLE (gesture));
  switch (self->drag_start_btn)
    {
    case GDK_BUTTON_PRIMARY:
      z_debug ("primary button clicked");
      break;
    case GDK_BUTTON_SECONDARY:
      z_debug ("secondary button clicked");
      break;
    case GDK_BUTTON_MIDDLE:
      z_debug ("middle button clicked");
      break;
    default:
      break;
    }
  z_warn_if_fail (self->drag_start_btn);

  /* check if selections can create links */
  self->can_link = TYPE_IS (Timeline) && TL_SELECTIONS->contains_only_regions ();

  if (!gtk_widget_has_focus (GTK_WIDGET (self)))
    gtk_widget_grab_focus (GTK_WIDGET (self));

  /* get current pos */
  self->curr_pos = arranger_widget_px_to_pos (self, self->start_x, true);

  /* get difference with drag start pos */
  self->curr_ticks_diff_from_start =
    Position::get_ticks_diff (self->curr_pos, self->start_pos, nullptr);

  /* handle hit object */
  int objects_hit = on_drag_begin_handle_hit_object (self, start_x, start_y);
  z_info ("objects hit {}", objects_hit);
  arranger_widget_print_action (self);

  if (objects_hit)
    {
      ArrangerSelections * sel = arranger_widget_get_selections (self);
      self->sel_at_start =
        clone_unique_with_variant<ArrangerSelectionsVariant> (sel);
    }
  /* if nothing hit */
  else
    {
      self->sel_at_start = NULL;

      z_debug ("npress = {}", self->n_press);

      /* single click */
      if (self->n_press == 1)
        {
          switch (P_TOOL)
            {
            case Tool::Select:
              if (self->drag_start_btn == GDK_BUTTON_MIDDLE || self->alt_held)
                {
                  self->action = UiOverlayAction::StartingPanning;
                }
              else
                {
                  /* selection */
                  self->action = UiOverlayAction::STARTING_SELECTION;

                  if (!self->ctrl_held)
                    {
                      /* deselect all */
                      arranger_widget_select_all (self, false, true);
                    }

                  /* set whether selecting objects or selecting range */
                  set_select_type (self, start_y);

                  /* hide range selection */
                  TRANSPORT->set_has_range (false);

                  /* hide range selection if audio arranger and set appropriate
                   * action */
                  if (self->type == ArrangerWidgetType::Audio)
                    {
                      AUDIO_SELECTIONS->has_selection_ = false;
                      self->action =
                        audio_arranger_widget_get_action_on_drag_begin (self);
                    }
                }
              break;
            case Tool::Edit:
              if (
                self->type == ArrangerWidgetType::Timeline
                || self->type == ArrangerWidgetType::Midi
                || self->type == ArrangerWidgetType::Chord)
                {
                  if (self->ctrl_held)
                    {
                      /* autofill */
                      autofill (self, start_x, start_y);
                    }
                  else
                    {
                      /* something is created */
                      arranger_widget_create_item (
                        self, start_x, start_y, false);
                    }
                }
              else if (
                self->type == ArrangerWidgetType::MidiModifier
                || self->type == ArrangerWidgetType::Automation)
                {
                  /* autofill (also works for
                   * manual edit for velocity and
                   * automation */
                  autofill (self, start_x, start_y);
                }
              break;
            case Tool::Eraser:
              /* delete selection */
              self->action = UiOverlayAction::STARTING_DELETE_SELECTION;
              break;
            case Tool::Ramp:
              self->action = UiOverlayAction::STARTING_RAMP;
              break;
            case Tool::Audition:
              self->action = UiOverlayAction::STARTING_AUDITIONING;
              self->was_paused = TRANSPORT->is_paused ();
              self->playhead_pos_at_start = PLAYHEAD;
              TRANSPORT->set_playhead_pos (self->start_pos);
              TRANSPORT->request_roll (true);
            default:
              break;
            }
        }
      /* double click */
      else if (self->n_press == 2)
        {
          switch (P_TOOL)
            {
            case Tool::Select:
            case Tool::Edit:
              arranger_widget_create_item (self, start_x, start_y, false);
              break;
            case Tool::Eraser:
              /* delete selection */
              self->action = UiOverlayAction::STARTING_DELETE_SELECTION;
              break;
            default:
              break;
            }
        }
    }

  /* set the start pos of the earliest object and the flag whether the earliest
   * object exists */
  set_earliest_obj (self);

  arranger_widget_refresh_cursor (self);

  z_debug ("arranger drag begin done");
}

/**
 * Selects objects for the given arranger in the
 * range from start_* to offset_*.
 *
 * @param ignore_frozen Ignore frozen objects.
 * @param[in] delete Whether this is a select-delete
 *   operation.
 * @param[in] in_range Whether to select/delete
 *   objects in the range or exactly at the current
 *   point.
 */
ATTR_NONNULL static void
select_in_range (
  ArrangerWidget * self,
  double           offset_x,
  double           offset_y,
  bool             in_range,
  bool             ignore_frozen,
  bool             del)
{
  ArrangerSelections * arranger_sel = arranger_widget_get_selections (self);
  z_return_if_fail (arranger_sel);
  auto prev_sel =
    clone_unique_with_variant<ArrangerSelectionsVariant> (arranger_sel);

  if (del && in_range)
    {
      std::vector<ArrangerObject *> objs_to_delete;
      for (auto &obj : self->sel_to_delete->objects_)
        {
          objs_to_delete.push_back (obj.get ());
        }
      if (ignore_frozen)
        {
          filter_out_frozen_objects (self, objs_to_delete);
        }
      for (auto obj : objs_to_delete)
        {
          obj->deleted_temporarily_ = false;
        }
      self->sel_to_delete->clear (false);
    }
  else if (!del)
    {
      if (!self->ctrl_held)
        {
          /* deselect all */
          arranger_widget_select_all (self, false, false);
        }
    }

  GdkRectangle rect;
  if (in_range)
    {
      GdkRectangle _rect = {
        .x =
          (int) offset_x >= 0
            ? (int) self->start_x
            : (int) (self->start_x + offset_x),
        .y =
          (int) offset_y >= 0
            ? (int) self->start_y
            : (int) (self->start_y + offset_y),
        .width = abs ((int) offset_x),
        .height = abs ((int) offset_y),
      };
      rect = _rect;
    }
  else
    {
      GdkRectangle _rect = {
        .x = (int) (self->start_x + offset_x),
        .y = (int) (self->start_y + offset_y),
        .width = 1,
        .height = 1,
      };
      rect = _rect;
    }

  /* redraw union of last rectangle and new
   * rectangle */
  GdkRectangle union_rect;
  gdk_rectangle_union (&rect, &self->last_selection_rect, &union_rect);
  self->last_selection_rect = rect;

  auto add_each_hit_obj_to_sel = [&] (ArrangerObject::Type type) {
    std::vector<ArrangerObject *> objs_arr;
    arranger_widget_get_hit_objects_in_rect (self, type, &rect, objs_arr);
    if (ignore_frozen)
      {
        filter_out_frozen_objects (self, objs_arr);
      }
    for (auto obj : objs_arr)
      {
        auto shared_obj = obj->shared_from_this ();
        if (type == ArrangerObject::Type::Velocity)
          {
            auto vel_obj = dynamic_cast<Velocity *> (obj);
            shared_obj = vel_obj->get_midi_note ()->shared_from_this ();
          }
        if (del)
          {
            self->sel_to_delete->add_object_owned (
              clone_unique_with_variant<ArrangerObjectVariant> (
                shared_obj.get ()));
            obj->deleted_temporarily_ = true;
          }
        else
          {
            ArrangerObject::select (shared_obj, true, true, false);
          }
      }
  };

  switch (self->type)
    {
    case ArrangerWidgetType::Chord:
      add_each_hit_obj_to_sel (ArrangerObject::Type::ChordObject);
      break;
    case ArrangerWidgetType::Automation:
      add_each_hit_obj_to_sel (ArrangerObject::Type::AutomationPoint);
      break;
    case ArrangerWidgetType::Timeline:
      add_each_hit_obj_to_sel (ArrangerObject::Type::Region);
      add_each_hit_obj_to_sel (ArrangerObject::Type::ScaleObject);
      add_each_hit_obj_to_sel (ArrangerObject::Type::Marker);
      break;
    case ArrangerWidgetType::Midi:
      {
        add_each_hit_obj_to_sel (ArrangerObject::Type::MidiNote);
        auto prev_midi_selections =
          static_cast<MidiSelections *> (prev_sel.get ());
        prev_midi_selections->unlisten_note_diff (
          static_cast<MidiSelections &> (*arranger_sel));
      }
      break;
    case ArrangerWidgetType::MidiModifier:
      add_each_hit_obj_to_sel (ArrangerObject::Type::Velocity);
      break;
    default:
      break;
    }
}

static void
pan (ArrangerWidget * self, double offset_x, double offset_y)
{
  offset_x -= self->last_offset_x;
  offset_y -= self->last_offset_y;
  z_trace ("panning {:f} {:f}", offset_x, offset_y);

  /* pan */
  auto settings = arranger_widget_get_editor_settings (self);
  std::visit (
    [&] (auto &&s) {
      s->append_scroll ((int) -offset_x, (int) -offset_y, true);

      /* these are also affected */
      self->last_offset_x = MAX (0, self->last_offset_x - offset_x);
      self->hover_x = MAX (0, self->hover_x - offset_x);
      self->start_x = MAX (0, self->start_x - offset_x);
      self->last_offset_y = MAX (0, self->last_offset_y - offset_y);
      self->hover_y = MAX (0, self->hover_y - offset_y);
      self->start_y = MAX (0, self->start_y - offset_y);
    },
    settings);
}

ATTR_NONNULL static void
drag_update (
  GtkGestureDrag * gesture,
  gdouble          offset_x,
  gdouble          offset_y,
  ArrangerWidget * self)
{
  offset_x += self->offset_x_from_scroll;
  offset_y += self->offset_y_from_scroll;

  if (
    !self->drag_update_started
    && !gtk_drag_check_threshold (
      GTK_WIDGET (self), (int) self->start_x, (int) self->start_y,
      (int) (self->start_x + offset_x), (int) (self->start_y + offset_y)))
    {
      return;
    }

  self->drag_update_started = true;

  ArrangerSelections * sel = arranger_widget_get_selections (self);

  /* state mask needs to be updated */
  GdkModifierType state = gtk_event_controller_get_current_event_state (
    GTK_EVENT_CONTROLLER (gesture));
  self->shift_held = state & GDK_SHIFT_MASK;
  self->ctrl_held = state & GDK_CONTROL_MASK;
  self->alt_held = state & GDK_ALT_MASK;

#if 0
  z_trace (
    "shift: {}, ctrl: {}, alt: {}", self->shift_held, self->ctrl_held,
    self->alt_held);
#endif

  /* get current pos */
  self->curr_pos =
    arranger_widget_px_to_pos (self, self->start_x + offset_x, true);

  /* get difference with drag start pos */
  self->curr_ticks_diff_from_start =
    Position::get_ticks_diff (self->curr_pos, self->start_pos, nullptr);

  z_trace (
    "[action: {}] start position: {}, current position {} ({:.1f},{:.1f})",
    self->action, self->start_pos, self->curr_pos, offset_x, offset_y);

  if (self->earliest_obj_start_pos)
    {
      /* add diff to the earliest object's start pos and snap it, then get the
       * diff ticks */
      Position earliest_obj_new_pos = *self->earliest_obj_start_pos;
      earliest_obj_new_pos.add_ticks (self->curr_ticks_diff_from_start);

      if (earliest_obj_new_pos <= Position ())
        {
          /* stop at 0.0.0.0 */
          earliest_obj_new_pos = Position ();
        }
      else if (!self->shift_held && self->snap_grid->any_snap ())
        {
          Track * track_for_snap = NULL;
          if (self->type == ArrangerWidgetType::Timeline)
            {
              track_for_snap = timeline_arranger_widget_get_track_at_y (
                self, self->start_y + offset_y);
            }
          earliest_obj_new_pos.snap (
            self->earliest_obj_start_pos.get (), track_for_snap, nullptr,
            *self->snap_grid);
        }
      self->adj_ticks_diff = Position::get_ticks_diff (
        earliest_obj_new_pos, *self->earliest_obj_start_pos, nullptr);
    }

  /* if right clicking, start erasing action */
  if (
    self->drag_start_btn == GDK_BUTTON_SECONDARY && P_TOOL == Tool::Select
    && self->action != UiOverlayAction::STARTING_ERASING
    && self->action != UiOverlayAction::ERASING)
    {
      self->action = UiOverlayAction::STARTING_ERASING;
    }

  /* set action to selecting if starting selection. this is because drag_update
   * never gets called if it's just a click, so we can check at drag_end and see
   * if anything was selected */
  switch (self->action)
    {
    case UiOverlayAction::STARTING_SELECTION:
      self->action = UiOverlayAction::SELECTING;
      break;
    case UiOverlayAction::STARTING_DELETE_SELECTION:
      self->action = UiOverlayAction::DELETE_SELECTING;
      {
        sel->clear (false);
        self->sel_to_delete =
          clone_unique_with_variant<ArrangerSelectionsVariant> (sel);
      }
      break;
    case UiOverlayAction::STARTING_ERASING:
      self->action = UiOverlayAction::ERASING;
      {
        sel->clear (false);
        self->sel_to_delete =
          clone_unique_with_variant<ArrangerSelectionsVariant> (sel);
      }
      break;
    case UiOverlayAction::STARTING_MOVING:
      if (self->alt_held && self->can_link)
        self->action = UiOverlayAction::MOVING_LINK;
      else if (self->ctrl_held && !sel->contains_unclonable_object ())
        self->action = UiOverlayAction::MovingCopy;
      else
        self->action = UiOverlayAction::MOVING;
      break;
    case UiOverlayAction::StartingPanning:
      self->action = UiOverlayAction::Panning;
      break;
    case UiOverlayAction::MOVING:
      if (self->alt_held && self->can_link)
        self->action = UiOverlayAction::MOVING_LINK;
      else if (self->ctrl_held && !sel->contains_unclonable_object ())
        self->action = UiOverlayAction::MovingCopy;
      break;
    case UiOverlayAction::MOVING_LINK:
      if (!self->alt_held)
        self->action =
          self->ctrl_held ? UiOverlayAction::MovingCopy : UiOverlayAction::MOVING;
      break;
    case UiOverlayAction::MovingCopy:
      if (!self->ctrl_held)
        self->action =
          self->alt_held && self->can_link
            ? UiOverlayAction::MOVING_LINK
            : UiOverlayAction::MOVING;
      break;
    case UiOverlayAction::STARTING_RAMP:
      self->action = UiOverlayAction::RAMPING;
      if (self->type == ArrangerWidgetType::MidiModifier)
        {
          midi_modifier_arranger_widget_set_start_vel (self);
        }
      break;
    case UiOverlayAction::CUTTING:
      /* alt + move changes the action from
       * cutting to moving-link */
      if (self->alt_held && self->can_link)
        self->action = UiOverlayAction::MOVING_LINK;
      break;
    case UiOverlayAction::STARTING_AUDITIONING:
      self->action = UiOverlayAction::AUDITIONING;
    default:
      break;
    }

  switch (self->action)
    {
    /* if drawing a selection */
    case UiOverlayAction::SELECTING:
      {
        /* find and select objects inside selection */
        select_in_range (
          self, offset_x, offset_y, F_IN_RANGE, F_IGNORE_FROZEN, F_NO_DELETE);

        /* redraw new selections and other needed
         * things */
        EVENTS_PUSH (EventType::ET_SELECTING_IN_ARRANGER, self);
      }
      break;
    case UiOverlayAction::DELETE_SELECTING:
      /* find and delete objects inside
       * selection */
      select_in_range (
        self, offset_x, offset_y, F_IN_RANGE, F_IGNORE_FROZEN, F_DELETE);
      EVENTS_PUSH (EventType::ET_SELECTING_IN_ARRANGER, self);
      break;
    case UiOverlayAction::ERASING:
      /* delete anything touched */
      select_in_range (
        self, offset_x, offset_y, F_NOT_IN_RANGE, F_IGNORE_FROZEN, F_DELETE);
      break;
    case UiOverlayAction::ResizingLFade:
    case UiOverlayAction::ResizingLLoop:
      /* snap selections based on new pos */
      if (self->type == ArrangerWidgetType::Timeline)
        {
          bool success =
            snap_selections_during_resize (self, &self->curr_pos, true);
          if (success)
            snap_selections_during_resize (self, &self->curr_pos, false);
        }
      else if (self->type == ArrangerWidgetType::Audio)
        {
          bool success = audio_arranger_widget_snap_fade (
            self, self->curr_pos, true, F_DRY_RUN);
          if (success)
            audio_arranger_widget_snap_fade (
              self, self->curr_pos, true, F_NOT_DRY_RUN);
        }
      break;
    case UiOverlayAction::ResizingL:
    case UiOverlayAction::StretchingL:
      {
        /* snap selections based on new pos */
        if (self->type == ArrangerWidgetType::Timeline)
          {
            bool success =
              snap_selections_during_resize (self, &self->curr_pos, true);
            if (success)
              snap_selections_during_resize (self, &self->curr_pos, 0);
          }
        else if (self->type == ArrangerWidgetType::Midi)
          {
            bool success =
              snap_selections_during_resize (self, &self->curr_pos, true);
            if (success)
              snap_selections_during_resize (self, &self->curr_pos, false);
          }
      }
      break;
    case UiOverlayAction::ResizingRFade:
    case UiOverlayAction::ResizingRLoop:
      if (self->type == ArrangerWidgetType::Timeline)
        {
          if (self->resizing_range)
            timeline_arranger_widget_snap_range_r (self, &self->curr_pos);
          else
            {
              bool success =
                snap_selections_during_resize (self, &self->curr_pos, true);
              if (success)
                snap_selections_during_resize (self, &self->curr_pos, false);
            }
        }
      else if (self->type == ArrangerWidgetType::Audio)
        {
          bool success =
            audio_arranger_widget_snap_fade (self, self->curr_pos, false, true);
          if (success)
            audio_arranger_widget_snap_fade (self, self->curr_pos, false, false);
        }
      break;
    case UiOverlayAction::ResizingR:
    case UiOverlayAction::StretchingR:
    case UiOverlayAction::CreatingResizingR:
      {
        if (self->type == ArrangerWidgetType::Timeline)
          {
            if (self->resizing_range)
              {
                timeline_arranger_widget_snap_range_r (self, &self->curr_pos);
              }
            else
              {
                bool success =
                  snap_selections_during_resize (self, &self->curr_pos, true);
                if (success)
                  {
                    snap_selections_during_resize (self, &self->curr_pos, false);
                  }
              }
          }
        else if (self->type == ArrangerWidgetType::Midi)
          {
            bool success =
              snap_selections_during_resize (self, &self->curr_pos, true);
            if (success)
              {
                snap_selections_during_resize (self, &self->curr_pos, false);
              }
            move_items_y (self, offset_y);
          }
        else if (self->type == ArrangerWidgetType::Audio)
          {
            if (self->resizing_range)
              {
                audio_arranger_widget_snap_range_r (self, &self->curr_pos);
              }
          }

        TRANSPORT->recalculate_total_bars (sel);
      }
      break;
    case UiOverlayAction::RESIZING_UP:
      if (self->type == ArrangerWidgetType::MidiModifier)
        {
          midi_modifier_arranger_widget_resize_velocities (self, offset_y);
        }
      else if (self->type == ArrangerWidgetType::Automation)
        {
          automation_arranger_widget_resize_curves (self, offset_y);
        }
      else if (self->type == ArrangerWidgetType::Audio)
        {
          audio_arranger_widget_update_gain (self, offset_y);
        }
      break;
    case UiOverlayAction::RESIZING_UP_FADE_IN:
      if (self->type == ArrangerWidgetType::Timeline)
        {
          timeline_arranger_widget_fade_up (self, offset_y, true);
        }
      else if (self->type == ArrangerWidgetType::Audio)
        {
          audio_arranger_widget_fade_up (self, offset_y, true);
        }
      break;
    case UiOverlayAction::RESIZING_UP_FADE_OUT:
      if (self->type == ArrangerWidgetType::Timeline)
        {
          timeline_arranger_widget_fade_up (self, offset_y, false);
        }
      else if (self->type == ArrangerWidgetType::Audio)
        {
          audio_arranger_widget_fade_up (self, offset_y, false);
        }
      break;
    case UiOverlayAction::MOVING:
    case UiOverlayAction::CREATING_MOVING:
    case UiOverlayAction::MovingCopy:
    case UiOverlayAction::MOVING_LINK:
      move_items_x (self, self->adj_ticks_diff - self->last_adj_ticks_diff);
      move_items_y (self, offset_y);
      break;
    case UiOverlayAction::Panning:
      pan (self, offset_x, offset_y);
      break;
    case UiOverlayAction::AUTOFILLING:
      z_info ("autofilling");
      autofill (self, self->start_x + offset_x, self->start_y + offset_y);
      break;
    case UiOverlayAction::AUDITIONING:
      z_debug ("auditioning");
      break;
    case UiOverlayAction::RAMPING:
      /* find and select objects inside selection */
      if (self->type == ArrangerWidgetType::MidiModifier)
        {
          midi_modifier_arranger_widget_ramp (self, offset_x, offset_y);
        }
      break;
    case UiOverlayAction::CUTTING:
      /* nothing to do, wait for drag end */
      break;
    default:
      /* TODO */
      break;
    }

  if (self->action != UiOverlayAction::None)
    auto_scroll (
      self, (int) (self->start_x + offset_x), (int) (self->start_y + offset_y));

  if (self->type == ArrangerWidgetType::Midi)
    {
      midi_arranger_listen_notes (self, 1);
    }

  /* update last offsets */
  self->last_offset_x = offset_x;
  self->last_offset_y = offset_y;
  self->last_adj_ticks_diff = self->adj_ticks_diff;

  arranger_widget_refresh_cursor (self);
}

void
arranger_widget_handle_erase_action (ArrangerWidget * self)
{
  if (self->sel_to_delete)
    {
      if (
        self->sel_to_delete->has_any ()
        && !self->sel_to_delete->contains_undeletable_object ())
        {
          try
            {
              UNDO_MANAGER->perform (
                std::make_unique<DeleteArrangerSelectionsAction> (
                  *self->sel_to_delete));
            }
          catch (ZrythmException &e)
            {
              e.handle (_ ("Failed to delete selections"));
            }
        }
      self->sel_to_delete.reset ();
    }
}

static void
drag_end (
  GtkGestureDrag * gesture,
  gdouble          offset_x,
  gdouble          offset_y,
  ArrangerWidget * self)
{
  z_debug ("arranger drag end starting...");

  if (
    self->action == UiOverlayAction::SELECTING
    || self->action == UiOverlayAction::DELETE_SELECTING)
    {
      EVENTS_PUSH (EventType::ET_SELECTING_IN_ARRANGER, self);
    }

#undef ON_DRAG_END

  /* if something was clicked with ctrl without
   * moving */
  if (
    self->action == UiOverlayAction::STARTING_MOVING && self->start_object
    && self->ctrl_held)
    {
      /* if was selected, deselect it */
      if (self->start_object_was_selected)
        {
          self->start_object->select (F_NO_SELECT, true, true);
          z_debug ("ctrl-deselecting object");
        }
      /* if was deselected, select it */
      else
        {
          /* select it */
          self->start_object->select (true, true, true);
          z_debug ("ctrl-selecting object");
        }
    }

  /* handle click without drag for delete-selecting */
  if (
    (self->action == UiOverlayAction::STARTING_DELETE_SELECTION
     || self->action == UiOverlayAction::STARTING_ERASING)
    && self->drag_start_btn == GDK_BUTTON_PRIMARY)
    {
      self->action = UiOverlayAction::DELETE_SELECTING;
      ArrangerSelections * sel = arranger_widget_get_selections (self);
      z_return_if_fail (sel);
      sel->clear (false);
      self->sel_to_delete =
        clone_unique_with_variant<ArrangerSelectionsVariant> (sel);
      select_in_range (
        self, offset_x, offset_y, F_IN_RANGE, F_IGNORE_FROZEN, F_DELETE);
    }

  /* handle audition stop */
  if (
    self->action == UiOverlayAction::STARTING_AUDITIONING
    || self->action == UiOverlayAction::AUDITIONING)
    {
      if (self->was_paused)
        {
          TRANSPORT->request_pause (true);
        }
      TRANSPORT->set_playhead_pos (self->playhead_pos_at_start);
    }

  switch (self->type)
    {
    case ArrangerWidgetType::Timeline:
      timeline_arranger_on_drag_end (self);
      break;
    case ArrangerWidgetType::Audio:
      audio_arranger_on_drag_end (self);
      break;
    case ArrangerWidgetType::Midi:
      midi_arranger_on_drag_end (self);
      break;
    case ArrangerWidgetType::MidiModifier:
      midi_modifier_arranger_on_drag_end (self);
      break;
    case ArrangerWidgetType::Chord:
      chord_arranger_on_drag_end (self);
      break;
    case ArrangerWidgetType::Automation:
      automation_arranger_on_drag_end (self);
      break;
    default:
      break;
    }

  /* if right click, show context */
  if (gesture)
    {
      GdkEventSequence * sequence =
        gtk_gesture_single_get_current_sequence (GTK_GESTURE_SINGLE (gesture));
      GdkEvent * ev =
        gtk_gesture_get_last_event (GTK_GESTURE (gesture), sequence);
      guint btn;
      if (
        ev
        && (GDK_IS_EVENT_TYPE (ev, GDK_BUTTON_PRESS) || GDK_IS_EVENT_TYPE (ev, GDK_BUTTON_RELEASE)))
        {
          btn = gdk_button_event_get_button (ev);
          z_warn_if_fail (btn);
          if (
            btn == GDK_BUTTON_SECONDARY
            && self->action != UiOverlayAction::ERASING)
            {
              show_context_menu (
                self, self->start_x + offset_x, self->start_y + offset_y);
            }
        }
    }

  /* reset start coordinates and offsets */
  self->start_x = 0;
  self->start_y = 0;
  self->last_offset_x = 0;
  self->last_offset_y = 0;
  self->drag_update_started = false;
  self->last_adj_ticks_diff = 0;
  self->start_object.reset ();
  self->prj_start_object.reset ();
  self->hovered_object.reset ();

  self->shift_held = 0;
  self->ctrl_held = 0;
  self->alt_held = 0;

  self->sel_at_start.reset ();
  self->region_at_start.reset ();

  /* reset action */
  self->action = UiOverlayAction::None;

  /* queue redraw to hide the selection */
  /*gtk_widget_queue_draw (GTK_WIDGET (self->bg));*/

  arranger_widget_refresh_cursor (self);

  z_debug ("arranger drag end done");
}

/**
 * To be called after using arranger_widget_create_item() in
 * an action (ie, not from click + drag interaction with the
 * arranger) to finish the action.
 *
 * @return Whether an action was performed.
 */
bool
arranger_widget_finish_creating_item_from_action (
  ArrangerWidget * self,
  double           x,
  double           y)
{
  bool has_action = self->action != UiOverlayAction::None;

  drag_end (nullptr, x, y, self);

  return has_action;
}

ArrangerObject *
arranger_widget_get_hit_arranger_object (
  ArrangerWidget *     self,
  ArrangerObject::Type type,
  double               x,
  double               y)
{
  std::vector<ArrangerObject *> objs;
  arranger_widget_get_hit_objects_at_point (self, type, x, y, objs);
  return objs.size () > 0 ? objs.front () : nullptr;
}

int
arranger_widget_pos_to_px (
  ArrangerWidget * self,
  const Position   pos,
  bool             use_padding)
{
  if (self->type == ArrangerWidgetType::Timeline)
    {
      return ui_pos_to_px_timeline (pos, use_padding);
    }
  else
    {
      return ui_pos_to_px_editor (pos, use_padding);
    }

  z_return_val_if_reached (-1);
}

template <typename T>
  requires std::derived_from<T, ArrangerSelections>
T *
arranger_widget_get_selections (ArrangerWidget * self)
{
  ArrangerSelections * sel = nullptr;
  switch (self->type)
    {
    case ArrangerWidgetType::Timeline:
      sel = TL_SELECTIONS.get ();
      break;
    case ArrangerWidgetType::Midi:
    case ArrangerWidgetType::MidiModifier:
      sel = MIDI_SELECTIONS.get ();
      break;
    case ArrangerWidgetType::Automation:
      sel = AUTOMATION_SELECTIONS.get ();
      break;
    case ArrangerWidgetType::Chord:
      sel = CHORD_SELECTIONS.get ();
      break;
    case ArrangerWidgetType::Audio:
      sel = AUDIO_SELECTIONS.get ();
      break;
    default:
      z_error ("should not be reached");
      sel = TL_SELECTIONS.get ();
      break;
    }
  return static_cast<T *> (sel);
}

/**
 * Get all objects currently present in the arranger.
 */
void
arranger_widget_get_all_objects (
  ArrangerWidget *               self,
  std::vector<ArrangerObject *> &objs_arr)
{
  RulerWidget * ruler = arranger_widget_get_ruler (self);
  GdkRectangle  rect = {
    0,
    0,
    (int) ruler->total_px,
    arranger_widget_get_total_height (self),
  };

  arranger_widget_get_hit_objects_in_rect (
    self, ArrangerObject::Type::All, &rect, objs_arr);
}

/**
 * Returns the total height (including off-screen).
 */
int
arranger_widget_get_total_height (ArrangerWidget * self)
{
  int height = 0;
  switch (self->type)
    {
    case ArrangerWidgetType::Timeline:
      if (self->is_pinned)
        {
          height = gtk_widget_get_height (GTK_WIDGET (self));
        }
      else
        {
          height = gtk_widget_get_height (GTK_WIDGET (MW_TRACKLIST));
        }
      break;
    case ArrangerWidgetType::Midi:
      height = gtk_widget_get_height (GTK_WIDGET (MW_PIANO_ROLL_KEYS));
      break;
    default:
      height = gtk_widget_get_height (GTK_WIDGET (self));
      break;
    }

  return height;
}

RulerWidget *
arranger_widget_get_ruler (ArrangerWidget * self)
{
  return self->type == ArrangerWidgetType::Timeline ? MW_RULER : EDITOR_RULER;
}

/**
 * Returns the current visible rectangle.
 *
 * @param rect The rectangle to fill in.
 */
void
arranger_widget_get_visible_rect (ArrangerWidget * self, GdkRectangle * rect)
{
  auto settings = arranger_widget_get_editor_setting_values (self);
  rect->x = settings->scroll_start_x_;
  rect->y = settings->scroll_start_y_;
  rect->width = gtk_widget_get_width (GTK_WIDGET (self));
  rect->height = gtk_widget_get_height (GTK_WIDGET (self));
}

bool
arranger_widget_is_playhead_visible (ArrangerWidget * self)
{
  GdkRectangle rect;
  arranger_widget_get_visible_rect (self, &rect);

  int playhead_x = arranger_widget_get_playhead_px (self);
  int min_x = MIN (self->last_playhead_px, playhead_x);
  min_x = MAX (min_x - 4, rect.x);
  int max_x = MAX (self->last_playhead_px, playhead_x);
  max_x = MIN (max_x + 4, rect.x + rect.width);

  int width = max_x - min_x;

  return width >= 0;
}

static gboolean
on_scroll (
  GtkEventControllerScroll * scroll_controller,
  gdouble                    dx,
  gdouble                    dy,
  gpointer                   user_data)
{
  ArrangerWidget * self = Z_ARRANGER_WIDGET (user_data);

#if 0
  GdkEvent * event = gtk_event_controller_get_current_event (GTK_EVENT_CONTROLLER (scroll_controller));
  GdkDevice * source_device = gdk_event_get_device (event);
  GdkInputSource input_source = gdk_device_get_source (source_device);
  /* adjust for scroll unit */
  /* TODO */
  GdkScrollUnit scroll_unit = gtk_event_controller_scroll_get_unit (scroll_controller);
  if (scroll_unit == GDK_SCROLL_UNIT_WHEEL)
    {
      z_debug ("wheel");
      /*double page_size =*/
      /*double scroll_step = pow (page_size, 2.0 / 3.0);*/
    }
  else if (scroll_unit == GDK_SCROLL_UNIT_SURFACE)
    {
      z_debug ("unit interface");
      dx *= MAGIC_SCROLL_FACTOR;
    }
#endif

  z_debug (
    "scrolled to %.1f (d %d), %.1f (d %d)", self->hover_x, (int) dx,
    self->hover_y, (int) dy);

  EVENTS_PUSH (EventType::ET_ARRANGER_SCROLLED, self);

  GdkModifierType modifier_type = gtk_event_controller_get_current_event_state (
    GTK_EVENT_CONTROLLER (scroll_controller));

  bool ctrl_held = modifier_type & GDK_CONTROL_MASK;
  if (ctrl_held)
    {
      /* if shift also pressed, handle vertical zoom */
      if (modifier_type & GDK_SHIFT_MASK)
        {
          if (self->type == ArrangerWidgetType::Midi)
            {
              midi_arranger_handle_vertical_zoom_scroll (
                self, scroll_controller, dy);
            }
          else if (self->type == ArrangerWidgetType::Timeline)
            {
              tracklist_widget_handle_vertical_zoom_scroll (
                MW_TRACKLIST, scroll_controller, dy);
            }

          /* TODO also update hover y since we're using it here */
          /*self->hover_y = new_y;*/
        }
      /* else if just control pressed handle horizontal zoom */
      else
        {
          RulerWidget * ruler = arranger_widget_get_ruler (self);

          ruler_widget_handle_horizontal_zoom (ruler, &self->hover_x, dy);
        }
    }
  else /* else if not ctrl held */
    {
      /* scroll normally */
      const int scroll_amt = RW_SCROLL_SPEED;
      int       scroll_x = 0;
      int       scroll_y = 0;

      /* if scrolling on x-axis from a touch pad */
      if (!math_doubles_equal (dx, 0.0))
        {
          scroll_x = (int) dx;
        }
      else if (modifier_type & GDK_SHIFT_MASK)
        {
          scroll_x = (int) dy;
          scroll_y = 0;
        }
      else
        {
          scroll_x = 0;

          /* if not midi modifier and not pinned timeline,
           * handle the scroll, otherwise ignore */
          if (
            self->type != ArrangerWidgetType::MidiModifier
            && !(self->type == ArrangerWidgetType::Timeline && self->is_pinned))
            {
              scroll_y = (int) dy;
            }
        }
      auto settings = arranger_widget_get_editor_settings (self);
      std::visit (
        [&] (auto &&s) {
          scroll_x = scroll_x * scroll_amt;
          scroll_y = scroll_y * scroll_amt;
          int scroll_x_before = s->scroll_start_x_;
          int scroll_y_before = s->scroll_start_y_;
          s->append_scroll (scroll_x, scroll_y, true);

          /* also adjust the drag offsets (in case we are currently dragging */
          scroll_x = s->scroll_start_x_ - scroll_x_before;
          scroll_y = s->scroll_start_y_ - scroll_y_before;
          self->offset_x_from_scroll += scroll_x;
          self->offset_y_from_scroll += scroll_y;

          /* auto-scroll linked widgets */
          if (scroll_y != 0)
            {
              if (self->type == ArrangerWidgetType::Timeline && !self->is_pinned)
                {
                  tracklist_widget_set_unpinned_scroll_start_y (
                    MW_TRACKLIST, s->scroll_start_y_);
                }
              else if (self->type == ArrangerWidgetType::Midi)
                {
                  midi_editor_space_widget_set_piano_keys_scroll_start_y (
                    MW_MIDI_EDITOR_SPACE, s->scroll_start_y_);
                }
              else if (self->type == ArrangerWidgetType::Chord)
                {
                  chord_editor_space_widget_set_chord_keys_scroll_start_y (
                    MW_CHORD_EDITOR_SPACE, s->scroll_start_y_);
                }
            }
        },
        settings);
    }

  return true;
}

static void
on_leave (GtkEventControllerMotion * motion_controller, ArrangerWidget * self)
{
  switch (self->type)
    {
    case ArrangerWidgetType::Timeline:
      timeline_arranger_widget_set_cut_lines_visible (self);
      break;
    case ArrangerWidgetType::Chord:
      self->hovered_chord_index = -1;
      break;
    case ArrangerWidgetType::Midi:
      midi_arranger_widget_set_hovered_note (self, -1);
      break;
    default:
      break;
    }

  self->hovered = false;
}

/**
 * Motion callback.
 */
static void
on_motion (
  GtkEventControllerMotion * motion_controller,
  gdouble                    x,
  gdouble                    y,
  ArrangerWidget *           self)
{
  auto settings = arranger_widget_get_editor_setting_values (self);
  x += settings->scroll_start_x_;
  y += settings->scroll_start_y_;
  x = MAX (x, 0.0);
  y = MAX (y, 0.0);
  self->hover_x = x;
  self->hover_y = y;

  /*GdkModifierType state = gtk_event_controller_get_current_event_state (*/
  /*GTK_EVENT_CONTROLLER (motion_controller));*/

  /* highlight hovered object */
  ArrangerObject * obj = arranger_widget_get_hit_arranger_object (
    self, ArrangerObject::Type::All, self->hover_x, self->hover_y);
  if (obj && obj->is_frozen ())
    {
      obj = NULL;
    }
  self->hovered_object = obj ? obj->shared_from_this () : nullptr;

  if (
    self->action == UiOverlayAction::CUTTING && !self->alt_held
    && P_TOOL != Tool::Cut)
    {
      self->action = UiOverlayAction::None;
    }

  arranger_widget_refresh_cursor (self);

  switch (self->type)
    {
    case ArrangerWidgetType::Timeline:
      timeline_arranger_widget_set_cut_lines_visible (self);
      break;
    case ArrangerWidgetType::Chord:
      self->hovered_chord_index = chord_arranger_widget_get_chord_at_y (y);
      break;
    case ArrangerWidgetType::Midi:
      midi_arranger_widget_set_hovered_note (
        self, piano_roll_keys_widget_get_key_from_y (MW_PIANO_ROLL_KEYS, y));
      break;
    default:
      break;
    }

  self->hovered = true;
}

static void
on_focus_leave (GtkEventControllerFocus * focus, ArrangerWidget * self)
{
  z_debug ("arranger focus out");

  self->alt_held = 0;
  self->ctrl_held = 0;
  self->shift_held = 0;
}

Position
arranger_widget_px_to_pos (ArrangerWidget * self, double px, bool has_padding)
{
  if (self->type == ArrangerWidgetType::Timeline)
    {
      return ui_px_to_pos_timeline (px, has_padding);
    }
  else
    {
      return ui_px_to_pos_editor (px, has_padding);
    }
}

ArrangerCursor
arranger_widget_get_cursor (ArrangerWidget * self)
{
  ArrangerCursor ac = ArrangerCursor::Select;

  switch (self->type)
    {
    case ArrangerWidgetType::Timeline:
      ac = timeline_arranger_widget_get_cursor (self, P_TOOL);
      break;
    case ArrangerWidgetType::Audio:
      ac = audio_arranger_widget_get_cursor (self, P_TOOL);
      break;
    case ArrangerWidgetType::Chord:
      ac = chord_arranger_widget_get_cursor (self, P_TOOL);
      break;
    case ArrangerWidgetType::Midi:
      ac = midi_arranger_widget_get_cursor (self, P_TOOL);
      break;
    case ArrangerWidgetType::MidiModifier:
      ac = midi_modifier_arranger_widget_get_cursor (self, P_TOOL);
      break;
    case ArrangerWidgetType::Automation:
      ac = automation_arranger_widget_get_cursor (self, P_TOOL);
      break;
    default:
      break;
    }

  return ac;
}

/**
 * Figures out which cursor should be used based
 * on the current state and then sets it.
 */
void
arranger_widget_refresh_cursor (ArrangerWidget * self)
{
  if (!gtk_widget_get_realized (GTK_WIDGET (self)))
    return;

  ArrangerCursor ac = arranger_widget_get_cursor (self);

  arranger_widget_set_cursor (self, ac);
}

/**
 * Toggles the mute status of the selection, based
 * on the mute status of the selected object.
 *
 * This creates an undoable action and executes it.
 */
void
arranger_widget_toggle_selections_muted (
  ArrangerWidget * self,
  ArrangerObject * clicked_object)
{
  z_return_if_fail (clicked_object->can_mute ());

  GAction * action =
    g_action_map_lookup_action (G_ACTION_MAP (MAIN_WINDOW), "mute-selection");
  GVariant * var = g_variant_new_string ("timeline");
  g_action_activate (action, var);
}

void
arranger_widget_scroll_until_obj (
  ArrangerWidget * self,
  ArrangerObject * obj,
  bool             horizontal,
  bool             up,
  bool             left,
  int              padding)
{
  auto settings = arranger_widget_get_editor_settings (self);
  std::visit (
    [&] (auto &&s) {
      z_return_if_fail (s);

      int  width = gtk_widget_get_width (GTK_WIDGET (self));
      int  height = gtk_widget_get_height (GTK_WIDGET (self));
      auto lobj = dynamic_cast<LengthableObject *> (obj);
      if (horizontal)
        {
          int start_px = arranger_widget_pos_to_px (self, obj->pos_, 1);
          int end_px =
            obj->has_length ()
              ? arranger_widget_pos_to_px (self, lobj->end_pos_, true)
              : start_px;

          /* adjust px for objects with non-global positions */
          if (!ArrangerObject::type_has_global_pos (obj->type_))
            {
              auto region = CLIP_EDITOR->get_region ();
              z_return_if_fail (region);
              int tmp_px = arranger_widget_pos_to_px (self, region->pos_, true);
              start_px += tmp_px;
              end_px += tmp_px;
            }

          if (
            start_px <= s->scroll_start_x_
            || end_px >= s->scroll_start_x_ + width)
            {
              if (left)
                {
                  s->set_scroll_start_x (start_px - padding, false);
                }
              else
                {
                  int tmp = (end_px + padding) - width;
                  s->set_scroll_start_x (tmp, false);
                }
            }
        }
      else
        {
          arranger_object_set_full_rectangle (obj, self);
          int start_px = obj->full_rect_.y;
          int end_px = obj->full_rect_.y + obj->full_rect_.height;
          if (
            start_px <= s->scroll_start_y_
            || end_px >= s->scroll_start_y_ + height)
            {
              if (up)
                {
                  s->set_scroll_start_y (start_px - padding, false);
                }
              else
                {
                  int tmp = (end_px + padding) - height;
                  s->set_scroll_start_y (tmp, false);
                }
            }
        }
    },
    settings);
}

/**
 * Returns whether any arranger is in the middle
 * of an action.
 */
bool
arranger_widget_any_doing_action ()
{
#define CHECK_ARRANGER(arranger) \
  if (arranger && arranger->action != UiOverlayAction::None) \
    return true;

  CHECK_ARRANGER (MW_TIMELINE);
  CHECK_ARRANGER (MW_PINNED_TIMELINE);
  CHECK_ARRANGER (MW_MIDI_ARRANGER);
  CHECK_ARRANGER (MW_MIDI_MODIFIER_ARRANGER);
  CHECK_ARRANGER (MW_CHORD_ARRANGER);
  CHECK_ARRANGER (MW_AUTOMATION_ARRANGER);
  CHECK_ARRANGER (MW_AUDIO_ARRANGER);

#undef CHECK_ARRANGER

  return false;
}

void
arranger_widget_get_min_possible_position (ArrangerWidget * self, Position * pos)
{
  switch (self->type)
    {
    case ArrangerWidgetType::Timeline:
      pos->set_to_bar (1);
      break;
    case ArrangerWidgetType::Midi:
    case ArrangerWidgetType::MidiModifier:
    case ArrangerWidgetType::Chord:
    case ArrangerWidgetType::Automation:
    case ArrangerWidgetType::Audio:
      {
        auto region = CLIP_EDITOR->get_region ();
        z_return_if_fail (region);
        *pos = region->pos_;
        pos->change_sign ();
      }
      break;
    }
}

void
arranger_widget_handle_playhead_auto_scroll (ArrangerWidget * self, bool force)
{
  if (!TRANSPORT->is_rolling () && !force)
    return;

  bool scroll_edges = false;
  bool follow = false;
  if (self->type == ArrangerWidgetType::Timeline)
    {
      scroll_edges =
        g_settings_get_boolean (S_UI, "timeline-playhead-scroll-edges");
      follow = g_settings_get_boolean (S_UI, "timeline-playhead-follow");
    }
  else
    {
      scroll_edges =
        g_settings_get_boolean (S_UI, "editor-playhead-scroll-edges");
      follow = g_settings_get_boolean (S_UI, "editor-playhead-follow");
    }

  GdkRectangle rect;
  arranger_widget_get_visible_rect (self, &rect);

  int buffer = 5;
  int playhead_x = arranger_widget_get_playhead_px (self);

  auto settings = arranger_widget_get_editor_settings (self);
  std::visit (
    [&] (auto &&s) {
      if (follow)
        {
          /* scroll */
          s->set_scroll_start_x (playhead_x - rect.width / 2, true);
          z_debug ("autoscrolling to follow playhead");
        }
      else if (scroll_edges)
        {
          buffer = 32;
          /* if playhead is after the visible range + buffer or if before
           * visible range - buffer, scroll */
          if (
            playhead_x > ((rect.x + rect.width) - buffer)
            || playhead_x < rect.x + buffer)
            {
              s->set_scroll_start_x (playhead_x - buffer, true);
              /*z_debug ("autoscrolling at playhead edges");*/
            }
        }
    },
    settings);
}

static gboolean
arranger_tick_cb (
  GtkWidget *     widget,
  GdkFrameClock * frame_clock,
  gpointer        user_data)
{
  if (!gtk_widget_is_visible (widget))
    {
      return true;
    }
  ArrangerWidget * self = Z_ARRANGER_WIDGET (widget);
  self->queued_playhead_px = arranger_widget_get_playhead_px (self);

  gtk_widget_queue_draw (widget);

  /* auto scroll */
  arranger_widget_handle_playhead_auto_scroll (self, false);

  return G_SOURCE_CONTINUE;
}

/**
 * Runs the given function for each arranger.
 */
void
arranger_widget_foreach (ArrangerWidgetForeachFunc func)
{
  func (MW_TIMELINE);
  func (MW_PINNED_TIMELINE);
  func (MW_MIDI_ARRANGER);
  func (MW_MIDI_MODIFIER_ARRANGER);
  func (MW_CHORD_ARRANGER);
  func (MW_AUTOMATION_ARRANGER);
  func (MW_AUDIO_ARRANGER);
}

void
arranger_widget_setup (
  ArrangerWidget *                 self,
  ArrangerWidgetType               type,
  const std::shared_ptr<SnapGrid> &snap_grid)
{
  z_debug ("setting up arranger widget...");

  z_return_if_fail (self && type >= ArrangerWidgetType::Timeline && snap_grid);
  self->type = type;
  self->snap_grid = snap_grid;

  int icon_texture_size = 12;
  self->region_icon_texture_size = icon_texture_size;
  switch (type)
    {
    case ArrangerWidgetType::Timeline:
      /* make drag dest */
      gtk_widget_add_css_class (GTK_WIDGET (self), "timeline-arranger");
      timeline_arranger_setup_drag_dest (self);

      /* create common textures */
      self->symbolic_link_texture = z_gdk_texture_new_from_icon_name (
        "emblem-symbolic-link", icon_texture_size, icon_texture_size, 1);
      self->music_note_16th_texture = z_gdk_texture_new_from_icon_name (
        "music-note-16th", icon_texture_size, icon_texture_size, 1);
      self->fork_awesome_snowflake_texture = z_gdk_texture_new_from_icon_name (
        "fork-awesome-snowflake-o", icon_texture_size, icon_texture_size, 1);
      self->media_playlist_repeat_texture = z_gdk_texture_new_from_icon_name (
        "gnome-icon-library-media-playlist-repeat-symbolic", icon_texture_size,
        icon_texture_size, 1);
      break;
    case ArrangerWidgetType::Automation:
      gtk_widget_add_css_class (GTK_WIDGET (self), "automation-arranger");
      self->ap_layout = z_cairo_create_pango_layout_from_string (
        GTK_WIDGET (self), "8", PANGO_ELLIPSIZE_NONE, 0);
      break;
    case ArrangerWidgetType::MidiModifier:
      gtk_widget_add_css_class (GTK_WIDGET (self), "midi-modifier-arranger");
      self->vel_layout = z_cairo_create_pango_layout_from_string (
        GTK_WIDGET (self), "8", PANGO_ELLIPSIZE_NONE, 0);
      break;
    case ArrangerWidgetType::Audio:
      gtk_widget_add_css_class (GTK_WIDGET (self), "audio-arranger");
      self->audio_layout = z_cairo_create_pango_layout_from_string (
        GTK_WIDGET (self), "8", PANGO_ELLIPSIZE_NONE, 0);
    default:
      break;
    }

  self->debug_layout = z_cairo_create_pango_layout_from_string (
    GTK_WIDGET (self), "8", PANGO_ELLIPSIZE_NONE, 0);

  GtkEventControllerScroll * scroll_controller = GTK_EVENT_CONTROLLER_SCROLL (
    gtk_event_controller_scroll_new (GTK_EVENT_CONTROLLER_SCROLL_BOTH_AXES));
  g_signal_connect (
    G_OBJECT (scroll_controller), "scroll", G_CALLBACK (on_scroll), self);
  gtk_widget_add_controller (
    GTK_WIDGET (self), GTK_EVENT_CONTROLLER (scroll_controller));

  /* connect signals */
  g_signal_connect (
    G_OBJECT (self->drag), "drag-begin", G_CALLBACK (drag_begin), self);
  g_signal_connect (
    G_OBJECT (self->drag), "drag-update", G_CALLBACK (drag_update), self);
  g_signal_connect (
    G_OBJECT (self->drag), "drag-end", G_CALLBACK (drag_end), self);
  g_signal_connect (
    G_OBJECT (self->drag), "cancel", G_CALLBACK (drag_cancel), self);
  g_signal_connect (
    G_OBJECT (self->click), "pressed", G_CALLBACK (click_pressed), self);
  g_signal_connect (
    G_OBJECT (self->click), "stopped", G_CALLBACK (click_stopped), self);
  g_signal_connect (
    G_OBJECT (self->right_click), "released", G_CALLBACK (on_right_click), self);

  GtkEventControllerKey * key_controller =
    GTK_EVENT_CONTROLLER_KEY (gtk_event_controller_key_new ());
  g_signal_connect (
    G_OBJECT (key_controller), "key-pressed",
    G_CALLBACK (arranger_widget_on_key_press), self);
  g_signal_connect (
    G_OBJECT (key_controller), "key-released",
    G_CALLBACK (arranger_widget_on_key_release), self);
  gtk_widget_add_controller (
    GTK_WIDGET (self), GTK_EVENT_CONTROLLER (key_controller));

  GtkEventControllerMotion * motion_controller =
    GTK_EVENT_CONTROLLER_MOTION (gtk_event_controller_motion_new ());
  g_signal_connect (
    G_OBJECT (motion_controller), "motion", G_CALLBACK (on_motion), self);
  g_signal_connect (
    G_OBJECT (motion_controller), "leave", G_CALLBACK (on_leave), self);
  gtk_widget_add_controller (
    GTK_WIDGET (self), GTK_EVENT_CONTROLLER (motion_controller));

  GtkEventController * focus = gtk_event_controller_focus_new ();
  g_signal_connect (
    G_OBJECT (focus), "leave", G_CALLBACK (on_focus_leave), self);
  gtk_widget_add_controller (GTK_WIDGET (self), GTK_EVENT_CONTROLLER (focus));

  gtk_widget_add_tick_callback (
    GTK_WIDGET (self), arranger_tick_cb, self, nullptr);

  gtk_widget_set_focus_on_click (GTK_WIDGET (self), true);
  gtk_widget_set_focusable (GTK_WIDGET (self), true);

  z_debug ("done setting up arranger");
}

static void
dispose (ArrangerWidget * self)
{
  gtk_widget_unparent (GTK_WIDGET (self->popover_menu));
  g_source_remove_and_zero (self->unlisten_notes_timeout_id);

  G_OBJECT_CLASS (arranger_widget_parent_class)->dispose (G_OBJECT (self));
}

static void
finalize (ArrangerWidget * self)
{
  std::destroy_at (&self->vel_layout);
  std::destroy_at (&self->ap_layout);
  std::destroy_at (&self->audio_layout);
  std::destroy_at (&self->debug_layout);
  std::destroy_at (&self->snap_grid);
  std::destroy_at (&self->earliest_obj_start_pos);
  std::destroy_at (&self->start_object);
  std::destroy_at (&self->prj_start_object);
  std::destroy_at (&self->hovered_object);
  std::destroy_at (&self->sel_at_start);
  std::destroy_at (&self->region_at_start);
  std::destroy_at (&self->sel_to_delete);
  std::destroy_at (&self->fade_pos_at_start);
  std::destroy_at (&self->start_pos);
  std::destroy_at (&self->playhead_pos_at_start);
  std::destroy_at (&self->curr_pos);
  std::destroy_at (&self->end_pos);

  object_free_w_func_and_null (g_object_unref, self->symbolic_link_texture);
  object_free_w_func_and_null (g_object_unref, self->music_note_16th_texture);
  object_free_w_func_and_null (
    g_object_unref, self->fork_awesome_snowflake_texture);
  object_free_w_func_and_null (
    g_object_unref, self->media_playlist_repeat_texture);

  object_free_w_func_and_null (gsk_render_node_unref, self->loop_line_node);
  object_free_w_func_and_null (
    gsk_render_node_unref, self->clip_start_line_node);

  G_OBJECT_CLASS (arranger_widget_parent_class)->finalize (G_OBJECT (self));
}

static void
arranger_widget_class_init (ArrangerWidgetClass * _klass)
{
  GObjectClass * oklass = G_OBJECT_CLASS (_klass);
  oklass->finalize = (GObjectFinalizeFunc) finalize;
  oklass->dispose = (GObjectFinalizeFunc) dispose;

  GtkWidgetClass * wklass = GTK_WIDGET_CLASS (_klass);
  wklass->snapshot = arranger_snapshot;
  gtk_widget_class_set_accessible_role (wklass, GTK_ACCESSIBLE_ROLE_GROUP);

  gtk_widget_class_set_layout_manager_type (wklass, GTK_TYPE_BIN_LAYOUT);
  gtk_widget_class_set_css_name (wklass, "arranger");

  gtk_widget_class_add_binding (
    wklass, GDK_KEY_space, static_cast<GdkModifierType> (0),
    z_gtk_simple_action_shortcut_func, "s", "play-pause", nullptr);
  gtk_widget_class_add_binding (
    wklass, GDK_KEY_space, GDK_SHIFT_MASK, z_gtk_simple_action_shortcut_func,
    "s", "record-play", nullptr);
  gtk_widget_class_add_binding (
    wklass, GDK_KEY_1, static_cast<GdkModifierType> (0),
    z_gtk_simple_action_shortcut_func, "s", "select-mode", nullptr);
  gtk_widget_class_add_binding (
    wklass, GDK_KEY_2, static_cast<GdkModifierType> (0),
    z_gtk_simple_action_shortcut_func, "s", "edit-mode", nullptr);
  gtk_widget_class_add_binding (
    wklass, GDK_KEY_3, static_cast<GdkModifierType> (0),
    z_gtk_simple_action_shortcut_func, "s", "cut-mode", nullptr);
  gtk_widget_class_add_binding (
    wklass, GDK_KEY_4, static_cast<GdkModifierType> (0),
    z_gtk_simple_action_shortcut_func, "s", "eraser-mode", nullptr);
  gtk_widget_class_add_binding (
    wklass, GDK_KEY_5, static_cast<GdkModifierType> (0),
    z_gtk_simple_action_shortcut_func, "s", "ramp-mode", nullptr);
  gtk_widget_class_add_binding (
    wklass, GDK_KEY_6, static_cast<GdkModifierType> (0),
    z_gtk_simple_action_shortcut_func, "s", "audition-mode", nullptr);
  gtk_widget_class_add_binding (
    wklass, GDK_KEY_M, GDK_SHIFT_MASK, z_gtk_simple_action_shortcut_func, "s",
    "mute-selection::global", nullptr);
  gtk_widget_class_add_binding (
    wklass, GDK_KEY_less, GDK_SHIFT_MASK, z_gtk_simple_action_shortcut_func,
    "s", "nudge-selection::left", nullptr);
  gtk_widget_class_add_binding (
    wklass, GDK_KEY_greater, GDK_SHIFT_MASK, z_gtk_simple_action_shortcut_func,
    "s", "nudge-selection::right", nullptr);
  gtk_widget_class_add_binding (
    wklass, GDK_KEY_A, GDK_CONTROL_MASK, z_gtk_simple_action_shortcut_func, "s",
    "select-all", nullptr);
  gtk_widget_class_add_binding (
    wklass, GDK_KEY_A,
    static_cast<GdkModifierType> (GDK_CONTROL_MASK | GDK_SHIFT_MASK),
    z_gtk_simple_action_shortcut_func, "s", "clear-selection", nullptr);
  gtk_widget_class_add_binding (
    wklass, GDK_KEY_Delete, static_cast<GdkModifierType> (0),
    z_gtk_simple_action_shortcut_func, "s", "delete", nullptr);
  gtk_widget_class_add_binding (
    wklass, GDK_KEY_Q, static_cast<GdkModifierType> (0),
    z_gtk_simple_action_shortcut_func, "s", "quick-quantize::global", nullptr);
  gtk_widget_class_add_binding (
    wklass, GDK_KEY_F2, static_cast<GdkModifierType> (0),
    z_gtk_simple_action_shortcut_func, "s", "rename-arranger-object", nullptr);
}

static void
arranger_widget_init (ArrangerWidget * self)
{
  std::construct_at (&self->vel_layout);
  std::construct_at (&self->ap_layout);
  std::construct_at (&self->audio_layout);
  std::construct_at (&self->debug_layout);
  std::construct_at (&self->snap_grid);
  std::construct_at (&self->earliest_obj_start_pos);
  std::construct_at (&self->start_object);
  std::construct_at (&self->prj_start_object);
  std::construct_at (&self->hovered_object);
  std::construct_at (&self->sel_at_start);
  std::construct_at (&self->region_at_start);
  std::construct_at (&self->sel_to_delete);
  std::construct_at (&self->fade_pos_at_start);
  std::construct_at (&self->start_pos);
  std::construct_at (&self->playhead_pos_at_start);
  std::construct_at (&self->curr_pos);
  std::construct_at (&self->end_pos);

  self->first_draw = true;

  gtk_accessible_update_property (
    GTK_ACCESSIBLE (self), GTK_ACCESSIBLE_PROPERTY_LABEL, "Arranger", -1);

  self->popover_menu =
    GTK_POPOVER_MENU (gtk_popover_menu_new_from_model (nullptr));
  gtk_widget_set_parent (GTK_WIDGET (self->popover_menu), GTK_WIDGET (self));

  /* make widget able to focus */
  gtk_widget_set_focus_on_click (GTK_WIDGET (self), true);

  self->drag = GTK_GESTURE_DRAG (gtk_gesture_drag_new ());
  gtk_widget_add_controller (
    GTK_WIDGET (self), GTK_EVENT_CONTROLLER (self->drag));
  gtk_event_controller_set_propagation_phase (
    GTK_EVENT_CONTROLLER (self->drag), GTK_PHASE_CAPTURE);

  /* allow all buttons for drag */
  gtk_gesture_single_set_button (GTK_GESTURE_SINGLE (self->drag), 0);

  self->click = GTK_GESTURE_CLICK (gtk_gesture_click_new ());
  /* allow all buttons */
  gtk_gesture_single_set_button (GTK_GESTURE_SINGLE (self->click), 0);
  gtk_widget_add_controller (
    GTK_WIDGET (self), GTK_EVENT_CONTROLLER (self->click));
  gtk_event_controller_set_propagation_phase (
    GTK_EVENT_CONTROLLER (self->click), GTK_PHASE_CAPTURE);

  self->right_click = GTK_GESTURE_CLICK (gtk_gesture_click_new ());
  gtk_widget_add_controller (
    GTK_WIDGET (self), GTK_EVENT_CONTROLLER (self->right_click));
  gtk_gesture_single_set_button (
    GTK_GESTURE_SINGLE (self->right_click), GDK_BUTTON_SECONDARY);

  gtk_widget_set_overflow (GTK_WIDGET (self), GTK_OVERFLOW_HIDDEN);
}

template MidiSelections *
arranger_widget_get_selections (ArrangerWidget * self);
template TimelineSelections *
arranger_widget_get_selections (ArrangerWidget * self);
template ChordSelections *
arranger_widget_get_selections (ArrangerWidget * self);
template AutomationSelections *
arranger_widget_get_selections (ArrangerWidget * self);