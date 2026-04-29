// SPDX-FileCopyrightText: © 2022, 2024-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "gui/backend/backend/settings/chord_preset.h"
#include "utils/icloneable.h"

/**
 * Chord preset pack.
 */
class ChordPresetPack : public QObject
{
  Q_OBJECT
  Q_PROPERTY (QString name READ getName WRITE setName NOTIFY nameChanged)
  QML_ELEMENT

public:
  using NameT = QString;
  ChordPresetPack (QObject * parent = nullptr);
  ChordPresetPack (
    const NameT &name,
    bool         is_standard,
    QObject *    parent = nullptr);

  // ====================================================================
  // QML Interface
  // ====================================================================

  QString       getName () const { return name_; }
  void          setName (const QString &name);
  Q_SIGNAL void nameChanged (const QString &name);

  // ============================================================================

  std::string get_document_type () const { return "Zrythm Chord Preset Pack"; }
  int         get_format_major_version () const { return 2; }
  int         get_format_minor_version () const { return 0; }

  friend void init_from (
    ChordPresetPack               &obj,
    const ChordPresetPack         &other,
    zrythm::utils::ObjectCloneType clone_type);

  bool contains_name (const NameT &name) const;

  bool contains_preset (const ChordPreset &pset) const;

  void add_preset (const ChordPreset &pset);

  void delete_preset (const ChordPreset &pset);

  int get_preset_index (const ChordPreset &pset) const;

  // GMenuModel * generate_context_menu () const;

private:
  static constexpr std::string_view kNameKey = "name";
  static constexpr std::string_view kPresetsKey = "presets";
  static constexpr std::string_view kIsStandardKey = "isStandard";
  friend void to_json (nlohmann::json &j, const ChordPresetPack &pack);
  friend void from_json (const nlohmann::json &j, ChordPresetPack &pack);

public:
  /** Pack name. */
  NameT name_;

  /** Presets. */
  std::vector<ChordPreset *> presets_;

  /** Whether this is a standard preset pack (not user-defined). */
  bool is_standard_ = false;
};

inline bool
operator== (const ChordPresetPack &lhs, const ChordPresetPack &rhs)
{
  return lhs.name_ == rhs.name_ && lhs.presets_ == rhs.presets_
         && lhs.is_standard_ == rhs.is_standard_;
}
