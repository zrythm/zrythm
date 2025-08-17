// SPDX-FileCopyrightText: © 2021, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "structure/tracks/track.h"

namespace zrythm::structure::tracks
{
/**
 * @brief Mixin class for a foldable track.
 */
class FoldableTrackMixin : public QAbstractListModel
{
  Q_OBJECT
  Q_PROPERTY (bool folded READ folded WRITE setFolded NOTIFY foldedChanged)
  QML_ELEMENT
  QML_UNCREATABLE ("")

public:
  FoldableTrackMixin (
    TrackRegistry &track_registry,
    QObject *      parent = nullptr) noexcept;
  ~FoldableTrackMixin () noexcept override = default;
  Z_DISABLE_COPY_MOVE (FoldableTrackMixin)

  enum TrackRoles
  {
    TrackPtrRole = Qt::UserRole + 1,
  };

  // ========================================================================
  // QML Interface
  // ========================================================================

  bool folded () const { return folded_; }
  void setFolded (bool folded)
  {
    if (folded_ == folded)
      return;

    folded_ = folded;
    Q_EMIT foldedChanged (folded);
  }
  Q_SIGNAL void foldedChanged (bool folded);

  QHash<int, QByteArray> roleNames () const override;
  int rowCount (const QModelIndex &parent = QModelIndex ()) const override;
  QVariant
  data (const QModelIndex &index, int role = Qt::DisplayRole) const override;

  // ========================================================================

private:
  static constexpr auto kChildrenKey = "children"sv;
  static constexpr auto kFoldedKey = "folded"sv;
  friend void to_json (nlohmann::json &j, const FoldableTrackMixin &track);
  friend void from_json (const nlohmann::json &j, FoldableTrackMixin &track);

private:
  TrackRegistry &track_registry_;

  /** Whether currently folded. */
  bool folded_ = false;

  std::vector<TrackUuidReference> children_;
};

} // namespace zrythm::structure::tracks
