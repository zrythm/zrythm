// SPDX-FileCopyrightText: © 2018-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "common/dsp/arranger_object.h"
#include "common/dsp/audio_function.h"
#include "common/dsp/automation_region.h"
#include "common/dsp/chord_region.h"
#include "common/dsp/chord_track.h"
#include "common/dsp/fadeable_object.h"
#include "common/dsp/marker_track.h"
#include "common/dsp/piano_roll_track.h"
#include "common/dsp/tracklist.h"
#include "common/utils/gtk.h"
#include "common/utils/rt_thread_id.h"
#include "common/utils/ui.h"
#include "gui/backend/backend/actions/arranger_selections.h"
#include "gui/backend/backend/actions/undo_manager.h"
#include "gui/backend/backend/event.h"
#include "gui/backend/backend/event_manager.h"
#include "gui/backend/backend/project.h"
#include "gui/backend/backend/wrapped_object_with_change_signal.h"
#include "gui/backend/backend/zrythm.h"
#include "gui/backend/gtk_widgets/arranger.h"
#include "gui/backend/gtk_widgets/arranger_object.h"
#include "gui/backend/gtk_widgets/center_dock.h"
#include "gui/backend/gtk_widgets/dialogs/export_midi_file_dialog.h"
#include "gui/backend/gtk_widgets/dialogs/string_entry_dialog.h"
#include "gui/backend/gtk_widgets/gtk_wrapper.h"
#include "gui/backend/gtk_widgets/main_notebook.h"
#include "gui/backend/gtk_widgets/ruler.h"
#include "gui/backend/gtk_widgets/timeline_arranger.h"
#include "gui/backend/gtk_widgets/timeline_panel.h"
#include "gui/backend/gtk_widgets/track.h"
#include "gui/backend/gtk_widgets/tracklist.h"
#include "gui/backend/gtk_widgets/zrythm_app.h"

#include <glib/gi18n.h>

#define ACTION_IS(x) (self->action == ##x)

void
timeline_arranger_widget_set_cut_lines_visible (ArrangerWidget * self)
{
}

ArrangerCursor
timeline_arranger_widget_get_cursor (ArrangerWidget * self, Tool tool)
{
  ArrangerCursor  ac = ArrangerCursor::Select;
  UiOverlayAction action = self->action;

  auto r_obj = dynamic_cast<Region *> (arranger_widget_get_hit_arranger_object (
    self, ArrangerObject::Type::Region, self->hover_x, self->hover_y));
  auto s_obj =
    dynamic_cast<ScaleObject *> (arranger_widget_get_hit_arranger_object (
      self, ArrangerObject::Type::ScaleObject, self->hover_x, self->hover_y));
  auto m_obj = dynamic_cast<Marker *> (arranger_widget_get_hit_arranger_object (
    self, ArrangerObject::Type::Marker, self->hover_x, self->hover_y));
  if (r_obj && r_obj->is_frozen ())
    {
      r_obj = NULL;
    }
  if (s_obj && s_obj->is_frozen ())
    {
      s_obj = NULL;
    }
  if (m_obj && m_obj->is_frozen ())
    {
      m_obj = NULL;
    }
  bool is_hit = r_obj || s_obj || m_obj;

  switch (action)
    {
    case UiOverlayAction::None:
      switch (P_TOOL)
        {
        case Tool::Select:
          {
            if (is_hit)
              {
                if (r_obj)
                  {
                    if (self->alt_held)
                      return ArrangerCursor::ARRANGER_CURSOR_CUT;
                    int wx = (int) self->hover_x - r_obj->full_rect_.x;
                    int wy = (int) self->hover_y - r_obj->full_rect_.y;
                    int is_fade_in_point =
                      arranger_object_is_fade_in (r_obj, wx, wy, 1, 0);
                    int is_fade_out_point =
                      arranger_object_is_fade_out (r_obj, wx, wy, 1, 0);
                    int is_fade_in_outer_region =
                      arranger_object_is_fade_in (r_obj, wx, wy, 0, 1);
                    int is_fade_out_outer_region =
                      arranger_object_is_fade_out (r_obj, wx, wy, 0, 1);
                    int is_resize_l = arranger_object_is_resize_l (r_obj, wx);
                    int is_resize_r = arranger_object_is_resize_r (r_obj, wx);
                    int is_resize_loop = arranger_object_is_resize_loop (
                      r_obj, wy, self->ctrl_held);
                    bool is_rename = arranger_object_is_rename (r_obj, wx, wy);
                    if (is_fade_in_point)
                      return ArrangerCursor::FadeIn;
                    else if (is_fade_out_point)
                      return ArrangerCursor::FadeOut;
                    else if (is_resize_l && is_resize_loop)
                      {
                        return ArrangerCursor::ARRANGER_CURSOR_RESIZING_L_LOOP;
                      }
                    else if (is_resize_l)
                      {
                        return self->ctrl_held
                                 ? ArrangerCursor::ARRANGER_CURSOR_STRETCHING_L
                                 : ArrangerCursor::ResizingL;
                      }
                    else if (is_resize_r && is_resize_loop)
                      return ArrangerCursor::ARRANGER_CURSOR_RESIZING_R_LOOP;
                    else if (is_resize_r)
                      {
                        return self->ctrl_held
                                 ? ArrangerCursor::ARRANGER_CURSOR_STRETCHING_R
                                 : ArrangerCursor::ResizingR;
                      }
                    else if (is_fade_in_outer_region)
                      return ArrangerCursor::FadeIn;
                    else if (is_fade_out_outer_region)
                      return ArrangerCursor::FadeOut;
                    else if (is_rename)
                      return ArrangerCursor::Rename;
                  }
                return ArrangerCursor::Grab;
              }
            else
              {
                Track * track =
                  timeline_arranger_widget_get_track_at_y (self, self->hover_y);

                if (track)
                  {
                    if (track_widget_is_cursor_in_range_select_half (
                          track->widget_, self->hover_y))
                      {
                        /* set cursor to range selection */
                        return ArrangerCursor::Range;
                      }
                    else
                      {
                        /* set cursor to normal */
                        return ac;
                      }
                  }
                else
                  {
                    /* set cursor to normal */
                    return ac;
                  }
              }
          }
          break;
        case Tool::Edit:
          ac = ArrangerCursor::Edit;
          break;
        case Tool::Cut:
          ac = ArrangerCursor::ARRANGER_CURSOR_CUT;
          break;
        case Tool::Eraser:
          ac = ArrangerCursor::Eraser;
          break;
        case Tool::Ramp:
          ac = ArrangerCursor::Ramp;
          break;
        case Tool::Audition:
          ac = ArrangerCursor::Audition;
          break;
        }
      break;
    case UiOverlayAction::STARTING_DELETE_SELECTION:
    case UiOverlayAction::DELETE_SELECTING:
    case UiOverlayAction::STARTING_ERASING:
    case UiOverlayAction::ERASING:
      ac = ArrangerCursor::Eraser;
      break;
    case UiOverlayAction::STARTING_MOVING_COPY:
    case UiOverlayAction::MovingCopy:
      ac = ArrangerCursor::GrabbingCopy;
      break;
    case UiOverlayAction::STARTING_MOVING_LINK:
    case UiOverlayAction::MOVING_LINK:
      ac = ArrangerCursor::GrabbingLink;
      break;
    case UiOverlayAction::STARTING_MOVING:
    case UiOverlayAction::CREATING_MOVING:
    case UiOverlayAction::MOVING:
      ac = ArrangerCursor::Grabbing;
      break;
    case UiOverlayAction::StartingPanning:
    case UiOverlayAction::Panning:
      ac = ArrangerCursor::Panning;
      break;
    case UiOverlayAction::StretchingL:
      ac = ArrangerCursor::ARRANGER_CURSOR_STRETCHING_L;
      break;
    case UiOverlayAction::ResizingL:
      if (self->resizing_range)
        ac = ArrangerCursor::Range;
      else
        ac = ArrangerCursor::ResizingL;
      break;
    case UiOverlayAction::ResizingLLoop:
      ac = ArrangerCursor::ARRANGER_CURSOR_RESIZING_L_LOOP;
      break;
    case UiOverlayAction::ResizingLFade:
      ac = ArrangerCursor::FadeIn;
      break;
    case UiOverlayAction::StretchingR:
      ac = ArrangerCursor::ARRANGER_CURSOR_STRETCHING_R;
      break;
    case UiOverlayAction::CreatingResizingR:
    case UiOverlayAction::ResizingR:
      if (self->resizing_range)
        ac = ArrangerCursor::Range;
      else
        ac = ArrangerCursor::ResizingR;
      break;
    case UiOverlayAction::ResizingRLoop:
      ac = ArrangerCursor::ARRANGER_CURSOR_RESIZING_R_LOOP;
      break;
    case UiOverlayAction::ResizingRFade:
      ac = ArrangerCursor::FadeOut;
      break;
    case UiOverlayAction::RESIZING_UP_FADE_IN:
      ac = ArrangerCursor::FadeIn;
      break;
    case UiOverlayAction::RESIZING_UP_FADE_OUT:
      ac = ArrangerCursor::FadeOut;
      break;
    case UiOverlayAction::AUTOFILLING:
      ac = ArrangerCursor::Autofill;
      break;
    case UiOverlayAction::STARTING_SELECTION:
    case UiOverlayAction::SELECTING:
      ac = ArrangerCursor::Select;
      break;
    case UiOverlayAction::RENAMING:
      ac = ArrangerCursor::Rename;
      break;
    case UiOverlayAction::CUTTING:
      ac = ArrangerCursor::ARRANGER_CURSOR_CUT;
      break;
    case UiOverlayAction::STARTING_AUDITIONING:
    case UiOverlayAction::AUDITIONING:
      ac = ArrangerCursor::Audition;
      break;
    default:
      z_warn_if_reached ();
      ac = ArrangerCursor::Select;
      break;
    }

  return ac;
}

TrackLane *
timeline_arranger_widget_get_track_lane_at_y (ArrangerWidget * self, double y)
{
  auto track = dynamic_cast<LanedTrack *> (
    timeline_arranger_widget_get_track_at_y (self, y));
  if (!track || !track->lanes_visible_)
    return NULL;

  /* y local to track */
  int y_local = track_widget_get_local_y (track->widget_, self, (int) y);

  return std::visit (
    [y_local] (auto &&t) -> TrackLane * {
      for (auto &lane : t->lanes_)
        {
          if (y_local >= lane->y_ && y_local < lane->y_ + lane->height_)
            return lane.get ();
        }
      return nullptr;
    },
    convert_to_variant<LanedTrackPtrVariant> (track));
}

Track *
timeline_arranger_widget_get_track_at_y (ArrangerWidget * self, double y)
{
  for (auto &track : TRACKLIST->tracks_)
    {
      if (
        /* ignore invisible tracks */
        !track->visible_ ||
        /* ignore tracks in the other timeline */
        self->is_pinned != track->is_pinned ())
        continue;

      if (!track->should_be_visible ())
        continue;

      if (!track->widget_)
        {
          z_debug ("no track widget for {}", track->name_);
          continue;
        }

      if (
        ui_is_child_hit (
          self->is_pinned
            ? GTK_WIDGET (self)
            : GTK_WIDGET (MW_TRACKLIST->unpinned_box),
          GTK_WIDGET (track->widget_), 0, 1, 0, y, 0, 1))
        return track.get ();
    }

  return NULL;
}

/**
 * Returns the hit AutomationTrack at y.
 */
AutomationTrack *
timeline_arranger_widget_get_at_at_y (ArrangerWidget * self, double y)
{
  Track * track = timeline_arranger_widget_get_track_at_y (self, y);
  if (!track)
    return NULL;

  /* y local to track */
  int y_local = track_widget_get_local_y (track->widget_, self, (int) y);

  return track_widget_get_at_at_y (track->widget_, y_local);
}

template <FinalRegionSubclass RegionT>
void
timeline_arranger_widget_create_region (
  ArrangerWidget * self,
  Track *          track,
  std::conditional_t<
    LaneOwnedRegionSubclass<RegionT>,
    TrackLaneImpl<RegionT> *,
    std::nullptr_t> lane,
  AutomationTrack * at,
  const Position *  pos)
{
  static_assert (RegionSubclass<RegionT>, "RegionT must be a Region subclass");

  bool autofilling = self->action == UiOverlayAction::AUTOFILLING;

  /* if autofilling, the action is already set */
  if (!autofilling)
    {
      self->action = UiOverlayAction::CreatingResizingR;
    }

  z_info ("creating region");

  Position end_pos;
  Position::set_min_size (*pos, end_pos, *self->snap_grid);

  /* create a new region */
  std::shared_ptr<RegionT> region;
  if constexpr (std::is_same_v<RegionT, MidiRegion>)
    {
      auto laned_track = dynamic_cast<PianoRollTrack *> (track);
      region = std::make_shared<MidiRegion> (
        *pos, end_pos, track->get_name_hash (),
        /* create on lane 0 if creating in main track */
        lane ? lane->pos_ : 0,
        lane ? lane->regions_.size () : laned_track->lanes_[0]->regions_.size ());
    }
  else if constexpr (std::is_same_v<RegionT, ChordRegion>)
    {
      region = std::make_shared<ChordRegion> (
        *pos, end_pos, P_CHORD_TRACK->regions_.size ());
    }
  else if constexpr (std::is_same_v<RegionT, AutomationRegion>)
    {
      z_return_if_fail (at);
      region = std::make_shared<AutomationRegion> (
        *pos, end_pos, track->get_name_hash (), at->index_,
        at->regions_.size ());
    }

  /* create region */
  std::shared_ptr<RegionT> added_region;
  if constexpr (std::is_same_v<RegionT, MidiRegion>)
    {
      auto laned_track = dynamic_cast<LanedTrackImpl<RegionT> *> (track);
      added_region = track->add_region (
        std::move (region), nullptr,
        lane
          ? lane->pos_
          : (laned_track->lanes_.size () == 1 ? 0 : laned_track->lanes_.size () - 2),
        true, true);
    }
  else if constexpr (std::is_same_v<RegionT, ChordRegion>)
    {
      added_region = P_CHORD_TRACK->Track::add_region (
        std::move (region), nullptr, -1, true, true);
    }
  else if constexpr (std::is_same_v<RegionT, AutomationRegion>)
    {
      added_region = track->add_region (std::move (region), at, -1, true, true);
    }

  added_region->set_position (
    &added_region->end_pos_, ArrangerObject::PositionType::End, false);
  ArrangerObject::select (added_region, true, autofilling, false);

  self->prj_start_object = added_region;
  self->start_object = added_region->clone_unique ();
}

void
timeline_arranger_widget_create_chord_or_scale (
  ArrangerWidget * self,
  Track *          track,
  double           y,
  const Position * pos)
{
  int track_height = gtk_widget_get_height (GTK_WIDGET (track->widget_));

  if (y >= (double) track_height / 2.0)
    {
      timeline_arranger_widget_create_scale (self, track, pos);
    }
  else
    {
      try
        {
          timeline_arranger_widget_create_region<ChordRegion> (
            self, track, nullptr, nullptr, pos);
        }
      catch (const ZrythmException &e)
        {
          e.handle (_ ("Failed to create region"));
          return;
        }
    }
}

void
timeline_arranger_widget_create_scale (
  ArrangerWidget * self,
  Track *          track,
  const Position * pos)
{
  z_return_if_fail (track->is_chord ());

  self->action = UiOverlayAction::CREATING_MOVING;

  auto descr = std::make_unique<MusicalScale> (
    MusicalScale::Type::Aeolian, MusicalNote::A);

  auto chord_track = dynamic_cast<ChordTrack *> (track);
  z_return_if_fail (chord_track);
  auto scale = chord_track->add_scale (std::make_unique<ScaleObject> (*descr));

  scale->pos_setter (pos);

  EVENTS_PUSH (EventType::ET_ARRANGER_OBJECT_CREATED, scale.get ());
  scale->select (scale, true, false, false);
}

void
timeline_arranger_widget_create_marker (
  ArrangerWidget * self,
  Track *          track,
  const Position * pos)
{
  z_return_if_fail (track->is_marker ());

  self->action = UiOverlayAction::CREATING_MOVING;

  auto marker = dynamic_cast<MarkerTrack *> (track)->add_marker (
    std::make_shared<Marker> (_ ("Custom Marker")));

  marker->pos_setter (pos);

  EVENTS_PUSH (EventType::ET_ARRANGER_OBJECT_CREATED, marker.get ());
  marker->select (marker, true, false, false);
}

void
timeline_arranger_widget_set_select_type (ArrangerWidget * self, double y)
{
  auto track = timeline_arranger_widget_get_track_at_y (self, y);

  if (track)
    {
      if (track_widget_is_cursor_in_range_select_half (track->widget_, y))
        {
          self->resizing_range = true;
          self->resizing_range_start = true;
          self->action = UiOverlayAction::ResizingR;
        }
      else
        {
          self->resizing_range = false;
        }
    }
  else
    {
      self->resizing_range = false;
    }
}

void
timeline_arranger_widget_snap_range_r (ArrangerWidget * self, Position * pos)
{
  if (self->resizing_range_start)
    {
      TRANSPORT->range_1_ = ui_px_to_pos_timeline (self->start_x, true);
      if (self->snap_grid->any_snap () && !self->shift_held)
        {
          TRANSPORT->range_1_.snap_simple (*SNAP_GRID_TIMELINE);
        }
      TRANSPORT->range_2_ = TRANSPORT->range_1_;

      MW_TIMELINE->resizing_range_start = false;
    }

  if (self->snap_grid->any_snap () && !self->shift_held)
    pos->snap_simple (*SNAP_GRID_TIMELINE);
  TRANSPORT->range_2_ = *pos;
  TRANSPORT->set_has_range (true);
}

/**
 * @param fade_in 1 for in, 0 for out.
 */
static void
create_fade_preset_menu (
  ArrangerWidget * self,
  GMenu *          menu,
  ArrangerObject * obj,
  bool             fade_in)
{
  GMenu * section = g_menu_new ();

  auto psets = CurveFadePreset::get_fade_presets ();
  for (auto &pset : psets)
    {
      char tmp[200];
      sprintf (
        tmp, "app.set-region-fade-%s-algorithm-preset::%s",
        fade_in ? "in" : "out", pset.id_.c_str ());
      GMenuItem * menuitem =
        z_gtk_create_menu_item (pset.label_.c_str (), nullptr, tmp);
      g_menu_append_item (section, menuitem);
    }

  g_menu_append_section (menu, _ ("Fade Preset"), G_MENU_MODEL (section));
}

GMenu *
timeline_arranger_widget_gen_context_menu (
  ArrangerWidget * self,
  double           x,
  double           y)
{
  GMenu *     menu = g_menu_new ();
  GMenuItem * menuitem;

  ArrangerObject * obj = arranger_widget_get_hit_arranger_object (
    (ArrangerWidget *) self, ArrangerObject::Type::All, x, y);

  if (obj)
    {
      int local_x = (int) (x - obj->full_rect_.x);
      int local_y = (int) (y - obj->full_rect_.y);

      GMenu * edit_submenu = g_menu_new ();

      /* create cut, copy, duplicate, delete */
      menuitem = CREATE_CUT_MENU_ITEM ("app.cut");
      g_menu_append_item (edit_submenu, menuitem);
      menuitem = CREATE_COPY_MENU_ITEM ("app.copy");
      g_menu_append_item (edit_submenu, menuitem);
      menuitem = CREATE_DUPLICATE_MENU_ITEM ("app.duplicate");
      g_menu_append_item (edit_submenu, menuitem);
      menuitem = CREATE_DELETE_MENU_ITEM ("app.delete");
      g_menu_append_item (edit_submenu, menuitem);

      if (obj->has_name ())
        {
          menuitem = z_gtk_create_menu_item (
            _ ("Rename"), nullptr, "app.rename-arranger-object");
          g_menu_append_item (edit_submenu, menuitem);
        }

      menuitem = z_gtk_create_menu_item (
        _ ("Loop Selection"), nullptr, "app.loop-selection");
      g_menu_append_item (edit_submenu, menuitem);

      char str[100];
      sprintf (str, "app.arranger-object-view-info::%p", obj);
      menuitem = z_gtk_create_menu_item (_ ("View info"), nullptr, str);
      g_menu_append_item (edit_submenu, menuitem);

      g_menu_append_section (menu, nullptr, G_MENU_MODEL (edit_submenu));

      if (TL_SELECTIONS->contains_only_regions ())
        {
          auto region = dynamic_cast<Region *> (obj);
          if (region->get_muted (false))
            {
              menuitem =
                CREATE_UNMUTE_MENU_ITEM ("app.mute-selection::timeline");
            }
          else
            {
              menuitem = CREATE_MUTE_MENU_ITEM ("app.mute-selection::timeline");
            }
          g_menu_append_item (menu, menuitem);

          menuitem = z_gtk_create_menu_item (
            _ ("Change Color..."), nullptr, "app.change-region-color");
          g_menu_append_item (menu, menuitem);

          menuitem = z_gtk_create_menu_item (
            _ ("Reset Color"), nullptr, "app.reset-region-color");
          g_menu_append_item (menu, menuitem);

          if (TL_SELECTIONS->contains_only_region_types (RegionType::Audio))
            {
              GMenu * audio_regions_submenu = g_menu_new ();

              if (TL_SELECTIONS->objects_.size () == 1)
                {
                  char tmp[200];
                  sprintf (
                    tmp, "app.detect-bpm::%p",
                    TL_SELECTIONS->objects_[0].get ());
                  menuitem =
                    z_gtk_create_menu_item (_ ("Detect BPM"), nullptr, tmp);
                  g_menu_append_item (audio_regions_submenu, menuitem);
                  /* create fade menus */
                  if (arranger_object_is_fade_in (obj, local_x, local_y, 0, 0))
                    {
                      create_fade_preset_menu (
                        self, audio_regions_submenu, obj, true);
                    }
                  if (arranger_object_is_fade_out (obj, local_x, local_y, 0, 0))
                    {
                      create_fade_preset_menu (
                        self, audio_regions_submenu, obj, false);
                    }

#if 0
                  /* create musical mode menu */
                  create_musical_mode_pset_menu (
                    self, menu, obj);
#endif
                }

              GMenu * region_functions_subsubmenu = g_menu_new ();

              for (
                unsigned int i = ENUM_VALUE_TO_INT (AudioFunctionType::Invert);
                i < ENUM_VALUE_TO_INT (AudioFunctionType::CustomPlugin); i++)
                {
                  if (
                    ENUM_INT_TO_VALUE (AudioFunctionType, i)
                      == AudioFunctionType::NormalizeRMS
                    || ENUM_INT_TO_VALUE (AudioFunctionType, i)
                         == AudioFunctionType::NormalizeLUFS)
                    continue;

                  auto        action_name = "app.timeline-function";
                  GMenuItem * submenu_item = z_gtk_create_menu_item (
                    AudioFunctionType_to_string (
                      ENUM_INT_TO_VALUE (AudioFunctionType, i), true)
                      .c_str (),
                    nullptr, action_name);
                  g_menu_item_set_action_and_target_value (
                    submenu_item, action_name, g_variant_new_int32 (i));
                  g_menu_append_item (region_functions_subsubmenu, submenu_item);
                }
              g_menu_append_submenu (
                audio_regions_submenu, _ ("Functions"),
                G_MENU_MODEL (region_functions_subsubmenu));
              g_menu_append_section (
                menu, _ ("Audio Regions"), G_MENU_MODEL (audio_regions_submenu));
            } /* endif contains only audio regions */

          if (TL_SELECTIONS->contains_only_region_types (RegionType::Midi))
            {
              GMenu * midi_regions_submenu = g_menu_new ();

              menuitem = z_gtk_create_menu_item (
                _ ("Export as MIDI file..."), nullptr,
                "app.export-midi-regions");
              g_menu_append_item (midi_regions_submenu, menuitem);

              g_menu_append_section (
                menu, nullptr, G_MENU_MODEL (midi_regions_submenu));
            }

          if (TL_SELECTIONS->contains_only_region_types (RegionType::Automation))
            {
              GMenu * automation_regions_submenu = g_menu_new ();

              GMenu * tracks_submenu = g_menu_new ();

              for (
                auto track : TRACKLIST->tracks_ | type_is<AutomatableTrack> ())
                {
                  auto &atl = track->get_automation_tracklist ();

                  GMenu * ats_submenu = g_menu_new ();

                  for (auto &at : atl.ats_)
                    {
                      if (!at->created_)
                        continue;

                      auto port =
                        Port::find_from_identifier<ControlPort> (at->port_id_);

                      char tmp[200];
                      sprintf (tmp, "app.move-automation-regions::%p", port);
                      GMenuItem * submenu_item = z_gtk_create_menu_item (
                        port->id_.label_.c_str (), nullptr, tmp);
                      g_menu_append_item (ats_submenu, submenu_item);
                    }

                  g_menu_append_submenu (
                    tracks_submenu, track->name_.c_str (),
                    G_MENU_MODEL (ats_submenu));
                }
              g_menu_append_submenu (
                automation_regions_submenu, _ ("Move to Lane"),
                G_MENU_MODEL (tracks_submenu));
              g_menu_append_section (
                menu, _ ("Automation Region"),
                G_MENU_MODEL (automation_regions_submenu));
            } /* endif contains only automation regions */

          GMenu * bounce_submenu = g_menu_new ();

          menuitem = z_gtk_create_menu_item (
            _ ("Quick Bounce"), nullptr, "app.quick-bounce-selections");
          g_menu_append_item (bounce_submenu, menuitem);

          menuitem = z_gtk_create_menu_item (
            _ ("Bounce..."), nullptr, "app.bounce-selections");
          g_menu_append_item (bounce_submenu, menuitem);

          g_menu_append_section (menu, nullptr, G_MENU_MODEL (bounce_submenu));
        }
    }
  else /* else if no object clicked */
    {
      GMenu * edit_submenu = g_menu_new ();

      menuitem = CREATE_PASTE_MENU_ITEM ("app.paste");
      g_menu_append_item (edit_submenu, menuitem);

      g_menu_append_section (menu, nullptr, G_MENU_MODEL (edit_submenu));
    }

  return menu;
}

void
timeline_arranger_widget_fade_up (
  ArrangerWidget * self,
  double           offset_y,
  bool             fade_in)
{
  for (auto region : TL_SELECTIONS->objects_ | type_is<FadeableObject> ())
    {
      auto prj_region =
        dynamic_pointer_cast<FadeableObject> (region->find_in_project ());
      z_return_if_fail (prj_region);

      auto &opts =
        fade_in ? prj_region->fade_in_opts_ : prj_region->fade_out_opts_;
      constexpr double sensitivity = 0.008;
      double           delta = (self->last_offset_y - offset_y) * sensitivity;
      auto             new_curviness =
        std::clamp<double> (opts.curviness_ + delta, -1.0, 1.0);
      opts.curviness_ = new_curviness;

      /* update selections as well */
      opts = fade_in ? region->fade_in_opts_ : region->fade_out_opts_;
      opts.curviness_ = new_curviness;
    }
}

static void
highlight_timeline (
  ArrangerWidget * self,
  GdkModifierType  mask,
  int              x,
  int              y,
  Track *          track,
  TrackLane *      lane)
{
  /* get default size */
  int      ticks = SNAP_GRID_TIMELINE->get_default_ticks ();
  Position length_pos (static_cast<double> (ticks));
  int      width_px = ui_pos_to_px_timeline (length_pos, false);

  /* get snapped x */
  Position pos = ui_px_to_pos_timeline (x, true);
  if (!(mask & GDK_SHIFT_MASK))
    {
      pos.snap_simple (*SNAP_GRID_TIMELINE);
      x = ui_pos_to_px_timeline (pos, true);
    }

  int height = TRACK_DEF_HEIGHT;
  /* if track, get y/height inside track */
  if (track)
    {
      height = (int) track->main_height_;
      int track_y_local =
        track_widget_get_local_y (track->widget_, self, (int) y);
      if (lane)
        {
          y -= track_y_local - lane->y_;
          height = (int) lane->height_;
        }
      else
        {
          y -= track_y_local;
        }
    }
  /* else if no track, get y/height under last visible track */
  else
    {
      /* get y below the track */
      int y_after_last_track = 0;
      for (auto &t : TRACKLIST->tracks_)
        {
          if (t->should_be_visible () && t->is_pinned () == self->is_pinned)
            {
              y_after_last_track += (int) t->get_full_visible_height ();
            }
        }
      y = y_after_last_track;
    }

  GdkRectangle highlight_rect = { x, y, width_px, height };
  arranger_widget_set_highlight_rect (self, &highlight_rect);
}

static gboolean
on_dnd_drop (
  GtkDropTarget * drop_target,
  const GValue *  value,
  double          x,
  double          y,
  gpointer        data)
{
  ArrangerWidget * self = Z_ARRANGER_WIDGET (data);

  auto settings = arranger_widget_get_editor_setting_values (self);
  x += (double) settings->scroll_start_x_;
  y += (double) settings->scroll_start_y_;

  Track *     track = timeline_arranger_widget_get_track_at_y (self, y);
  TrackLane * lane = timeline_arranger_widget_get_track_lane_at_y (self, y);
  AutomationTrack * at = timeline_arranger_widget_get_at_at_y (self, y);

  GdkModifierType state = gtk_event_controller_get_current_event_state (
    GTK_EVENT_CONTROLLER (drop_target));

  highlight_timeline (self, state, (int) x, (int) y, track, lane);

  z_debug (
    "dnd data dropped (timeline - is highlighted %d)", self->is_highlighted);

  FileDescriptor *  file = NULL;
  ChordDescriptor * chord_descr = NULL;
  if (G_VALUE_HOLDS (value, WRAPPED_OBJECT_WITH_CHANGE_SIGNAL_TYPE))
    {
      WrappedObjectWithChangeSignal * wrapped_obj =
        Z_WRAPPED_OBJECT_WITH_CHANGE_SIGNAL (g_value_get_object (value));
      if (
        wrapped_obj->type
        == WrappedObjectType::WRAPPED_OBJECT_TYPE_SUPPORTED_FILE)
        {
          file = std::get<FileDescriptor *> (wrapped_obj->obj);
        }
      if (
        wrapped_obj->type == WrappedObjectType::WRAPPED_OBJECT_TYPE_CHORD_DESCR)
        {
          chord_descr = std::get<ChordDescriptor *> (wrapped_obj->obj);
        }
    }

  if (chord_descr && self->is_highlighted && track && track->has_piano_roll ())
    {
      auto piano_roll_track = dynamic_cast<PianoRollTrack *> (track);
      ChordDescriptor * descr = chord_descr;

      /* create chord region */
      Position pos = ui_px_to_pos_timeline (self->highlight_rect.x, true);
      Position end_pos = ui_px_to_pos_timeline (
        self->highlight_rect.x + self->highlight_rect.width, true);
      int lane_pos =
        lane
          ? lane->pos_
          : (piano_roll_track->lanes_.size () == 1
               ? 0
               : piano_roll_track->lanes_.size () - 2);
      int idx_in_lane = piano_roll_track->lanes_[lane_pos]->regions_.size ();
      std::shared_ptr<MidiRegion> region;
      try
        {
          region = piano_roll_track->Track::add_region (
            std::make_shared<MidiRegion> (
              pos, *descr, track->get_name_hash (), lane_pos, idx_in_lane),
            nullptr, lane_pos, true, true);
        }
      catch (const ZrythmException &e)
        {
          e.handle (_ ("Failed to add MIDI reigon to track"));
          return false;
        }
      region->select (true, false, false);

      try
        {
          UNDO_MANAGER->perform (
            std::make_unique<CreateArrangerSelectionsAction> (*TL_SELECTIONS));
        }
      catch (const ZrythmException &e)
        {
          e.handle (_ ("Failed to create selections"));
          return false;
        }
    }
  else if (
    G_VALUE_HOLDS (value, GDK_TYPE_FILE_LIST)
    || G_VALUE_HOLDS (value, G_TYPE_FILE) || file)
    {
      if (at)
        {
          /* nothing to do */
          goto finish_data_received;
        }

      StringArray uris;
      if (G_VALUE_HOLDS (value, G_TYPE_FILE))
        {
          GFile * gfile = G_FILE (g_value_get_object (value));
          char *  uri = g_file_get_uri (gfile);
          uris.add (uri);
          g_free (uri);
        }
      else if (G_VALUE_HOLDS (value, GDK_TYPE_FILE_LIST))
        {
          GSList * l;
          for (l = (GSList *) g_value_get_boxed (value); l; l = l->next)
            {
              char * uri = g_file_get_uri (G_FILE (l->data));
              uris.add (uri);
              g_free (uri);
            }
        }

      Position pos = ui_px_to_pos_timeline (self->highlight_rect.x, true);

      try
        {
          TRACKLIST->import_files (&uris, file, track, lane, -1, &pos, nullptr);
        }
      catch (const ZrythmException &e)
        {
          e.handle (_ ("Failed to import files"));
          return false;
        }
    }

finish_data_received:
  arranger_widget_set_highlight_rect (self, nullptr);

  return true;
}

static GdkDragAction
on_dnd_motion (
  GtkDropTarget * drop_target,
  gdouble         x,
  gdouble         y,
  gpointer        user_data)
{
  ArrangerWidget * self = Z_ARRANGER_WIDGET (user_data);

  auto settings = arranger_widget_get_editor_setting_values (self);
  x += (double) settings->scroll_start_x_;
  y += (double) settings->scroll_start_y_;

  self->hovered_at = timeline_arranger_widget_get_at_at_y (self, y);
  self->hovered_lane = timeline_arranger_widget_get_track_lane_at_y (self, y);
  self->hovered_track = timeline_arranger_widget_get_track_at_y (self, y);

  arranger_widget_set_highlight_rect (self, nullptr);

  const GValue *    value = gtk_drop_target_get_value (drop_target);
  ChordDescriptor * chord_descr = NULL;
  FileDescriptor *  supported_file = NULL;
  if (G_VALUE_HOLDS (value, WRAPPED_OBJECT_WITH_CHANGE_SIGNAL_TYPE))
    {
      WrappedObjectWithChangeSignal * wrapped_obj =
        Z_WRAPPED_OBJECT_WITH_CHANGE_SIGNAL (g_value_get_object (value));
      if (
        wrapped_obj->type
        == WrappedObjectType::WRAPPED_OBJECT_TYPE_SUPPORTED_FILE)
        {
          supported_file = std::get<FileDescriptor *> (wrapped_obj->obj);
        }
      else if (
        wrapped_obj->type == WrappedObjectType::WRAPPED_OBJECT_TYPE_CHORD_DESCR)
        {
          chord_descr = std::get<ChordDescriptor *> (wrapped_obj->obj);
        }
    }

  bool has_files = false;
  if (
    G_VALUE_HOLDS (value, GDK_TYPE_FILE_LIST)
    || G_VALUE_HOLDS (value, G_TYPE_FILE))
    {
      has_files = true;
    }

  Track *           track = self->hovered_track;
  TrackLane *       lane = self->hovered_lane;
  AutomationTrack * at = self->hovered_at;
  if (chord_descr)
    {
      if (at || !track || !track->has_piano_roll ())
        {
          /* nothing to do */
          return (GdkDragAction) 0;
        }

      /* highlight track */
      highlight_timeline (
        self, (GdkModifierType) 0, (int) x, (int) y, track, lane);

      return GDK_ACTION_COPY;
    }
  else if (has_files || supported_file)
    {
      /* if current track exists and current track supports dnd highlight */
      if (track)
        {
          if (
            track->type_ != Track::Type::Midi
            && track->type_ != Track::Type::Instrument
            && track->type_ != Track::Type::Audio)
            {
              return (GdkDragAction) 0;
            }

          /* track is compatible, highlight */
          highlight_timeline (
            self, (GdkModifierType) 0, (int) x, (int) y, track, lane);
          z_info ("highlighting track");

          return GDK_ACTION_COPY;
        }
      /* else if no track, highlight below the last track  TODO */
      else
        {
          highlight_timeline (
            self, (GdkModifierType) 0, (int) x, (int) y, nullptr, nullptr);
          return GDK_ACTION_COPY;
        }
    }

  return (GdkDragAction) 0;
}

static void
on_dnd_leave (GtkDropTarget * drop_target, ArrangerWidget * self)
{
  z_info ("dnd leaving timeline, unhighlighting rect");

  arranger_widget_set_highlight_rect (self, nullptr);
}

/**
 * Sets up the timeline arranger as a drag dest.
 */
void
timeline_arranger_setup_drag_dest (ArrangerWidget * self)
{
  GtkDropTarget * drop_target =
    gtk_drop_target_new (G_TYPE_INVALID, GDK_ACTION_MOVE | GDK_ACTION_COPY);
  GType types[] = {
    GDK_TYPE_FILE_LIST, G_TYPE_FILE, WRAPPED_OBJECT_WITH_CHANGE_SIGNAL_TYPE
  };
  gtk_drop_target_set_gtypes (drop_target, types, G_N_ELEMENTS (types));
  gtk_drop_target_set_preload (drop_target, true);
  gtk_widget_add_controller (
    GTK_WIDGET (self), GTK_EVENT_CONTROLLER (drop_target));

  g_signal_connect (drop_target, "drop", G_CALLBACK (on_dnd_drop), self);
  g_signal_connect (drop_target, "motion", G_CALLBACK (on_dnd_motion), self);
  g_signal_connect (drop_target, "leave", G_CALLBACK (on_dnd_leave), self);
}

void
timeline_arranger_on_drag_end (ArrangerWidget * self)
{
  ArrangerSelections * sel = arranger_widget_get_selections (self);
  z_return_if_fail (sel);

  auto   prj_start_object = self->prj_start_object.lock ();
  double ticks_diff = 0;
  if (prj_start_object)
    ticks_diff =
      prj_start_object->pos_.ticks_
      - prj_start_object->get_transient ()->pos_.ticks_;

  try
    {
      switch (self->action)
        {
        case UiOverlayAction::RESIZING_UP_FADE_IN:
        case UiOverlayAction::RESIZING_UP_FADE_OUT:
          {
            UNDO_MANAGER->perform (std::make_unique<EditArrangerSelectionsAction> (
              *self->sel_at_start, TL_SELECTIONS.get (),
              ArrangerSelectionsAction::EditType::Fades, true));
          }
          break;
        case UiOverlayAction::ResizingL:
        case UiOverlayAction::StretchingL:
        case UiOverlayAction::ResizingLLoop:
        case UiOverlayAction::ResizingLFade:
        case UiOverlayAction::ResizingR:
        case UiOverlayAction::StretchingR:
        case UiOverlayAction::ResizingRLoop:
        case UiOverlayAction::ResizingRFade:
          {
            if (
              self->resizing_range
              && (self->action == UiOverlayAction::ResizingL || self->action == UiOverlayAction::ResizingR))
              break;

            auto is_r = [] (auto operation) {
              switch (operation)
                {
                case UiOverlayAction::ResizingR:
                case UiOverlayAction::StretchingR:
                case UiOverlayAction::ResizingRLoop:
                case UiOverlayAction::ResizingRFade:
                  return true;
                default:
                  return false;
                }
            };

            if (
              self->action == UiOverlayAction::ResizingLFade
              || self->action == UiOverlayAction::ResizingRFade)
              {
                auto fo =
                  std::dynamic_pointer_cast<FadeableObject> (prj_start_object);
                auto fo_trans = fo->get_transient<FadeableObject> ();
                if (is_r (self->action))
                  {
                    ticks_diff =
                      fo->fade_out_pos_.ticks_ - fo_trans->fade_out_pos_.ticks_;
                  }
                else
                  {
                    ticks_diff =
                      fo->fade_in_pos_.ticks_ - fo_trans->fade_in_pos_.ticks_;
                  }
              }
            else if (is_r (self->action))
              {
                auto lo =
                  dynamic_pointer_cast<LengthableObject> (prj_start_object);
                auto lo_trans = lo->get_transient<LengthableObject> ();
                ticks_diff = lo->end_pos_.ticks_ - lo_trans->end_pos_.ticks_;
              }

            auto resize_type = [] (auto operation) {
              switch (operation)
                {
                case UiOverlayAction::ResizingL:
                  return ArrangerSelectionsAction::ResizeType::L;
                case UiOverlayAction::StretchingL:
                  return ArrangerSelectionsAction::ResizeType::LStretch;
                case UiOverlayAction::ResizingLLoop:
                  return ArrangerSelectionsAction::ResizeType::LLoop;
                case UiOverlayAction::ResizingLFade:
                  return ArrangerSelectionsAction::ResizeType::LFade;
                case UiOverlayAction::ResizingR:
                  return ArrangerSelectionsAction::ResizeType::R;
                case UiOverlayAction::StretchingR:
                  return ArrangerSelectionsAction::ResizeType::RStretch;
                case UiOverlayAction::ResizingRLoop:
                  return ArrangerSelectionsAction::ResizeType::RLoop;
                case UiOverlayAction::ResizingRFade:
                  return ArrangerSelectionsAction::ResizeType::RFade;
                default:
                  return ArrangerSelectionsAction::ResizeType::L;
                }
            };

            if (self->action == UiOverlayAction::StretchingR)
              {
                /* stretch now */
                TRANSPORT->stretch_regions (
                  TL_SELECTIONS.get (), false, 0.0, true);
              }

            UNDO_MANAGER->perform (
              std::make_unique<ArrangerSelectionsAction::ResizeAction> (
                *self->sel_at_start, TL_SELECTIONS.get (),
                resize_type (self->action), ticks_diff));
          }
          break;
        case UiOverlayAction::STARTING_MOVING:
          /* if something was clicked with ctrl without moving*/
          if (self->ctrl_held)
            {
              if (prj_start_object && self->start_object_was_selected)
                {
                  /* deselect it */
                  prj_start_object->select (false, true, true);
                  z_debug ("deselecting object");
                }
            }
          else if (self->n_press == 2)
            {
              /* double click on object */
              /*z_info ("DOUBLE CLICK");*/
            }
          else if (self->n_press == 1)
            {
              /* single click on object */
              if (prj_start_object && prj_start_object->is_marker ())
                {
                  TRANSPORT->move_playhead (
                    &prj_start_object->pos_, true, true, true);
                }
            }
          break;
        case UiOverlayAction::MOVING:
        case UiOverlayAction::MovingCopy:
          UNDO_MANAGER->perform (
            std::make_unique<
              ArrangerSelectionsAction::MoveOrDuplicateTimelineAction> (
              *TL_SELECTIONS, self->action == UiOverlayAction::MOVING, ticks_diff,
              self->visible_track_diff, self->lane_diff, nullptr, true));
          break;
        case UiOverlayAction::MOVING_LINK:
          UNDO_MANAGER->perform (
            std::make_unique<ArrangerSelectionsAction::LinkAction> (
              *self->sel_at_start, *TL_SELECTIONS, ticks_diff,
              self->visible_track_diff, self->lane_diff, true));
          break;
        case UiOverlayAction::None:
        case UiOverlayAction::STARTING_SELECTION:
          sel->clear (false);
          break;
        /* if something was created */
        case UiOverlayAction::CREATING_MOVING:
        case UiOverlayAction::CreatingResizingR:
        case UiOverlayAction::AUTOFILLING:
          if (sel->has_any ())
            {
              UNDO_MANAGER->perform (
                std::make_unique<CreateArrangerSelectionsAction> (*sel));
            }
          break;
        case UiOverlayAction::DELETE_SELECTING:
        case UiOverlayAction::ERASING:
          arranger_widget_handle_erase_action (self);
          break;
        case UiOverlayAction::CUTTING:
          {
            /* get cut position */
            Position cut_pos = self->curr_pos;

            if (self->snap_grid->any_snap () && !self->shift_held)
              {
                /* keep offset is not applicable here and causes errors if
                 * enabled so we create a copy without it */
                SnapGrid sg = *self->snap_grid;
                sg.snap_to_grid_keep_offset_ = false;

                cut_pos.snap_simple (sg);
              }
            if (TL_SELECTIONS->can_split_at_pos (cut_pos))
              {
                UNDO_MANAGER->perform (
                  std::make_unique<ArrangerSelectionsAction::SplitAction> (
                    *TL_SELECTIONS, cut_pos));
              }
          }
          break;
        case UiOverlayAction::RENAMING:
          {
            const auto obj_type_str = prj_start_object->get_type_as_string ();
            auto       str = format_str (_ ("{} name"), obj_type_str);
            auto       nameable_obj =
              dynamic_cast<NameableObject *> (prj_start_object.get ());
            StringEntryDialogWidget * dialog = string_entry_dialog_widget_new (
              str,
              bind_member_function (*nameable_obj, &NameableObject::get_name),
              bind_member_function (
                *nameable_obj, &NameableObject::set_name_with_action));
            gtk_window_present (GTK_WINDOW (dialog));
            self->action = UiOverlayAction::None;
          }
          break;
        default:
          break;
        }
    }
  catch (const ZrythmException &e)
    {
      e.handle (_ ("Failed to perform action"));
    }

  self->resizing_range = 0;
  self->resizing_range_start = 0;
  self->visible_track_diff = 0;
  self->lane_diff = 0;
  self->visible_at_diff = 0;

  z_debug ("drag end timeline done");
}

template void
timeline_arranger_widget_create_region<MidiRegion> (
  ArrangerWidget *,
  Track *,
  TrackLaneImpl<MidiRegion> *,
  AutomationTrack *,
  const Position *);
template void
timeline_arranger_widget_create_region<AudioRegion> (
  ArrangerWidget *,
  Track *,
  TrackLaneImpl<AudioRegion> *,
  AutomationTrack *,
  const Position *);
template void
timeline_arranger_widget_create_region<ChordRegion> (
  ArrangerWidget *,
  Track *,
  std::nullptr_t,
  AutomationTrack *,
  const Position *);
template void
timeline_arranger_widget_create_region<AutomationRegion> (
  ArrangerWidget *,
  Track *,
  std::nullptr_t,
  AutomationTrack *,
  const Position *);