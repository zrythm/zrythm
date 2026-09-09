// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <QConcatenateTablesProxyModel>
#include <QHash>
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
   *
   * Repeated registrations of the same model are counted; the model is
   * added as a source on the first registration only.
   *
   * @param model The model to add.
   */
  Q_INVOKABLE void addSourceModel (QAbstractItemModel * model);

  /**
   * @brief Removes a source model from the unified model.
   *
   * Removes the model as a source on the last outstanding registration.
   * Removing a model that is not registered is refused with a warning
   * and leaves the unified model unchanged.
   *
   * @param model The model to remove.
   */
  Q_INVOKABLE void removeSourceModel (QAbstractItemModel * model);

  /**
   * @brief Returns the index for the given row and column.
   *
   * Returns an invalid index when @a parent is valid, or when @a row or
   * @a column fall outside the rows covered by the source models at the
   * time of the call. The latter includes rows between a source model's
   * current row count and this model's cached total, which exist while a
   * row addition or removal in a source model has not been processed by
   * this model yet.
   *
   * @param row Row in the unified model.
   * @param column Column in the unified model.
   * @param parent Parent index (the unified model is flat).
   * @return The corresponding index, or an invalid index for
   *         out-of-range coordinates.
   */
  QModelIndex
  index (int row, int column, const QModelIndex &parent = {}) const override;

  /**
   * @brief Maps a source model index to the unified model index.
   *
   * Registration is explicit: returns an invalid index if @a sourceIndex
   * is invalid or its model is not a registered source model. Mapping an
   * unregistered model is reported as a warning, since sources are
   * expected to be registered before any of their indexes is mapped.
   *
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

private:
  /**
   * @brief Number of outstanding registrations per source model.
   *
   * Entries are discarded when a model is destroyed. The base class
   * keeps destroyed models in its own source list until they are
   * explicitly removed, which cannot happen during destruction.
   */
  QHash<const QAbstractItemModel *, int> registration_counts_;
};
}
