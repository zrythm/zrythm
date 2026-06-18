// SPDX-FileCopyrightText: © 2022, 2024-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <span>

#include "dsp/chord_descriptor.h"
#include "utils/icloneable.h"
#include "utils/qt.h"

#include <QtQmlIntegration/qqmlintegration.h>

/**
 * A named collection of chord descriptors, optionally grouped by category.
 *
 * Used by ChordPresetManager for built-in and user presets, and by
 * ChordPadBank for applying presets to the chord pad.
 */
class ChordPreset : public QObject
{
  Q_OBJECT
  Q_PROPERTY (QString name READ name WRITE setName NOTIFY nameChanged)
  Q_PROPERTY (
    QString category READ category WRITE setCategory NOTIFY categoryChanged)
  Q_PROPERTY (bool isBuiltin READ isBuiltin CONSTANT FINAL)
  QML_ELEMENT

public:
  ChordPreset (QObject * parent = nullptr);
  ChordPreset (const QString &name, QObject * parent = nullptr);
  ChordPreset (
    const QString &name,
    const QString &category,
    bool           is_builtin,
    QObject *      parent = nullptr);

  // =================================================================
  // QML Interface
  // =================================================================

  QString       name () const;
  void          setName (const QString &name);
  Q_SIGNAL void nameChanged (const QString &name);

  QString       category () const;
  void          setCategory (const QString &category);
  Q_SIGNAL void categoryChanged (const QString &category);

  bool isBuiltin () const { return is_builtin_; }

  /**
   * @brief Informational text describing the preset's chords.
   */
  Q_INVOKABLE QString infoText () const;

  // =================================================================
  // Descriptor access
  // =================================================================

  std::span<const zrythm::utils::QObjectUniquePtr<zrythm::dsp::ChordDescriptor>>
  descriptors () const
  {
    return descr_;
  }

  void addDescriptor (
    zrythm::utils::QObjectUniquePtr<zrythm::dsp::ChordDescriptor> &&descr);

  friend bool operator== (const ChordPreset &lhs, const ChordPreset &rhs);

  friend void init_from (
    ChordPreset                   &obj,
    const ChordPreset             &other,
    zrythm::utils::ObjectCloneType clone_type);

private:
  static constexpr std::string_view kNameKey = "name";
  static constexpr std::string_view kDescriptorsKey = "descriptors";
  static constexpr std::string_view kCategoryKey = "category";
  friend void to_json (nlohmann::json &j, const ChordPreset &preset);
  friend void from_json (const nlohmann::json &j, ChordPreset &preset);

  /** Preset name. */
  QString name_;

  /** Category this preset belongs to (e.g., "Pop", "J-Rock"). */
  QString category_;

  /** Whether this is a built-in preset (shipped with the application). */
  bool is_builtin_ = false;

  /** Chord descriptors. */
  std::vector<zrythm::utils::QObjectUniquePtr<zrythm::dsp::ChordDescriptor>>
    descr_;
};
