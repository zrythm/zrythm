// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __GUI_BACKEND_AUTOMATION_TRACKLIST_PROXY_MODEL_H__
#define __GUI_BACKEND_AUTOMATION_TRACKLIST_PROXY_MODEL_H__

#include <QSortFilterProxyModel>
#include <QtQmlIntegration/qqmlintegration.h>

/**
 * Proxy model for filtering/sorting automation tracks.
 */
class AutomationTracklistProxyModel : public QSortFilterProxyModel
{
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY (
    bool showOnlyVisible READ showOnlyVisible WRITE setShowOnlyVisible NOTIFY
      showOnlyVisibleChanged)
  Q_PROPERTY (
    bool showOnlyCreated READ showOnlyCreated WRITE setShowOnlyCreated NOTIFY
      showOnlyCreatedChanged)

public:
  explicit AutomationTracklistProxyModel (QObject * parent = nullptr);

  bool          showOnlyVisible () const;
  void          setShowOnlyVisible (bool show);
  Q_SIGNAL void showOnlyVisibleChanged ();

  bool          showOnlyCreated () const;
  void          setShowOnlyCreated (bool show);
  Q_SIGNAL void showOnlyCreatedChanged ();

protected:
  bool filterAcceptsRow (int source_row, const QModelIndex &source_parent)
    const override;

  void setSourceModel (QAbstractItemModel * sourceModel) override;

private:
  bool show_only_visible_ = true;
  bool show_only_created_ = true;
};

#endif
