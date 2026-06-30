// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "dsp/chord_descriptor.h"
#include "structure/arrangement/chord_clip.h"
#include "structure/arrangement/chord_object.h"

#include <QAbstractListModel>
#include <QPointer>
#include <QtQmlIntegration/qqmlintegration.h>

namespace zrythm::gui::qquick
{

/**
 * @brief Derived QAbstractListModel that lists the unique chords used in a
 * ChordClip, grouped by ChordDescriptor::isEquivalent.
 *
 * Each row represents one unique chord. Rows are sorted alphabetically by
 * display name.
 */
class ChordRowListModel : public QAbstractListModel
{
  Q_OBJECT
  Q_PROPERTY (
    zrythm::structure::arrangement::ChordClip * chordClip READ chordClip WRITE
      setChordClip NOTIFY chordClipChanged)
  QML_ELEMENT

public:
  enum Roles
  {
    ChordNameRole = Qt::UserRole + 1,
    ChordDescriptorRole,
    ChordObjectCountRole
  };
  Q_ENUM (Roles)

  explicit ChordRowListModel (QObject * parent = nullptr);

  structure::arrangement::ChordClip * chordClip () const { return clip_; }
  void setChordClip (structure::arrangement::ChordClip * clip);

  QHash<int, QByteArray> roleNames () const override;
  int rowCount (const QModelIndex &parent = QModelIndex ()) const override;
  QVariant
  data (const QModelIndex &index, int role = Qt::DisplayRole) const override;

  /**
   * @brief Returns the row index (into this model) that the given chord object
   * belongs to, or -1 if not found.
   */
  Q_INVOKABLE int
  rowForChordObject (zrythm::structure::arrangement::ChordObject * object) const;

  /**
   * @brief Returns the representative ChordDescriptor for the given row.
   */
  Q_INVOKABLE zrythm::dsp::ChordDescriptor * descriptorAtRow (int row) const;

  /**
   * @brief Returns all ChordObjects on the given row (for batch editing).
   */
  Q_INVOKABLE QVariantList chordObjectsAtRow (int row) const;

  Q_SIGNAL void chordClipChanged ();

  /**
   * @brief Emitted after every rebuild (when chord objects are added, removed,
   * moved, or have their descriptors changed). QML delegates use this to
   * recompute derived span widths and row positions.
   */
  Q_SIGNAL void contentChanged ();

private:
  struct Row
  {
    dsp::ChordDescriptor *                                     descriptor;
    std::vector<zrythm::structure::arrangement::ChordObject *> objects;
  };

  void rebuild ();
  void connect_to_clip_model ();
  void disconnect_from_clip_model ();
  void connect_descriptor_signals ();
  void disconnect_descriptor_signals ();

  QPointer<structure::arrangement::ChordClip> clip_;
  std::vector<Row>                            rows_;
  std::vector<QMetaObject::Connection>        descriptor_connections_;
};

} // namespace zrythm::gui::qquick
