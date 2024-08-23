// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/region.h"
#include "dsp/region_owned_object.h"
#include "gui/widgets/arranger_object.h"
#include "zrythm.h"

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

  /* note: this was only done for automation points, not sure why */
  /* set the info to the transient too */
  if (
    (ZRYTHM_HAVE_UI || ZRYTHM_TESTING) && PROJECT->loaded_ && transient_
    && arranger_object_should_orig_be_visible (
      dynamic_cast<ArrangerObject *> (this), nullptr))
    {
      const auto trans_obj = dynamic_cast<RegionOwnedObject *> (transient_);
      trans_obj->region_id_ = region.id_;
      trans_obj->index_ = index_;
    }
}

template class RegionOwnedObjectImpl<MidiRegion>;
template class RegionOwnedObjectImpl<AutomationRegion>;
template class RegionOwnedObjectImpl<ChordRegion>;