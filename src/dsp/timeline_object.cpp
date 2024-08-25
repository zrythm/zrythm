// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/timeline_object.h"
#include "dsp/track.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/main_notebook.h"
#include "gui/widgets/timeline_arranger.h"
#include "gui/widgets/timeline_panel.h"
#include "zrythm_app.h"

ArrangerWidget *
TimelineObject::get_arranger () const
{
  Track * track = get_track ();
  z_return_val_if_fail (track, nullptr);
  if (track->is_pinned ())
    {
      return (ArrangerWidget *) (MW_PINNED_TIMELINE);
    }
  else
    {
      return (ArrangerWidget *) (MW_TIMELINE);
    }
}