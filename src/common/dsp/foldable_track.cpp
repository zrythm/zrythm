// SPDX-License-Identifier: LicenseRef-ZrythmLicense
/*
 * SPDX-FileCopyrightText: © 2021 Alexandros Theodotou <alex@zrythm.org>
 */

#include <cstdlib>

#include "common/dsp/foldable_track.h"
#include "common/dsp/track.h"
#include "common/dsp/tracklist.h"
#include "common/utils/flags.h"
#include "common/utils/rt_thread_id.h"
#include "gui/backend/backend/actions/tracklist_selections.h"
#include "gui/backend/backend/project.h"
#include "gui/backend/backend/zrythm.h"

bool
FoldableTrack::is_status (MixerStatus status) const
{
  z_return_val_if_fail (tracklist_, false);
  bool all_soloed = size_ > 1;
  bool has_channel_tracks = false;
  for (int i = 1; i < size_; i++)
    {
      int     pos = pos_ + i;
      Track * child = Track::from_variant (tracklist_->get_track (pos));
      z_return_val_if_fail (IS_TRACK_AND_NONNULL (child), false);

      if (child->has_channel ())
        has_channel_tracks = true;
      else
        continue;

      auto ch_child = dynamic_cast<ChannelTrack *> (child);
      switch (status)
        {
        case MixerStatus::Muted:
          if (!ch_child->get_muted ())
            return false;
          break;
        case MixerStatus::Soloed:
          if (!ch_child->get_soloed ())
            return false;
          break;
        case MixerStatus::ImpliedSoloed:
          if (!ch_child->get_implied_soloed ())
            return false;
          break;
        case MixerStatus::Listened:
          if (!ch_child->get_listened ())
            return false;
          break;
        }
    }
  return has_channel_tracks && all_soloed;
}

bool
FoldableTrack::is_direct_child (const Track &child) const
{
  std::vector<FoldableTrack *> parents;
  child.add_folder_parents (parents, true);

  bool match = !parents.empty ();
  if (match)
    {
      auto parent = parents.front ();
      if (parent != this)
        {
          match = false;
        }
    }

  return match;
}

bool
FoldableTrack::is_child (const Track &child) const
{
  std::vector<FoldableTrack *> parents;
  child.add_folder_parents (parents, false);

  bool match = false;
  for (auto parent : parents)
    {
      if (parent == this)
        {
          match = true;
          break;
        }
    }

  return match;
}

void
FoldableTrack::add_to_size (int delta)
{
  std::vector<FoldableTrack *> parents;
  add_folder_parents (parents, false);

  size_ += delta;
  z_debug ("new {} size: {} (added {})", name_, size_, delta);
  for (auto parent : parents)
    {
      parent->size_ += delta;
      z_debug (
        "new {} size: {} (added {})", parent->name_, parent->size_, delta);
    }
}

void
FoldableTrack::
  set_folded (bool folded, bool trigger_undo, bool auto_select, bool fire_events)
{
  z_info ("Setting track {} folded ({})", name_, folded);
  if (auto_select)
    {
      select (true, true, fire_events);
    }

  if (trigger_undo)
    {
      auto highest_track_var = TRACKLIST_SELECTIONS->get_highest_track ();
      std::visit (
        [&] (auto &&highest_track) {
          if (
            TRACKLIST_SELECTIONS->get_num_tracks () != 1
            || dynamic_cast<FoldableTrack *> (highest_track) != this)
            {
              throw ZrythmException (
                "Cannot set track folded: not the highest track");
            }
        },
        highest_track_var);

      try
        {
          UNDO_MANAGER->perform (std::make_unique<FoldTracksAction> (
            TRACKLIST_SELECTIONS->gen_tracklist_selections ().get (), folded));
        }
      catch (const ZrythmException &e)
        {
          e.handle (QObject::tr ("Cannot set track folded"));
          return;
        }
    }
  else
    {
      folded_ = folded;

      if (fire_events)
        {
          // EVENTS_PUSH (EventType::ET_TRACK_FOLD_CHANGED, this);
        }
    }
}