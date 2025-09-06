// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/backend/unified_arranger_objects_model.h"

namespace zrythm::gui
{
UnifiedArrangerObjectsModel::UnifiedArrangerObjectsModel (QObject * parent)
    : QConcatenateTablesProxyModel (parent)
{
}

void
UnifiedArrangerObjectsModel::addSourceModel (QAbstractItemModel * model)
{
  if (QConcatenateTablesProxyModel::sourceModels ().contains (model))
    return;

  QConcatenateTablesProxyModel::addSourceModel (model);
}

void
UnifiedArrangerObjectsModel::removeSourceModel (QAbstractItemModel * model)
{
  QConcatenateTablesProxyModel::removeSourceModel (model);
}

QModelIndex
UnifiedArrangerObjectsModel::mapFromSource (const QModelIndex &sourceIndex) const
{
  return QConcatenateTablesProxyModel::mapFromSource (sourceIndex);
}
QModelIndex
UnifiedArrangerObjectsModel::mapToSource (const QModelIndex &proxyIndex) const
{
  return QConcatenateTablesProxyModel::mapToSource (proxyIndex);
}
}
