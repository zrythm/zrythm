#ifndef TRACK_FILTER_PROXY_MODEL_H
#define TRACK_FILTER_PROXY_MODEL_H

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
  Q_INVOKABLE void clearFilters ();

protected:
  bool filterAcceptsRow (int source_row, const QModelIndex &source_parent)
    const override;

private:
  bool use_visible_filter_ = false;
  bool visible_filter_ = true;
  bool use_pinned_filter_ = false;
  bool pinned_filter_ = false;
};
} // namespace zrythm::gui

#endif // TRACK_FILTER_PROXY_MODEL_H