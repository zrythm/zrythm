// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/backend/unified_proxy_model.h"

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

  if (QConcatenateTablesProxyModel::sourceModels ().contains (model))
    return;

  QConcatenateTablesProxyModel::addSourceModel (model);
}

void
UnifiedProxyModel::removeSourceModel (QAbstractItemModel * model)
{
  QConcatenateTablesProxyModel::removeSourceModel (model);
}

QModelIndex
UnifiedProxyModel::mapFromSource (const QModelIndex &sourceIndex) const
{
  return QConcatenateTablesProxyModel::mapFromSource (sourceIndex);
}
QModelIndex
UnifiedProxyModel::mapToSource (const QModelIndex &proxyIndex) const
{
  return QConcatenateTablesProxyModel::mapToSource (proxyIndex);
}
}
