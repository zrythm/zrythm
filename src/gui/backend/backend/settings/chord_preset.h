// SPDX-FileCopyrightText: © 2022, 2024-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "dsp/chord_descriptor.h"
#include "structure/arrangement/chord_editor.h"
#include "utils/utf8_string.h"

#include <QtQmlIntegration/qqmlintegration.h>

class ChordPresetPack;

/**
 * A preset of chord descriptors.
 */
class ChordPreset : public QObject
{
  Q_OBJECT
  Q_PROPERTY (QString name READ getName WRITE setName NOTIFY nameChanged)
  QML_ELEMENT

public:
  using NameT = QString;
  ChordPreset (QObject * parent = nullptr);
  ChordPreset (const NameT &name, QObject * parent = nullptr);

  // =================================================================
  // QML Interface
  // =================================================================

  NameT         getName () const;
  void          setName (const NameT &name);
  Q_SIGNAL void nameChanged (const NameT &name);

  // =================================================================

  /**
   * Gets informational text.
   */
  zrythm::utils::Utf8String get_info_text () const;

  // GMenuModel * generate_context_menu () const;

  friend void init_from (
    ChordPreset                   &obj,
    const ChordPreset             &other,
    zrythm::utils::ObjectCloneType clone_type);

private:
  static constexpr std::string_view kNameKey = "name";
  static constexpr std::string_view kDescriptorsKey = "descriptors";
  friend void to_json (nlohmann::json &j, const ChordPreset &preset);
  friend void from_json (const nlohmann::json &j, ChordPreset &preset);

public:
  /** Preset name. */
  NameT name_;

  /** Chord descriptors. */
  std::vector<zrythm::dsp::ChordDescriptor> descr_;

  /** Pointer to owner pack. */
  ChordPresetPack * pack_ = nullptr;
};

inline bool
operator== (const ChordPreset &lhs, const ChordPreset &rhs)
{
  return lhs.name_ == rhs.name_ && lhs.descr_ == rhs.descr_;
}
