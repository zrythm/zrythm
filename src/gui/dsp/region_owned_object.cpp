// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/backend/backend/zrythm.h"
#include "gui/dsp/region.h"
#include "gui/dsp/region_owned_object.h"

#include "utils/gtest_wrapper.h"

void
RegionOwnedObject::copy_members_from (
  const RegionOwnedObject &other,
  ObjectCloneType          clone_type)
{
  region_id_ = other.region_id_;
  index_ = other.index_;
}

void
RegionOwnedObject::init_loaded_base ()
{
}

template <typename RegionT>
RegionT *
RegionOwnedObjectImpl<RegionT>::get_region () const
{
  const auto &region = RegionImpl<RegionT>::find (region_id_);
  return region;
}

template <typename RegionT>
void
RegionOwnedObjectImpl<RegionT>::get_global_start_pos (Position &pos) const
{
  auto r = get_region ();
  pos = *static_cast<Position *> (pos_);
  pos.add_ticks (r->pos_->ticks_, AUDIO_ENGINE->frames_per_tick_);
}

void
RegionOwnedObject::set_region_and_index (const Region &region, int index)
{
  region_id_ = region.id_;
  index_ = index;
  track_id_ = region.track_id_;
  if (type_ == Type::MidiNote)
    {
      auto note = dynamic_cast<MidiNote *> (this);
      note->vel_->track_id_ = region.track_id_;
    }

  /* note: this was only done for automation points, not sure why */
  /* set the info to the transient too */
  if ((ZRYTHM_HAVE_UI || ZRYTHM_TESTING) && PROJECT->loaded_ && transient_)
    // && arranger_object_should_orig_be_visible (*this, nullptr))
    {
      auto trans_obj = get_transient<RegionOwnedObject> ();
      trans_obj->region_id_ = region.id_;
      trans_obj->index_ = index_;
      trans_obj->track_id_ = region.track_id_;
      if (type_ == Type::MidiNote)
        {
          auto note = dynamic_cast<MidiNote *> (trans_obj);
          note->vel_->track_id_ = region.track_id_;
        }
    }
}

template class RegionOwnedObjectImpl<MidiRegion>;
template class RegionOwnedObjectImpl<AutomationRegion>;
template class RegionOwnedObjectImpl<ChordRegion>;
