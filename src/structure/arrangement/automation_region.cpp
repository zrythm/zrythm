// SPDX-FileCopyrightText: © 2019-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/position.h"
#include "gui/backend/backend/project.h"
#include "gui/backend/backend/zrythm.h"
#include "gui/backend/ui.h"
#include "structure/arrangement/automation_point.h"
#include "structure/arrangement/automation_region.h"
#include "structure/tracks/automatable_track.h"
#include "utils/views.h"

namespace zrythm::structure::arrangement
{
AutomationRegion::AutomationRegion (
  ArrangerObjectRegistry &obj_registry,
  TrackResolver           track_resolver,
  QObject *               parent)
    : ArrangerObject (Type::AutomationRegion, track_resolver),
      Region (obj_registry), QObject (parent)
{
  ArrangerObject::set_parent_on_base_qproperties (*this);
  BoundedObject::parent_base_qproperties (*this);
  init_colored_object ();
}

void
AutomationRegion::print_automation () const
{
  for (const auto &[index, ap] : utils::views::enumerate (get_children_view ()))
    {
      z_debug ("[{}] {} : {}", index, ap->fvalue_, ap->get_position ());
    }
}

tracks::AutomationTrack *
AutomationRegion::get_automation_track () const
{
  auto track_var = get_track ();
  return std::visit (
    [&] (auto &&track) -> AutomationTrack * {
      using TrackT = base_type<decltype (track)>;
      if constexpr (std::derived_from<TrackT, tracks::AutomatableTrack>)
        {
          const auto &atl = track->get_automation_tracklist ();
          return atl.get_automation_track_by_port_id (automatable_port_id_);
        }
      else
        {
          z_return_val_if_reached (nullptr);
        }
    },
    track_var);
}

void
AutomationRegion::set_automation_track (AutomationTrack &at)
{
  z_debug (
    "setting region automation track to {} {}", at.index_, at.getLabel ());

  /* this must not be called on non-project regions (or during project
   * destruction) */
  z_return_if_fail (PROJECT);

  /* if clip editor region or region selected, unselect it */
  if (get_uuid () == *CLIP_EDITOR->get_region_id ())
    {
      CLIP_EDITOR->unsetRegion ();
    }
  automatable_port_id_ = at.port_id_;
  auto track_var = at.get_track ();
  track_id_ = tracks::TrackSpan::uuid_projection (track_var);

  update_identifier ();
}

bool
AutomationRegion::get_muted (bool check_parent) const
{
  if (check_parent)
    {
      auto at = get_automation_track ();
      z_return_val_if_fail (at, true);

      if (at->automation_mode_ == AutomationTrack::AutomationMode::Off)
        return true;
    }
  return muted_;
}

void
AutomationRegion::force_sort ()
{
  std::ranges::sort (
    get_children_vector (), [&] (const auto &a_id, const auto &b_id) {
      const auto a = std::get<AutomationPoint *> (a_id.get_object ());
      const auto b = std::get<AutomationPoint *> (b_id.get_object ());
      return *a < *b;
    });

  /* refresh indices */
  for (const auto &ap : get_children_view ())
    {
      ap->set_region_and_index (*this);
    }
}

AutomationPoint *
AutomationRegion::get_prev_ap (const AutomationPoint &ap) const
{
  auto it = std::ranges::find (
    get_children_vector (), ap.get_uuid (), &ArrangerObjectUuidReference::id);

  // if found and not the first element
  if (
    it != get_children_vector ().end () && it != get_children_vector ().begin ())
    {
      return std::get<AutomationPoint *> (
        (*std::ranges::prev (it)).get_object ());
    }

  return nullptr;
}

AutomationPoint *
AutomationRegion::get_next_ap (const AutomationPoint &ap, bool check_positions)
  const
{
  if (check_positions)
    {
      AutomationPoint * next_ap = nullptr;
      for (auto * cur_ap_outer : get_children_view ())
        {
          AutomationPoint * cur_ap = cur_ap_outer;

          if (cur_ap->get_uuid () == ap.get_uuid ())
            continue;

          if (
            cur_ap->get_position () >= ap.get_position ()
            && ((next_ap == nullptr) || cur_ap->get_position () < next_ap->get_position ()))
            {
              next_ap = cur_ap;
            }
        }
      return next_ap;
    }

  auto it = std::ranges::find (
    get_children_vector (), ap.get_uuid (), &ArrangerObjectUuidReference::id);

  // if found and not the last element
  if (
    it != get_children_vector ().end ()
    && std::ranges::next (it) != get_children_vector ().end ())
    {

      return std::get<AutomationPoint *> (
        (*std::ranges::next (it)).get_object ());
    }

  return nullptr;
}

void
AutomationRegion::get_aps_since_last_recorded (
  Position                        pos,
  std::vector<AutomationPoint *> &aps)
{
  aps.clear ();

  if (
    (last_recorded_ap_ == nullptr) || pos <= last_recorded_ap_->get_position ())
    return;

  for (auto * ap : get_children_view ())
    {
      if (
        ap->get_position () > last_recorded_ap_->get_position ()
        && ap->get_position () <= pos)
        {
          aps.push_back (ap);
        }
    }
}

AutomationPoint *
AutomationRegion::get_ap_around (
  Position * _pos,
  double     delta_ticks,
  bool       before_only,
  bool       use_snapshots)
{
  Position pos;
  pos = *_pos;
  AutomationTrack * at = get_automation_track ();
  AutomationPoint * ap = at->get_ap_before_pos (pos, true, use_snapshots);
  if ((ap != nullptr) && pos.ticks_ - ap->get_position ().ticks_ <= delta_ticks)
    {
      return ap;
    }
  else if (!before_only)
    {
      pos.add_ticks (delta_ticks, AUDIO_ENGINE->frames_per_tick_);
      ap = at->get_ap_before_pos (pos, true, use_snapshots);
      if (ap != nullptr)
        {
          double diff = ap->get_position ().ticks_ - _pos->ticks_;
          if (diff >= 0.0)
            return ap;
        }
    }

  return nullptr;
}

void
init_from (
  AutomationRegion       &obj,
  const AutomationRegion &other,
  utils::ObjectCloneType  clone_type)
{

  init_from (
    static_cast<AutomationRegion::RegionImpl &> (obj),
    static_cast<const AutomationRegion::RegionImpl &> (other), clone_type);
  init_from (
    static_cast<TimelineObject &> (obj),
    static_cast<const TimelineObject &> (other), clone_type);
  init_from (
    static_cast<NamedObject &> (obj), static_cast<const NamedObject &> (other),
    clone_type);
  init_from (
    static_cast<LoopableObject &> (obj),
    static_cast<const LoopableObject &> (other), clone_type);
  init_from (
    static_cast<MuteableObject &> (obj),
    static_cast<const MuteableObject &> (other), clone_type);
  init_from (
    static_cast<BoundedObject &> (obj),
    static_cast<const BoundedObject &> (other), clone_type);
  init_from (
    static_cast<ColoredObject &> (obj),
    static_cast<const ColoredObject &> (other), clone_type);
  init_from (
    static_cast<ArrangerObject &> (obj),
    static_cast<const ArrangerObject &> (other), clone_type);
  init_from (
    static_cast<AutomationRegion::ArrangerObjectOwner &> (obj),
    static_cast<const AutomationRegion::ArrangerObjectOwner &> (other),
    clone_type);
  obj.force_sort ();
}

bool
AutomationRegion::validate (bool is_project, dsp::FramesPerTick frames_per_tick)
  const
{
#if 0
  for (const auto &[index, ap] : utils::views::enumerate (get_children_view ()))
    {
      z_return_val_if_fail (ap->index_ == index, false);
    }
#endif

  if (
    !Region::are_members_valid (is_project, frames_per_tick)
    || !TimelineObject::are_members_valid (is_project, frames_per_tick)
    || !NamedObject::are_members_valid (is_project, frames_per_tick)
    || !LoopableObject::are_members_valid (is_project, frames_per_tick)
    || !MuteableObject::are_members_valid (is_project, frames_per_tick)
    || !BoundedObject::are_members_valid (is_project, frames_per_tick)
    || !ColoredObject::are_members_valid (is_project, frames_per_tick)
    || !ArrangerObject::are_members_valid (is_project, frames_per_tick))
    {
      return false;
    }

  return true;
}
}
