// SPDX-FileCopyrightText: © 2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __SETTINGS_CHORD_PRESET_H__
#define __SETTINGS_CHORD_PRESET_H__

#include "zrythm-config.h"

#include "common/dsp/chord_descriptor.h"
#include "common/utils/types.h"
#include "gui/backend/backend/chord_editor.h"

class ChordPresetPack;
TYPEDEF_STRUCT_UNDERSCORED (GMenuModel);

/**
 * @addtogroup settings
 *
 * @{
 */

/**
 * A preset of chord descriptors.
 */
class ChordPreset final : public ISerializable<ChordPreset>
{
public:
  ChordPreset () = default;
  ChordPreset (const std::string &name);

  /**
   * Gets informational text.
   */
  std::string get_info_text () const;

  std::string get_name () const;

  void set_name (const std::string &name);

  GMenuModel * generate_context_menu () const;

  DECLARE_DEFINE_FIELDS_METHOD ();

public:
  /** Preset name. */
  std::string name_;

  /** Chord descriptors. */
  std::vector<ChordDescriptor> descr_;

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
