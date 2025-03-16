// SPDX-FileCopyrightText: © 2019-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/backend/backend/project.h"
#include "gui/backend/backend/zrythm.h"
#include "gui/backend/ui.h"
#include "gui/dsp/automatable_track.h"
#include "gui/dsp/automation_point.h"
#include "gui/dsp/automation_region.h"

#include "dsp/position.h"

AutomationRegion::AutomationRegion (QObject * parent)
    : ArrangerObject (Type::Region), BoundedObject (), QAbstractListModel (parent)
{
  ArrangerObject::parent_base_qproperties (*this);
  BoundedObject::parent_base_qproperties (*this);
}

AutomationRegion::AutomationRegion (
  const Position                &start_pos,
  const Position                &end_pos,
  dsp::PortIdentifier::TrackUuid track_id,
  int                            at_idx,
  int                            idx_inside_at,
  QObject *                      parent)
    : AutomationRegion (parent)
{
  id_ = RegionIdentifier (RegionType::Automation);

  init (
    start_pos, end_pos, track_id, at_idx, idx_inside_at,
    AUDIO_ENGINE->ticks_per_frame_);
}

void
AutomationRegion::init_loaded ()
{
  ArrangerObject::init_loaded_base ();
  TimelineObject::init_loaded_base ();
  NamedObject::init_loaded_base ();
  for (auto &ap : aps_)
    {
      ap->init_loaded ();
    }
}

// ========================================================================
// QML Interface
// ========================================================================

QHash<int, QByteArray>
AutomationRegion::roleNames () const
{
  QHash<int, QByteArray> roles;
  roles[AutomationPointPtrRole] = "automationPoint";
  return roles;
}
int
AutomationRegion::rowCount (const QModelIndex &parent) const
{
  return aps_.size ();
}

QVariant
AutomationRegion::data (const QModelIndex &index, int role) const
{
  if (index.row () < 0 || index.row () >= static_cast<int> (aps_.size ()))
    return {};

  if (role == AutomationPointPtrRole)
    {
      return QVariant::fromValue (aps_.at (index.row ()));
    }
  return {};
}

// ========================================================================

void
AutomationRegion::print_automation () const
{
  for (const auto &ap : aps_)
    {
      z_debug ("[{}] {} : {}", ap->index_, ap->fvalue_, *ap->pos_);
    }
}

AutomationTrack *
AutomationRegion::get_automation_track () const
{
  auto track_var = get_track ();
  return std::visit (
    [&] (auto &&track) -> AutomationTrack * {
      using TrackT = base_type<decltype (track)>;
      if constexpr (std::derived_from<TrackT, AutomatableTrack>)
        {
          const auto &atl = track->get_automation_tracklist ();
          z_return_val_if_fail ((int) atl.ats_.size () > id_.at_idx_, nullptr);

          return atl.ats_.at (id_.at_idx_);
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
      CLIP_EDITOR->unset_region ();
    }
  bool was_selected = false;
  if (is_selected ())
    {
      was_selected = true;
      setSelected (false);
    }
  id_.at_idx_ = at.index_;
  auto track = at.get_track ();
  id_.track_uuid_ = track->get_uuid ();

  update_identifier ();

  /* reselect it if was selected */
  if (was_selected)
    {
      setSelected (true);
    }
}

bool
AutomationRegion::get_muted (bool check_parent) const
{
  if (check_parent)
    {
      auto at = get_automation_track ();
      z_return_val_if_fail (at, true);

      if (at->automation_mode_ == AutomationMode::Off)
        return true;
    }
  return muted_;
}

void
AutomationRegion::force_sort ()
{
  std::sort (aps_.begin (), aps_.end (), [] (const auto &a, const auto &b) {
    return *a < *b;
  });

  /* refresh indices */
  for (size_t i = 0; i < aps_.size (); i++)
    {
      aps_.at (i)->set_region_and_index (*this, i);
    }
}

AutomationPoint *
AutomationRegion::get_prev_ap (const AutomationPoint &ap) const
{
  if (ap.index_ > 0)
    return aps_[ap.index_ - 1];

  return nullptr;
}

AutomationPoint *
AutomationRegion::get_next_ap (
  const AutomationPoint &ap,
  bool                   check_positions,
  bool                   check_transients) const
{
  if (check_positions)
    {
      check_transients =
        ZRYTHM_HAVE_UI /* && MW_AUTOMATION_ARRANGER
        && MW_AUTOMATION_ARRANGER->action == UiOverlayAction::MovingCopy */
        ;
      AutomationPoint * next_ap = nullptr;
      const int         loop_times = check_transients ? 2 : 1;
      for (auto &cur_ap_outer : aps_)
        {
          for (int j = 0; j < loop_times; j++)
            {
              AutomationPoint * cur_ap = cur_ap_outer;
              if (j == 1)
                {
                  if (cur_ap->transient_)
                    {
                      cur_ap = dynamic_cast<AutomationPoint *> (
                        cur_ap->get_transient ());
                    }
                  else
                    continue;
                }

              if (cur_ap == ap)
                continue;

              if (
                cur_ap->pos_ >= ap.pos_
                && ((next_ap == nullptr) || cur_ap->pos_ < next_ap->pos_))
                {
                  next_ap = cur_ap;
                }
            }
        }
      return next_ap;
    }
  else if (ap.index_ < static_cast<int> (aps_.size ()) - 1)
    return aps_[ap.index_ + 1];

  return nullptr;
}

void
AutomationRegion::get_aps_since_last_recorded (
  Position                        pos,
  std::vector<AutomationPoint *> &aps)
{
  aps.clear ();

  if ((last_recorded_ap_ == nullptr) || pos <= *last_recorded_ap_->pos_)
    return;

  for (auto &ap : aps_)
    {
      if (ap->pos_ > last_recorded_ap_->pos_ && *ap->pos_ <= pos)
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
  if (ap && pos.ticks_ - ap->pos_->ticks_ <= (double) delta_ticks)
    {
      return ap;
    }
  else if (!before_only)
    {
      pos.add_ticks (delta_ticks, AUDIO_ENGINE->frames_per_tick_);
      ap = at->get_ap_before_pos (pos, true, use_snapshots);
      if (ap)
        {
          double diff = ap->pos_->ticks_ - _pos->ticks_;
          if (diff >= 0.0)
            return ap;
        }
    }

  return nullptr;
}

void
AutomationRegion::init_after_cloning (
  const AutomationRegion &other,
  ObjectCloneType         clone_type)
{
  init (
    *other.pos_, *other.end_pos_, other.id_.track_uuid_, other.id_.at_idx_,
    other.id_.idx_, AUDIO_ENGINE->ticks_per_frame_);
  aps_.reserve (other.aps_.size ());
  for (const auto &ap : other.aps_)
    {
      auto * clone = ap->clone_raw_ptr ();
      clone->setParent (this);
      aps_.push_back (clone);
    }
  RegionImpl::copy_members_from (other, clone_type);
  TimelineObject::copy_members_from (other, clone_type);
  NamedObject::copy_members_from (other, clone_type);
  LoopableObject::copy_members_from (other, clone_type);
  MuteableObject::copy_members_from (other, clone_type);
  BoundedObject::copy_members_from (other, clone_type);
  ColoredObject::copy_members_from (other, clone_type);
  ArrangerObject::copy_members_from (other, clone_type);
  force_sort ();
}

bool
AutomationRegion::validate (bool is_project, double frames_per_tick) const
{
  for (size_t i = 0; i < aps_.size (); i++)
    {
      auto * ap = aps_[i];
      z_return_val_if_fail (ap->index_ == (int) i, false);
    }

  if (
    !Region::are_members_valid (is_project)
    || !TimelineObject::are_members_valid (is_project)
    || !NamedObject::are_members_valid (is_project)
    || !LoopableObject::are_members_valid (is_project)
    || !MuteableObject::are_members_valid (is_project)
    || !BoundedObject::are_members_valid (is_project)
    || !ColoredObject::are_members_valid (is_project)
    || !ArrangerObject::are_members_valid (is_project))
    {
      return false;
    }

  return true;
}
