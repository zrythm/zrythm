// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/region.h"
#include "dsp/region_owned_object.h"
#include "gui/cpp/gtk_widgets/arranger_object.h"
#include "utils/gtest_wrapper.h"
#include "zrythm.h"

void
RegionOwnedObject::copy_members_from (const RegionOwnedObject &other)
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
  return region.get ();
}

template <typename RegionT>
void
RegionOwnedObjectImpl<RegionT>::get_global_start_pos (Position &pos) const
{
  auto r = get_region ();
  pos = pos_;
  pos.add_ticks (r->pos_.ticks_);
}

void
RegionOwnedObject::set_region_and_index (const Region &region, int index)
{
  region_id_ = region.id_;
  index_ = index;
  track_name_hash_ = region.track_name_hash_;
  if (type_ == Type::MidiNote)
    {
      auto note = dynamic_cast<MidiNote *> (this);
      note->vel_->track_name_hash_ = region.track_name_hash_;
    }

  /* note: this was only done for automation points, not sure why */
  /* set the info to the transient too */
  if (
    (ZRYTHM_HAVE_UI || ZRYTHM_TESTING) && PROJECT->loaded_ && transient_
    && arranger_object_should_orig_be_visible (*this, nullptr))
    {
      auto trans_obj = get_transient<RegionOwnedObject> ();
      trans_obj->region_id_ = region.id_;
      trans_obj->index_ = index_;
      trans_obj->track_name_hash_ = region.track_name_hash_;
      if (type_ == Type::MidiNote)
        {
          auto note = dynamic_pointer_cast<MidiNote> (trans_obj);
          note->vel_->track_name_hash_ = region.track_name_hash_;
        }
    }
}

template class RegionOwnedObjectImpl<MidiRegion>;
template class RegionOwnedObjectImpl<AutomationRegion>;
template class RegionOwnedObjectImpl<ChordRegion>;