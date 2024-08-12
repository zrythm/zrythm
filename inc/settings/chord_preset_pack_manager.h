// SPDX-FileCopyrightText: © 2022-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __SETTINGS_CHORD_PRESET_PACK_MANAGER_H__
#define __SETTINGS_CHORD_PRESET_PACK_MANAGER_H__

#include "zrythm-config.h"

#include "settings/chord_preset_pack.h"

/**
 * @addtogroup settings
 *
 * @{
 */

#define CHORD_PRESET_PACK_MANAGER (gZrythm->chord_preset_pack_manager_)

/**
 * Chord preset pack manager.
 */
class ChordPresetPackManager
{
public:
  ChordPresetPackManager () { add_standard_packs (); }

  /**
   * @brief Construct a new Chord Preset Pack Manager object
   *
   * @param scan_for_packs Whether to scan for preset packs.
   */
  ChordPresetPackManager (bool scan_for_packs) : ChordPresetPackManager ()
  {
    if (scan_for_packs)
      {
        add_user_packs ();
      }
  };

  size_t get_num_packs () const;

  ChordPresetPack * get_pack_at (size_t idx);

  ChordPresetPack * get_pack_for_preset (const ChordPreset &pset);

  int get_pack_index (const ChordPresetPack &pack) const;

  /**
   * Returns the preset index in its pack.
   */
  int get_pset_index (const ChordPreset &pset);

  /**
   * Add a copy of the given preset to the given preset pack and optionally
   * serializes the pack manager.
   *
   * @see ChordPresetPack::add_preset().
   */
  void
  add_preset (ChordPresetPack &pack, const ChordPreset &pset, bool serialize);

  /**
   * Add a copy of the given pack.
   */
  void add_pack (const ChordPresetPack &pack, bool serialize);

  void delete_pack (const ChordPresetPack &pack, bool serialize);

  void delete_preset (const ChordPreset &pset, bool serialize);

  /**
   * Serializes the chord presets.
   *
   * @throw ZrythmException on error.
   */
  void serialize ();

private:
  void                          add_standard_packs ();
  void                          add_user_packs ();
  static std::string            get_user_packs_path ();
  static constexpr const char * UserPacksDirName = "chord-preset-packs";
  static constexpr const char * UserPackJsonFilename = "chord-presets.json";

public:
  /**
   * Scanned preset packs.
   */
  std::vector<std::unique_ptr<ChordPresetPack>> packs_;
};

/**
 * @}
 */

#endif // __SETTINGS_CHORD_PRESET_PACK_MANAGER_H__