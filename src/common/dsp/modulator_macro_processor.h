// SPDX-FileCopyrightText: © 2021-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __AUDIO_MODULATOR_MACRO_PROCESSOR_H__
#define __AUDIO_MODULATOR_MACRO_PROCESSOR_H__

#include "common/dsp/control_port.h"
#include "common/dsp/cv_port.h"
#include "common/utils/types.h"

class ModulatorTrack;

/**
 * @addtogroup dsp
 *
 * @{
 */

/**
 * Modulator macro button processor.
 *
 * Has 1 control input, many CV inputs and 1 CV output.
 *
 * Can only belong to modulator track.
 */
class ModulatorMacroProcessor final
    : public ICloneable<ModulatorMacroProcessor>,
      public ISerializable<ModulatorMacroProcessor>
{
public:
  ModulatorMacroProcessor () = default;
  ModulatorMacroProcessor (ModulatorTrack * track, int idx);

  bool is_in_active_project () const;

  std::string get_name () const { return name_; }

  ATTR_COLD void init_loaded (ModulatorTrack &track);

  void set_name (std::string_view name) { name_ = name; }

  ModulatorTrack * get_track () const { return track_; }

  /**
   * Process.
   */
  void process (const EngineProcessTimeInfo time_nfo);

  void init_after_cloning (const ModulatorMacroProcessor &other) override;

  DECLARE_DEFINE_FIELDS_METHOD ();

public:
  /**
   * Name to be shown in the modulators tab.
   *
   * @note This is only cosmetic and should not be used anywhere during
   * processing.
   */
  std::string name_;

  /** CV input port for connecting CV signals to. */
  std::unique_ptr<CVPort> cv_in_;

  /**
   * CV output after macro is applied.
   *
   * This can be routed to other parameters to apply the macro.
   */
  std::unique_ptr<CVPort> cv_out_;

  /** Control port controlling the amount. */
  std::unique_ptr<ControlPort> macro_;

  /** Pointer to owner track, if any. */
  ModulatorTrack * track_;
};

/**
 * @}
 */

#endif
