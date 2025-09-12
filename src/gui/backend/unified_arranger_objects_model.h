// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <QConcatenateTablesProxyModel>
#include <QObject>
#include <QtQmlIntegration>

namespace zrythm::gui
{
class UnifiedArrangerObjectsModel : public QConcatenateTablesProxyModel
{
  Q_OBJECT
  QML_ELEMENT

public:
  explicit UnifiedArrangerObjectsModel (QObject * parent = nullptr);

  Q_INVOKABLE void        addSourceModel (QAbstractItemModel * model);
  Q_INVOKABLE void        removeSourceModel (QAbstractItemModel * model);
  Q_INVOKABLE QModelIndex mapFromSource (const QModelIndex &sourceIndex) const;
  Q_INVOKABLE QModelIndex mapToSource (const QModelIndex &proxyIndex) const;
};
}
