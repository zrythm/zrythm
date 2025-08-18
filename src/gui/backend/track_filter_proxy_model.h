// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <QSortFilterProxyModel>
#include <QtQmlIntegration>

namespace zrythm::gui
{
class TrackFilterProxyModel : public QSortFilterProxyModel
{
  Q_OBJECT
  QML_ELEMENT

public:
  explicit TrackFilterProxyModel (QObject * parent = nullptr);

  Q_INVOKABLE void addVisibilityFilter (bool visible);
  Q_INVOKABLE void addPinnedFilter (bool pinned);
  Q_INVOKABLE void addChannelFilter (bool channel);
  Q_INVOKABLE void clearFilters ();

protected:
  bool filterAcceptsRow (int source_row, const QModelIndex &source_parent)
    const override;

private:
  bool use_visible_filter_ = false;
  bool visible_filter_ = true;
  bool use_pinned_filter_ = false;
  bool pinned_filter_ = false;
  bool use_channel_filter_ = false;
  bool channel_filter_ = false;
};
} // namespace zrythm::gui
