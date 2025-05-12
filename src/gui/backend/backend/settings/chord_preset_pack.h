// SPDX-FileCopyrightText: © 2022, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __SETTINGS_CHORD_PRESET_PACK_H__
#define __SETTINGS_CHORD_PRESET_PACK_H__

#include "zrythm-config.h"

#include <utility>

#include "gui/backend/backend/settings/chord_preset.h"
#include "utils/format.h"
#include "utils/icloneable.h"

/**
 * @addtogroup settings
 *
 * @{
 */

/**
 * Chord preset pack.
 */
class ChordPresetPack final : public QObject, public ICloneable<ChordPresetPack>
{
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY (QString name READ getName WRITE setName NOTIFY nameChanged)
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

  void
  init_after_cloning (const ChordPresetPack &other, ObjectCloneType clone_type)
    override;

  bool contains_name (const NameT &name) const;

  bool contains_preset (const ChordPreset &pset) const;

  void add_preset (const ChordPreset &pset);

  void delete_preset (const ChordPreset &pset);

  int get_preset_index (const ChordPreset &pset) const
  {
    auto it = std::ranges::find (presets_, &pset);
    z_return_val_if_fail (it != presets_.end (), -1);
    return static_cast<int> (std::distance (presets_.begin (), it));
  }

  // GMenuModel * generate_context_menu () const;

private:
  static constexpr std::string_view kNameKey = "name";
  static constexpr std::string_view kPresetsKey = "presets";
  static constexpr std::string_view kIsStandardKey = "isStandard";
  friend void to_json (nlohmann::json &j, const ChordPresetPack &pack)
  {
    j[kNameKey] = pack.name_;
    j[kPresetsKey] = pack.presets_;
    j[kIsStandardKey] = pack.is_standard_;
  }
  friend void from_json (const nlohmann::json &j, ChordPresetPack &pack)
  {
    j.at (kNameKey).get_to (pack.name_);
    for (const auto &pset : j.at (kPresetsKey))
      {
        // TODO: check if this is correct
        auto * pset_ptr = new ChordPreset (&pack);
        from_json (pset, *pset_ptr);
        pack.presets_.push_back (pset_ptr);
      }
    j.at (kIsStandardKey).get_to (pack.is_standard_);
  }

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

/**
 * @}
 */

#endif
