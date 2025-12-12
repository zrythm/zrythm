// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <QConcatenateTablesProxyModel>
#include <QObject>
#include <QtQmlIntegration/qqmlintegration.h>

namespace zrythm::gui
{

/**
 * @brief A unified model that concatenates multiple models.
 *
 * This class provides a single interface to access objects from multiple source
 * models, allowing unified selection and manipulation across different arranger
 * object types. It inherits from QConcatenateTablesProxyModel to combine
 * multiple models into one.
 */
class UnifiedProxyModel : public QConcatenateTablesProxyModel
{
  Q_OBJECT
  QML_ELEMENT

public:
  /**
   * @brief Constructor for the unified arranger objects model.
   * @param parent Parent QObject (optional).
   */
  explicit UnifiedProxyModel (QObject * parent = nullptr);

  /**
   * @brief Adds a source model to the unified model.
   * @param model The model to add.
   */
  Q_INVOKABLE void addSourceModel (QAbstractItemModel * model);

  /**
   * @brief Removes a source model from the unified model.
   * @param model The model to remove.
   */
  Q_INVOKABLE void removeSourceModel (QAbstractItemModel * model);

  /**
   * @brief Maps a source model index to the unified model index.
   * @param sourceIndex The index in the source model.
   * @return The corresponding index in the unified model.
   */
  Q_INVOKABLE QModelIndex mapFromSource (const QModelIndex &sourceIndex) const;

  /**
   * @brief Maps a unified model index to the source model index.
   * @param proxyIndex The index in the unified model.
   * @return The corresponding index in the source model.
   */
  Q_INVOKABLE QModelIndex mapToSource (const QModelIndex &proxyIndex) const;
};
}
