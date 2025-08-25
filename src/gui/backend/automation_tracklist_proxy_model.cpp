// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/backend/automation_tracklist_proxy_model.h"
#include "structure/tracks/automation_tracklist.h"

AutomationTracklistProxyModel::AutomationTracklistProxyModel (QObject * parent)
    : QSortFilterProxyModel (parent)
{
}

void
AutomationTracklistProxyModel::setSourceModel (QAbstractItemModel * sourceModel)
{
  QSortFilterProxyModel::setSourceModel (sourceModel);
  QT_IGNORE_DEPRECATIONS (connect (
    sourceModel, &QAbstractItemModel::dataChanged, this,
    &AutomationTracklistProxyModel::invalidateFilter));
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
      QT_IGNORE_DEPRECATIONS (invalidateFilter ());
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
      QT_IGNORE_DEPRECATIONS (invalidateFilter ());
      Q_EMIT showOnlyCreatedChanged ();
    }
}

bool
AutomationTracklistProxyModel::filterAcceptsRow (
  int                source_row,
  const QModelIndex &source_parent) const
{
  auto source_model = qobject_cast<
    zrythm::structure::tracks::AutomationTracklist *> (sourceModel ());
  if (source_model == nullptr)
    return false;

  auto   index = source_model->index (source_row, 0, source_parent);
  auto * ath = qvariant_cast<
    zrythm::structure::tracks::AutomationTrackHolder *> (source_model->data (
    index,
    zrythm::structure::tracks::AutomationTracklist::AutomationTrackHolderRole));

  if (ath == nullptr)
    return false;

  if (show_only_created_ && !ath->created_by_user_)
    return false;

  if (show_only_visible_ && !ath->visible ())
    return false;

  return true;
}
