// SPDX-FileCopyrightText : © 2024-2025 Alexandros Theodotou<alex @zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "structure/tracks/automatable_track.h"

namespace zrythm::structure::tracks
{
AutomatableTrackMixin::AutomatableTrackMixin (
  AutomationTrackHolder::Dependencies dependencies,
  QObject *                           parent)
    : dependencies_ (dependencies),
      automation_tracklist_ (
        utils::make_qobject_unique<AutomationTracklist> (dependencies, this))
{
  setParent (parent);
}

void
init_from (
  AutomatableTrackMixin       &obj,
  const AutomatableTrackMixin &other,
  utils::ObjectCloneType       clone_type)
{
  obj.automation_tracklist_ = utils::clone_unique_qobject (
    *other.automation_tracklist_, &obj, clone_type, obj.dependencies_);
  obj.automation_visible_ = other.automation_visible_;
}

void
AutomatableTrackMixin::setAutomationVisible (const bool visible)
{
  if (automation_visible_ == visible)
    return;

  automation_visible_ = visible;

  if (visible)
    {
      /* if no visible automation tracks, set the first one visible */
      auto *      atl = automationTracklist ();
      const auto &ath =
        const_cast<const AutomationTracklist *> (atl)
          ->automation_track_holders ();

      if (
        std::ranges::none_of (ath, [] (const auto &h) { return h->visible (); }))
        {
          auto * at = atl->get_first_invisible_automation_track_holder ();
          if (at != nullptr)
            {
              at->created_by_user_ = true;
              at->setVisible (true);
            }
        }
    }

  Q_EMIT automationVisibleChanged (visible);
}
}
