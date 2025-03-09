// SPDX-FileCopyrightText: © 2022, 2024 Alexandros Theodotou <alex@zrythm.org>
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
class ChordPresetPack final
    : public QObject,
      public zrythm::utils::serialization::ISerializable<ChordPresetPack>,
      public ICloneable<ChordPresetPack>
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

  DECLARE_DEFINE_FIELDS_METHOD ();

  std::string get_document_type () const override
  {
    return "Zrythm Chord Preset Pack";
  }
  int get_format_major_version () const override { return 2; }
  int get_format_minor_version () const override { return 0; }

  void
  init_after_cloning (const ChordPresetPack &other, ObjectCloneType clone_type)
    override;

  bool contains_name (const NameT &name) const;

  bool contains_preset (const ChordPreset &pset) const;

  void add_preset (const ChordPreset &pset);

  void delete_preset (const ChordPreset &pset);

  int get_preset_index (const ChordPreset &pset) const
  {
    auto it = std::find (presets_.begin (), presets_.end (), pset);
    z_return_val_if_fail (it != presets_.end (), -1);
    return std::distance (presets_.begin (), it);
  }

  // GMenuModel * generate_context_menu () const;

  /**
   * @brief Used in GTK callbacks.
   */
  static void destroy_cb (void * data) { delete (ChordPresetPack *) data; };

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
