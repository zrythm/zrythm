// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/backend/automation_tracklist_proxy_model.h"

#include "gui/dsp/automation_track.h"
#include "gui/dsp/automation_tracklist.h"

AutomationTracklistProxyModel::AutomationTracklistProxyModel (QObject * parent)
    : QSortFilterProxyModel (parent)
{
}

void
AutomationTracklistProxyModel::setSourceModel (QAbstractItemModel * sourceModel)
{
  QSortFilterProxyModel::setSourceModel (sourceModel);
  connect (
    sourceModel, &QAbstractItemModel::dataChanged, this,
    &AutomationTracklistProxyModel::invalidateFilter);
}

bool
AutomationTracklistProxyModel::showOnlyVisible () const
{
  return show_only_visible_;
}

void
AutomationTracklistProxyModel::setShowOnlyVisible (bool show)
{
  if (show != show_only_visible_)
    {
      show_only_visible_ = show;
      invalidateFilter ();
      Q_EMIT showOnlyVisibleChanged ();
    }
}

bool
AutomationTracklistProxyModel::showOnlyCreated () const
{
  return show_only_created_;
}

void
AutomationTracklistProxyModel::setShowOnlyCreated (bool show)
{
  if (show != show_only_created_)
    {
      show_only_created_ = show;
      invalidateFilter ();
      Q_EMIT showOnlyCreatedChanged ();
    }
}

bool
AutomationTracklistProxyModel::filterAcceptsRow (
  int                source_row,
  const QModelIndex &source_parent) const
{
  auto source_model = qobject_cast<AutomationTracklist *> (sourceModel ());
  if (!source_model)
    return false;

  auto index = source_model->index (source_row, 0, source_parent);
  auto at = qvariant_cast<AutomationTrack *> (
    source_model->data (index, AutomationTracklist::AutomationTrackPtrRole));

  if (!at)
    return false;

  if (show_only_created_ && !at->created_)
    return false;

  if (show_only_visible_ && !at->visible_)
    return false;

  return true;
}
