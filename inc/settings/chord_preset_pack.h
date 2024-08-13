// SPDX-FileCopyrightText: © 2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __SETTINGS_CHORD_PRESET_PACK_H__
#define __SETTINGS_CHORD_PRESET_PACK_H__

#include "zrythm-config.h"

#include <utility>

#include "settings/chord_preset.h"
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
    : public ISerializable<ChordPresetPack>,
      public ICloneable<ChordPresetPack>
{
public:
  ChordPresetPack () = default;
  ChordPresetPack (std::string name, bool is_standard)
      : name_ (std::move (name)), is_standard_ (is_standard)
  {
  }

  DECLARE_DEFINE_FIELDS_METHOD ();

  std::string get_document_type () const override
  {
    return "Zrythm Chord Preset Pack";
  }
  int get_format_major_version () const override { return 2; }
  int get_format_minor_version () const override { return 0; }

  void init_after_cloning (const ChordPresetPack &other) override
  {
    name_ = other.name_;
    is_standard_ = other.is_standard_;
    presets_ = other.presets_;
    for (auto &pset : presets_)
      {
        pset.pack_ = this;
      }
  }

  bool contains_name (const std::string &name) const;

  bool contains_preset (const ChordPreset &pset) const;

  void add_preset (const ChordPreset &pset);

  void delete_preset (const ChordPreset &pset);

  int get_preset_index (const ChordPreset &pset) const
  {
    auto it = std::find (presets_.begin (), presets_.end (), pset);
    z_return_val_if_fail (it != presets_.end (), -1);
    return std::distance (presets_.begin (), it);
  }

  std::string get_name () const { return name_; }

  static std::string name_getter (void * pack)
  {
    return ((ChordPresetPack *) pack)->get_name ();
  }

  static void name_setter (void * pack, const std::string &name)
  {
    ((ChordPresetPack *) pack)->set_name (name);
  }

  /**
   * @brief Sets @ref name_ and emits @ref
   * EventType::ET_CHORD_PRESET_PACK_EDITED.
   */
  void set_name (const std::string &name);

  GMenuModel * generate_context_menu () const;

  /**
   * @brief Used in GTK callbacks.
   */
  static void destroy_cb (void * data) { delete (ChordPresetPack *) data; };

public:
  /** Pack name. */
  std::string name_;

  /** Presets. */
  std::vector<ChordPreset> presets_;

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
