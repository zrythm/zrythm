// SPDX-FileCopyrightText: © 2019-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "actions/arranger_selections.h"
#include "dsp/audio_region.h"
#include "dsp/automation_region.h"
#include "dsp/chord_object.h"
#include "dsp/chord_region.h"
#include "dsp/chord_track.h"
#include "dsp/marker.h"
#include "dsp/marker_track.h"
#include "dsp/scale_object.h"
#include "dsp/tracklist.h"
#include "dsp/transport.h"
#include "gui/backend/arranger_selections.h"
#include "gui/backend/automation_selections.h"
#include "gui/backend/chord_selections.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "gui/backend/midi_selections.h"
#include "gui/backend/timeline_selections.h"
#include "project.h"
#include "utils/rt_thread_id.h"
#include "zrythm.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>

void
ArrangerSelections::init_loaded (bool project, UndoableAction * action)
{
  for (auto &obj_sharedptr : objects_)
    {
      auto obj_variant =
        convert_to_variant<ArrangerObjectPtrVariant> (obj_sharedptr.get ());
      std::visit (
        [&] (auto &&o) {
          using ObjectT = base_type<decltype (o)>;
          if (project)
            { /* throws an error otherwise */
              if constexpr (std::is_same_v<AudioRegion, ObjectT>)
                {
                  o->read_from_pool_ = true;
                  auto clip = o->get_clip ();
                  z_return_if_fail (clip);
                }
              o->update_positions (true, false, nullptr);
              obj_sharedptr = o->find_in_project ()->shared_from_this ();
            }
          else /* else if not project */
            {
              o->init_loaded ();
              o->update_positions (true, false, action);
              if constexpr (std::derived_from<ObjectT, Region>)
                {
                  if constexpr (std::is_same_v<AudioRegion, ObjectT>)
                    {
                      o->fix_positions (action ? action->frames_per_tick_ : 0);
                      o->validate (
                        project, action ? action->frames_per_tick_ : 0);
                    }
                  o->validate (project, 0);
                }
            }
        },
        obj_variant);
    }
}

void
ArrangerSelections::sort_by_positions (bool desc)
{
  std::sort (
    objects_.begin (), objects_.end (), [desc] (const auto &a, const auto &b) {
      return desc ? a->pos_.ticks_ > b->pos_.ticks_
                  : a->pos_.ticks_ < b->pos_.ticks_;
    });
}

std::unique_ptr<ArrangerSelections>
ArrangerSelections::new_from_type (Type type)
{
  switch (type)
    {
    case Type::Timeline:
      return std::make_unique<TimelineSelections> ();
    case Type::Midi:
      return std::make_unique<MidiSelections> ();
    case Type::Automation:
      return std::make_unique<AutomationSelections> ();
    case Type::Chord:
      return std::make_unique<ChordSelections> ();
    case Type::Audio:
      return std::make_unique<AudioSelections> ();
    default:
      z_return_val_if_reached (nullptr);
    }
}

bool
ArrangerSelections::is_object_allowed (const ArrangerObject &obj) const
{
  /* check if object is allowed */
  switch (type_)
    {
    case Type::Chord:
      z_return_val_if_fail (
        obj.type_ == ArrangerObject::Type::ChordObject, false);
      break;
    case Type::Timeline:
      z_return_val_if_fail (
        obj.type_ == ArrangerObject::Type::Region
          || obj.type_ == ArrangerObject::Type::ScaleObject
          || obj.type_ == ArrangerObject::Type::Marker,
        false);
      break;
    case Type::Midi:
      z_return_val_if_fail (
        obj.type_ == ArrangerObject::Type::MidiNote
          || obj.type_ == ArrangerObject::Type::Velocity,
        false);
      break;
    case Type::Automation:
      z_return_val_if_fail (
        obj.type_ == ArrangerObject::Type::AutomationPoint, false);
      break;
    default:
      z_return_val_if_reached (false);
    }
  return true;
}

void
ArrangerSelections::add_object_owned (std::unique_ptr<ArrangerObject> &&obj)
{
  if (is_object_allowed (*obj))
    objects_.emplace_back (std::move (obj));
}

void
ArrangerSelections::add_object_ref (const std::shared_ptr<ArrangerObject> &obj)
{
  if (is_object_allowed (*obj))
    objects_.emplace_back (obj);
}

void
ArrangerSelections::add_region_ticks (Position &pos) const
{
  auto arranger_sel_variant =
    convert_to_variant<ArrangerSelectionsPtrVariant> (this);
  std::visit (
    [&] (auto &&sel) {
      if constexpr (
        std::is_same_v<AudioSelections, std::decay_t<decltype (sel)>>)
        {
          auto r = AudioRegion::find (sel->region_id_);
          z_return_if_fail (r);
          pos.add_ticks (r->pos_.ticks_);
        }
      else if constexpr (
        std::is_same_v<TimelineSelections, std::decay_t<decltype (sel)>>)
        {
          // no region...
        }
      else
        {
          z_return_if_fail (objects_.size () > 0);
          auto obj_variant =
            convert_to_variant<ArrangerObjectPtrVariant> (objects_[0].get ());
          std::visit (
            [&] (auto &&o) {
              if constexpr (
                std::derived_from<std::decay_t<decltype (o)>, RegionOwnedObject>)
                {
                  auto r = o->get_region ();
                  z_return_if_fail (r);
                  pos.add_ticks (r->pos_.ticks_);
                }
            },
            obj_variant);
        }
    },
    arranger_sel_variant);
}

template <typename T>
requires std::derived_from<T, ArrangerObject> std::pair<T *, Position>
ArrangerSelections::get_first_object_and_pos (bool global) const
{
  Position pos;
  pos.set_to_bar (POSITION_MAX_BAR);
  T * ret_obj = nullptr;

  auto variant = convert_to_variant<ArrangerSelectionsPtrVariant> (this);
  std::visit (
    [&] (auto &&sel) {
      if constexpr (std::is_same_v<AudioSelections, base_type<decltype (sel)>>)
        {
          pos = sel->sel_start_;
        }
      else
        {
          for (const auto &obj : objects_)
            {
              std::visit (
                [&] (auto &&o) {
                  using CurObjT = base_type<decltype (o)>;
                  if constexpr (std::derived_from<CurObjT, T>)
                    {
                      if (o->pos_ < pos)
                        {
                          ret_obj = o;
                          pos = o->pos_;
                        }
                    }
                },
                convert_to_variant<ArrangerObjectPtrVariant> (obj.get ()));
            }
        }

      if constexpr (
        !std::is_same_v<TimelineSelections, std::decay_t<decltype (sel)>>)
        {
          if (global)
            {
              add_region_ticks (pos);
            }
        }
    },
    variant);

  return std::make_pair (ret_obj, pos);
}

template <typename RetObjT>
requires std::derived_from<RetObjT, ArrangerObject> std::pair<RetObjT *, Position>
ArrangerSelections::get_last_object_and_pos (bool global, bool ends_last) const
{
  Position pos;
  pos.set_to_bar (-POSITION_MAX_BAR);
  RetObjT * ret_obj = nullptr;

  auto variant = convert_to_variant<ArrangerSelectionsPtrVariant> (this);
  std::visit (
    [&] (auto &&sel) {
      if constexpr (std::is_same_v<AudioSelections, base_type<decltype (sel)>>)
        {
          pos = sel->sel_end_;
        }
      else
        {
          for (const auto &obj : objects_)
            {
              auto obj_variant =
                convert_to_variant<ArrangerObjectPtrVariant> (obj.get ());
              std::visit (
                [&] (auto &&o) {
                  using CurObjT = base_type<decltype (o)>;
                  if constexpr (std::derived_from<CurObjT, RetObjT>)
                    {
                      if constexpr (std::derived_from<CurObjT, LengthableObject>)
                        {
                          if (ends_last)
                            {
                              if (o->end_pos_ > pos)
                                {
                                  ret_obj = o;
                                  pos = o->end_pos_;
                                }
                            }
                          else
                            {
                              if (o->pos_ > pos)
                                {
                                  ret_obj = o;
                                  pos = o->pos_;
                                }
                            }
                        }
                      else
                        {
                          if (o->pos_ > pos)
                            {
                              ret_obj = o;
                              pos = o->pos_;
                            }
                        }
                    }
                },
                obj_variant);
            }
        }

      if constexpr (
        !std::is_same_v<TimelineSelections, std::decay_t<decltype (sel)>>)
        {
          if (global)
            {
              add_region_ticks (pos);
            }
        }
    },
    variant);

  return std::make_pair (ret_obj, pos);
}

void
ArrangerSelections::add_to_region (Region &region)
{
  auto variant = convert_to_variant<ArrangerSelectionsPtrVariant> (this);
  std::visit (
    [&] (auto &&self) {
      if constexpr (
        std::is_same_v<TimelineSelections, std::decay_t<decltype (self)>>)
        {
          // no region...
          return;
        }
      for (auto &obj : self->objects_)
        {
          auto obj_variant =
            convert_to_variant<ArrangerObjectPtrVariant> (obj.get ());
          std::visit (
            [&] (auto &&o) {
              if constexpr (
                std::derived_from<std::decay_t<decltype (o)>, RegionOwnedObject>)
                {
                  auto region_variant =
                    convert_to_variant<RegionPtrVariant> (&region);
                  std::visit (
                    [&] (auto &&r) {
                      auto obj_clone = o->clone_shared ();
                      r->append_object (obj_clone, false);
                    },
                    region_variant);
                }
            },
            obj_variant);
        }
    },
    variant);
}

void
ArrangerSelections::add_ticks (const double ticks)
{
  for (auto &obj : objects_)
    {
      obj->move (ticks);
    }
}

void
ArrangerSelections::select_all (bool fire_events)
{
  clear (false);

  auto r = CLIP_EDITOR->get_region ();

  auto variant = convert_to_variant<ArrangerSelectionsPtrVariant> (this);
  std::visit (
    [&] (auto &&self) {
      if constexpr (
        std::is_same_v<TimelineSelections, std::decay_t<decltype (self)>>)
        {
          /* midi/audio regions */
          for (auto &track : TRACKLIST->tracks_)
            {
              if (auto laned_track = dynamic_cast<LanedTrack *> (track.get ()))
                {
                  auto laned_track_variant =
                    convert_to_variant<LanedTrackPtrVariant> (laned_track);
                  std::visit (
                    [&] (auto &&lt) {
                      for (auto &region : lt->regions_)
                        {
                          add_object_ref (region);
                        }
                    },
                    laned_track_variant);
                }

              /* automation regions */
              if (
                auto automatable_track =
                  dynamic_cast<AutomatableTrack *> (track.get ()))
                {
                  const auto &atl =
                    automatable_track->get_automation_tracklist ();
                  if (automatable_track->automation_visible_)
                    {
                      for (auto &at : atl.ats_)
                        {
                          if (!at->visible_)
                            continue;

                          for (auto &region : at->regions_)
                            {
                              add_object_ref (region);
                            }
                        }
                    }
                }
            }

          /* chord regions */
          for (auto &region : P_CHORD_TRACK->regions_)
            {
              add_object_ref (region);
            }

          /* scales */
          for (auto &scale : P_CHORD_TRACK->scales_)
            {
              add_object_ref (scale);
            }

          /* markers */
          for (auto &marker : P_MARKER_TRACK->markers_)
            {
              add_object_ref (marker);
            }
        }
      else if constexpr (
        std::is_same_v<ChordSelections, std::decay_t<decltype (self)>>)
        {
          if (!r || !r->is_chord ())
            return;

          auto cr = dynamic_cast<ChordRegion *> (r);
          for (auto &co : cr->chord_objects_)
            {
              z_return_if_fail (
                co->chord_index_ < CHORD_EDITOR->chords_.size ());
              add_object_ref (co);
            }
        }
      else if constexpr (
        std::is_same_v<MidiSelections, std::decay_t<decltype (self)>>)
        {
          if (!r || !r->is_midi ())
            return;

          auto mr = dynamic_cast<MidiRegion *> (r);
          for (auto &mn : mr->midi_notes_)
            {
              add_object_ref (mn);
            }
        }
      else if constexpr (
        std::is_same_v<AudioSelections, std::decay_t<decltype (self)>>)
        {
          /* no objects in audio arranger yet */
        }
      else if constexpr (
        std::is_same_v<AutomationSelections, std::decay_t<decltype (self)>>)
        {
          if (!r || !r->is_automation ())
            return;

          auto ar = dynamic_cast<AutomationRegion *> (r);
          for (auto &ap : ar->aps_)
            {
              add_object_ref (ap);
            }
        }

      if (fire_events)
        {
          EVENTS_PUSH (EventType::ET_ARRANGER_SELECTIONS_CHANGED, self);
        }
    },
    variant);
}

void
ArrangerSelections::clear (bool fire_events)
{
  if (!has_any ())
    {
      return;
    }

  auto variant = convert_to_variant<ArrangerSelectionsPtrVariant> (this);
  std::visit (
    [&] (auto &&self) {
      if constexpr (
        std::is_same_v<AudioSelections, std::decay_t<decltype (self)>>)
        {
          self->has_selection_ = false;
        }
      else
        {
          objects_.clear ();
        }
    },
    variant);

  // TODO: fire events
}

void
ArrangerSelections::post_deserialize ()
{
  for (auto &obj : objects_)
    {
      obj->post_deserialize ();
    }
}

bool
ArrangerSelections::validate () const
{
  auto variant = convert_to_variant<ArrangerSelectionsPtrVariant> (this);
  return std::visit (
    [] (auto &&self) {
      if constexpr (
        std::is_same_v<AudioSelections, std::decay_t<decltype (self)>>)
        {
          auto r = RegionImpl<AudioRegion>::find (self->region_id_);
          z_return_val_if_fail (r, false);
          if (self->sel_start_ < r->pos_ || self->sel_end_ >= r->end_pos_)
            return false;
        }
      return true;
    },
    variant);
}

bool
ArrangerSelections::contains_object (const ArrangerObject &obj) const
{
  return std::visit (
    [&] (auto &&obj_ptr) { return find_object (*obj_ptr) != nullptr; },
    convert_to_variant<ArrangerObjectPtrVariant> (&obj));
}

bool
ArrangerSelections::contains_undeletable_object () const
{
  return std::any_of (objects_.begin (), objects_.end (), [] (auto &element) {
    return !element->is_deletable ();
  });
}

bool
ArrangerSelections::contains_unclonable_object () const
{
  return contains_undeletable_object ();
}

bool
ArrangerSelections::contains_unrenamable_object () const
{
  return contains_undeletable_object ();
}

bool
ArrangerSelections::contains_object_with_property (Property property, bool value)
  const
{

#define CHECK_PROP(x) \
  (property == ArrangerSelectionsProperty::ARRANGER_SELECTIONS_PROPERTY_##x)

  for (auto &obj : objects_)
    {
      if (property == Property::HasLength && obj->has_length () == value)
        {
          return true;
        }
      if (property == Property::HasLooped)
        {
          if (!obj->can_loop ())
            {
              if (!value)
                return true;
            }
          else
            {
              auto loopable_object = dynamic_cast<LoopableObject *> (obj.get ());
              if ((loopable_object->get_num_loops (false) > 0) == value)
                return true;
            }
        }
      if (property == Property::CanLoop)
        {
          if (obj->can_loop () == value)
            return true;
        }
      if (property == Property::CanFade)
        {
          if (obj->can_fade () == value)
            return true;
        }
    }
  return false;
}

void
ArrangerSelections::remove_object (const ArrangerObject &obj)
{
  auto it =
    std::find_if (objects_.begin (), objects_.end (), [&obj] (auto &element) {
      return &obj == element.get ();
    });
  if (it != objects_.end ())
    {
      objects_.erase (it);
    }
}

double
ArrangerSelections::get_length_in_ticks ()
{
  auto p1 = get_first_object_and_pos (true);
  auto p2 = get_last_object_and_pos (true, true);

  return p2.second.ticks_ - p1.second.ticks_;
}

bool
ArrangerSelections::can_be_pasted () const
{
  return can_be_pasted_at (PLAYHEAD);
}

bool
ArrangerSelections::can_be_pasted_at (const Position pos, const int idx) const
{
  if (contains_unclonable_object ())
    {
      z_debug ("selections contain an unclonable object, cannot paste");
      return false;
    }
  return can_be_pasted_at_impl (pos, idx);
}

void
ArrangerSelections::paste_to_pos (const Position &pos, bool undoable)
{
  if (!has_any ())
    {
      z_debug ("nothing to paste");
      return;
    }

  auto variant = convert_to_variant<ArrangerSelectionsPtrVariant> (this);
  std::visit (
    [&] (auto &&self) {
      /* note: the objects in these selections will become the actual project
       * objects so they should not be free'd here */
      auto clone_sel = self->clone_unique ();

      /* clear current project selections */
      auto project_sel = ArrangerSelections::get_for_type (self->type_);
      z_return_if_fail (project_sel);
      project_sel->clear (false);

      auto first_obj_pair = clone_sel->get_first_object_and_pos (false);

      if constexpr (
        std::is_same_v<TimelineSelections, std::decay_t<decltype (self)>>)
        {
          auto track = TRACKLIST_SELECTIONS->get_highest_track ();
          z_return_if_fail (track);

          clone_sel->add_ticks (pos.ticks_ - first_obj_pair.second.ticks_);

          /* add selections to track */
          for (auto obj : clone_sel->objects_)
            {
              auto obj_variant =
                convert_to_variant<ArrangerObjectPtrVariant> (obj);
              std::visit (
                [&] (auto &&o) {
                  if constexpr (
                    std::derived_from<std::decay_t<decltype (o)>, Region>)
                    {
                      /* automation not allowed to be pasted this way */
                      if constexpr (
                        !std::is_same_v<decltype (o), AutomationRegion>)
                        {
                          try
                            {
                              track->add_region (
                                o->shared_from_this (), nullptr,
                                o->id_.lane_pos_, false, false);
                            }
                          catch (const ZrythmException &e)
                            {
                              e.handle (_ ("Failed to add region"));
                            }
                        }
                    }

                  if constexpr (
                    std::is_same_v<std::decay_t<decltype (o)>, ScaleObject>)
                    {
                      P_CHORD_TRACK->add_scale (o->shared_from_this ());
                    }
                  else if constexpr (
                    std::is_same_v<std::decay_t<decltype (o)>, Marker>)
                    {
                      P_MARKER_TRACK->add_marker (o->shared_from_this ());
                    }
                },
                obj_variant);
            }
        }
      /* else if selections inside region (ie not timeline selections) */
      else
        {
          auto region = CLIP_EDITOR->get_region ();

          /* add selections to region */
          clone_sel->add_to_region (*region);
          clone_sel->add_ticks (
            (pos.ticks_ - region->pos_.ticks_) - first_obj_pair.second.ticks_);
        }

      if (undoable)
        {
          try
            {
              UNDO_MANAGER->perform (
                std::make_unique<CreateArrangerSelectionsAction> (*clone_sel));
            }
          catch (const ZrythmException &e)
            {
              e.handle (_ ("Failed to paste selections"));
            }
        }
    },
    variant);
}

ArrangerSelections *
ArrangerSelections::get_for_type (ArrangerSelections::Type type)
{
  switch (type)
    {
    case ArrangerSelections::Type::Timeline:
      return TL_SELECTIONS.get ();
    case ArrangerSelections::Type::Midi:
      return MIDI_SELECTIONS.get ();
    case ArrangerSelections::Type::Chord:
      return CHORD_SELECTIONS.get ();
    case ArrangerSelections::Type::Automation:
      return AUTOMATION_SELECTIONS.get ();
    default:
      z_return_val_if_reached (nullptr);
    }
  z_return_val_if_reached (nullptr);
}

bool
ArrangerSelections::can_split_at_pos (const Position pos) const
{
  auto variant = convert_to_variant<ArrangerSelectionsPtrVariant> (this);
  bool can_split = std::visit (
    [&] (auto &&self) {
      if constexpr (
        std::is_same_v<TimelineSelections, std::decay_t<decltype (self)>>)
        {
          for (auto &obj : objects_)
            {
              auto obj_variant =
                convert_to_variant<ArrangerObjectPtrVariant> (obj.get ());
              auto ret = std::visit (
                [&] (auto &&o) {
                  if constexpr (
                    std::derived_from<std::decay_t<decltype (o)>, Region>)
                    {
                      /* don't allow splitting at edges */
                      if (pos <= o->pos_ || pos >= o->end_pos_)
                        {
                          return false;
                        }
                    }
                  else
                    {
                      /* only regions can be split*/
                      return false;
                    }
                },
                obj_variant);
              if (!ret)
                return false;
            }

          return true;
        }
      else if constexpr (
        std::is_same_v<MidiSelections, std::decay_t<decltype (self)>>)
        {
          for (auto obj : objects_ | type_is<MidiNote> ())
            {
              /* don't allow splitting at edges */
              if (pos <= obj->pos_ || pos >= obj->end_pos_)
                {
                  return false;
                }
            }
          return true;
        }
      else
        {
          return false;
        }
    },
    variant);

  if (!can_split)
    {
      z_debug ("cannot split {} selections at {}", ENUM_NAME (type_), pos);
    }

  return can_split;
}

template std::pair<MidiNote *, Position>
ArrangerSelections::get_first_object_and_pos (bool global) const;

template std::pair<MidiNote *, Position>
ArrangerSelections::get_last_object_and_pos (bool global, bool ends_last) const;
