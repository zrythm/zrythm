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
}

void
RegionOwnedObject::init_loaded_base ()
{
}

void
RegionOwnedObject::set_region_and_index (const Region &region)
{
  region_id_ = region.get_uuid ();
  set_track_id (region.get_track_id ());

#if 0
  /* note: this was only done for automation points, not sure why */
  /* set the info to the transient too */
  if ((ZRYTHM_HAVE_UI || ZRYTHM_TESTING) && PROJECT->loaded_ && transient_)
    // && arranger_object_should_orig_be_visible (*this, nullptr))
    {
      auto trans_obj = get_transient<RegionOwnedObject> ();
      trans_obj->region_id_ = region.id_;
      trans_obj->index_ = index_;
      trans_obj->track_id_ = region.track_id_;
    }
#endif
}
