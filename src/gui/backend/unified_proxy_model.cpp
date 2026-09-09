// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <algorithm>

#include "gui/backend/unified_proxy_model.h"
#include "utils/logger.h"

#include <fmt/format.h>

namespace zrythm::gui
{
UnifiedProxyModel::UnifiedProxyModel (QObject * parent)
    : QConcatenateTablesProxyModel (parent)
{
}

void
UnifiedProxyModel::addSourceModel (QAbstractItemModel * model)
{
  if (model == nullptr)
    return;

  const auto count_it = registration_counts_.find (model);
  if (count_it == registration_counts_.end ())
    {
      registration_counts_.insert (model, 1);

      // Discard the count entry when the model is destroyed: keeping it
      // would leave a dangling pointer that a model later allocated at
      // the same address would be counted under. The base class keeps
      // the destroyed model in its source list (removing it during
      // destruction is not possible because the base removal reads the
      // model's row count)
      QObject::connect (model, &QObject::destroyed, this, [this, model] () {
        registration_counts_.remove (model);
      });

      QConcatenateTablesProxyModel::addSourceModel (model);
      return;
    }

  ++count_it.value ();
}

void
UnifiedProxyModel::removeSourceModel (QAbstractItemModel * model)
{
  if (model == nullptr)
    return;

  const auto count_it = registration_counts_.find (model);
  if (count_it == registration_counts_.end ())
    {
      z_warning (
        "UnifiedProxyModel::removeSourceModel: model {} is not registered",
        fmt::ptr (model));
      return;
    }

  if (--count_it.value () > 0)
    return;

  registration_counts_.erase (count_it);
  QConcatenateTablesProxyModel::removeSourceModel (model);
}

QModelIndex
UnifiedProxyModel::index (int row, int column, const QModelIndex &parent) const
{
  if (parent.isValid ())
    return {};

  // Rows not covered by any registered source model at the time of the
  // call resolve to an invalid index, like any flat model returns for
  // out-of-range rows. Besides plain out-of-range rows, this covers rows
  // between the current source row counts and the cached total kept by
  // the base class, which appear while a source row addition or removal
  // has not been processed by this model yet
  int available_rows = 0;
  for (
    auto it = registration_counts_.keyBegin ();
    it != registration_counts_.keyEnd (); ++it)
    {
      available_rows += (*it)->rowCount ();
      if (available_rows > row)
        break;
    }
  if (row < 0 || row >= std::min (available_rows, rowCount ()))
    return {};
  if (column < 0 || column >= columnCount ())
    return {};

  return QConcatenateTablesProxyModel::index (row, column, parent);
}

QModelIndex
UnifiedProxyModel::mapFromSource (const QModelIndex &sourceIndex) const
{
  if (!sourceIndex.isValid ())
    return {};

  const auto * source_model = sourceIndex.model ();
  if (!registration_counts_.contains (source_model))
    {
      z_warning (
        "UnifiedProxyModel::mapFromSource: model {} is not registered",
        fmt::ptr (source_model));
      return {};
    }

  return QConcatenateTablesProxyModel::mapFromSource (sourceIndex);
}
QModelIndex
UnifiedProxyModel::mapToSource (const QModelIndex &proxyIndex) const
{
  return QConcatenateTablesProxyModel::mapToSource (proxyIndex);
}
}
