// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "dsp/chord_preset.h"
#include "utils/qt.h"

#include <QAbstractListModel>
#include <QtQmlIntegration/qqmlintegration.h>

/**
 * @brief Flat list model of all chord presets (built-in + user).
 *
 * Replaces the old ChordPresetPack + ChordPresetPackManager hierarchy.
 * Each preset has a @ref category_ string for grouping (e.g., "Pop",
 * "J-Rock"). Built-in presets are created at startup; user presets are loaded
 * from JSON.
 *
 * QML usage:
 * @code
 * Repeater {
 *     model: presetManager.categories()
 *     delegate: Menu {
 *         title: modelData
 *         Repeater {
 *             model: presetManager.presetsInCategory(modelData)
 *             delegate: MenuItem {
 *                 text: modelData.name
 *                 onTriggered: padOp.applyPreset(modelData)
 *             }
 *         }
 *     }
 * }
 * @endcode
 */
class ChordPresetManager : public QAbstractListModel
{
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE ("")

public:
  enum Roles
  {
    PresetRole = Qt::UserRole + 1,
    NameRole,
    CategoryRole,
    IsBuiltinRole,
  };

  explicit ChordPresetManager (QObject * parent = nullptr);

  // ========================================================================
  // QAbstractListModel interface
  // ========================================================================

  QHash<int, QByteArray> roleNames () const override;
  int      rowCount (const QModelIndex &parent = {}) const override;
  QVariant data (const QModelIndex &index, int role) const override;

  // ========================================================================
  // QML Interface
  // ========================================================================

  Q_INVOKABLE QStringList categories () const;

  Q_INVOKABLE QVariantList presetsInCategory (const QString &category) const;

  // ========================================================================

  void load_user_presets ();

private:
  void add_builtin_presets ();

  static std::filesystem::path get_user_presets_path ();

  std::vector<zrythm::utils::QObjectUniquePtr<ChordPreset>> presets_;
};
