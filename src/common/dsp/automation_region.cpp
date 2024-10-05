// SPDX-FileCopyrightText: © 2019-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <cstdlib>

#include "gui/cpp/backend/project.h"
#include "gui/cpp/backend/zrythm.h"
#include "gui/cpp/gtk_widgets/arranger.h"
#include "gui/cpp/gtk_widgets/automation_arranger.h"
#include "gui/cpp/gtk_widgets/automation_editor_space.h"
#include "gui/cpp/gtk_widgets/bot_dock_edge.h"
#include "gui/cpp/gtk_widgets/center_dock.h"
#include "gui/cpp/gtk_widgets/clip_editor.h"
#include "gui/cpp/gtk_widgets/clip_editor_inner.h"
#include "gui/cpp/gtk_widgets/main_window.h"
#include "gui/cpp/gtk_widgets/zrythm_app.h"

#include "common/dsp/automatable_track.h"
#include "common/dsp/automation_point.h"
#include "common/dsp/automation_region.h"
#include "common/dsp/position.h"
#include "common/utils/ui.h"

AutomationRegion::AutomationRegion (
  const Position &start_pos,
  const Position &end_pos,
  unsigned int    track_name_hash,
  int             at_idx,
  int             idx_inside_at)
{
  id_ = RegionIdentifier (RegionType::Automation);

  init (start_pos, end_pos, track_name_hash, at_idx, idx_inside_at);
}

void
AutomationRegion::init_loaded ()
{
  ArrangerObject::init_loaded_base ();
  TimelineObject::init_loaded_base ();
  NameableObject::init_loaded_base ();
  for (auto &ap : aps_)
    {
      ap->init_loaded ();
    }
}

void
AutomationRegion::print_automation () const
{
  for (const auto &ap : aps_)
    {
      z_debug ("[{}] {} : {}", ap->index_, ap->fvalue_, ap->pos_.to_string ());
    }
}

AutomationTrack *
AutomationRegion::get_automation_track () const
{
  auto track = dynamic_cast<AutomatableTrack *> (get_track ());
  z_return_val_if_fail (track != nullptr, nullptr);
  const auto &atl = track->get_automation_tracklist ();
  z_return_val_if_fail ((int) atl.ats_.size () > id_.at_idx_, nullptr);

  return atl.ats_[id_.at_idx_].get ();
}

void
AutomationRegion::set_automation_track (AutomationTrack &at)
{
  z_debug (
    "setting region automation track to {} {}", at.index_,
    at.port_id_.get_label ());

  /* this must not be called on non-project regions (or during project
   * destruction) */
  z_return_if_fail (PROJECT);

  /* if clip editor region or region selected, unselect it */
  if (id_ == CLIP_EDITOR->region_id_)
    {
      CLIP_EDITOR->set_region (nullptr, true);
    }
  bool was_selected = false;
  if (is_selected ())
    {
      was_selected = true;
      select (false, false, false);
    }
  id_.at_idx_ = at.index_;
  auto track = at.get_track ();
  id_.track_name_hash_ = track->get_name_hash ();

  update_identifier ();

  /* reselect it if was selected */
  if (was_selected)
    {
      select (true, true, false);
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
    return *a.get () < *b.get ();
  });

  /* refresh indices */
  for (size_t i = 0; i < aps_.size (); i++)
    {
      aps_[i]->set_region_and_index (*this, i);
    }
}

AutomationPoint *
AutomationRegion::get_prev_ap (const AutomationPoint &ap) const
{
  if (ap.index_ > 0)
    return aps_[ap.index_ - 1].get ();

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
        ZRYTHM_HAVE_UI && MW_AUTOMATION_ARRANGER
        && MW_AUTOMATION_ARRANGER->action == UiOverlayAction::MovingCopy;
      AutomationPoint * next_ap = nullptr;
      const int         loop_times = check_transients ? 2 : 1;
      for (auto &cur_ap_outer : aps_)
        {
          for (int j = 0; j < loop_times; j++)
            {
              AutomationPoint &cur_ap = *cur_ap_outer;
              if (j == 1)
                {
                  if (cur_ap.transient_)
                    {
                      cur_ap = *cur_ap.get_transient<AutomationPoint> ();
                    }
                  else
                    continue;
                }

              if (cur_ap == ap)
                continue;

              if (
                cur_ap.pos_ >= ap.pos_
                && (!next_ap || cur_ap.pos_ < next_ap->pos_))
                {
                  next_ap = &cur_ap;
                }
            }
        }
      return next_ap;
    }
  else if (ap.index_ < static_cast<int> (aps_.size ()) - 1)
    return aps_[ap.index_ + 1].get ();

  return nullptr;
}

void
AutomationRegion::get_aps_since_last_recorded (
  Position                        pos,
  std::vector<AutomationPoint *> &aps)
{
  aps.clear ();

  if (!last_recorded_ap_ || pos <= last_recorded_ap_->pos_)
    return;

  for (auto &ap : aps_)
    {
      if (ap->pos_ > last_recorded_ap_->pos_ && ap->pos_ <= pos)
        {
          aps.push_back (ap.get ());
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
  if (ap && pos.ticks_ - ap->pos_.ticks_ <= (double) delta_ticks)
    {
      return ap;
    }
  else if (!before_only)
    {
      pos.add_ticks (delta_ticks);
      ap = at->get_ap_before_pos (pos, true, use_snapshots);
      if (ap)
        {
          double diff = ap->pos_.ticks_ - _pos->ticks_;
          if (diff >= 0.0)
            return ap;
        }
    }

  return nullptr;
}

bool
AutomationRegion::validate (bool is_project, double frames_per_tick) const
{
  for (size_t i = 0; i < aps_.size (); i++)
    {
      AutomationPoint * ap = aps_[i].get ();
      z_return_val_if_fail (ap->index_ == (int) i, false);
    }

  if (
    !Region::are_members_valid (is_project)
    || !TimelineObject::are_members_valid (is_project)
    || !NameableObject::are_members_valid (is_project)
    || !LoopableObject::are_members_valid (is_project)
    || !MuteableObject::are_members_valid (is_project)
    || !LengthableObject::are_members_valid (is_project)
    || !ColoredObject::are_members_valid (is_project)
    || !ArrangerObject::are_members_valid (is_project))
    {
      return false;
    }

  return true;
}

ArrangerSelections *
AutomationRegion::get_arranger_selections () const
{
  return AUTOMATION_SELECTIONS.get ();
}

ArrangerWidget *
AutomationRegion::get_arranger_for_children () const
{
  return MW_AUTOMATION_ARRANGER;
}