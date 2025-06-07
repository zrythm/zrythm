// SPDX-FileCopyrightText: © 2022, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __SETTINGS_CHORD_PRESET_H__
#define __SETTINGS_CHORD_PRESET_H__

#include "zrythm-config.h"

#include "dsp/chord_descriptor.h"
#include "gui/backend/backend/chord_editor.h"
#include "utils/types.h"

#include <QtQmlIntegration>

class ChordPresetPack;

/**
 * @addtogroup settings
 *
 * @{
 */

/**
 * A preset of chord descriptors.
 */
class ChordPreset final : public QObject
{
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY (QString name READ getName WRITE setName NOTIFY nameChanged)
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
  utils::Utf8String get_info_text () const;

  // GMenuModel * generate_context_menu () const;

  friend void init_from (
    ChordPreset           &obj,
    const ChordPreset     &other,
    utils::ObjectCloneType clone_type);

private:
  static constexpr std::string_view kNameKey = "name";
  static constexpr std::string_view kDescriptorsKey = "descriptors";
  friend void to_json (nlohmann::json &j, const ChordPreset &preset)
  {
    j[kNameKey] = preset.name_;
    j[kDescriptorsKey] = preset.descr_;
  }
  friend void from_json (const nlohmann::json &j, ChordPreset &preset)
  {
    j.at (kNameKey).get_to (preset.name_);
    j.at (kDescriptorsKey).get_to (preset.descr_);
  }

public:
  /** Preset name. */
  NameT name_;

  /** Chord descriptors. */
  std::vector<dsp::ChordDescriptor> descr_;

  /** Pointer to owner pack. */
  ChordPresetPack * pack_ = nullptr;
};

inline bool
operator== (const ChordPreset &lhs, const ChordPreset &rhs)
{
  return lhs.name_ == rhs.name_ && lhs.descr_ == rhs.descr_;
}

/**
 * @}
 */

#endif
