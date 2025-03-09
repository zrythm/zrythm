// SPDX-FileCopyrightText: © 2021-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __AUDIO_MODULATOR_MACRO_PROCESSOR_H__
#define __AUDIO_MODULATOR_MACRO_PROCESSOR_H__

#include "gui/dsp/control_port.h"
#include "gui/dsp/cv_port.h"

#include "utils/types.h"

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
      public dsp::IProcessable,
      public utils::serialization::ISerializable<ModulatorMacroProcessor>,
      public IPortOwner
{
public:
  ModulatorMacroProcessor (const DeserializationDependencyHolder &dh);
  ModulatorMacroProcessor (
    PortRegistry      &port_registry,
    ModulatorTrack *   track,
    std::optional<int> idx,
    bool               new_identity);

  bool is_in_active_project () const override;

  void set_port_metadata_from_owner (dsp::PortIdentifier &id, PortRange &range)
    const override;

  std::string
  get_full_designation_for_port (const dsp::PortIdentifier &id) const override;

  std::string get_name () const { return name_; }

  ATTR_COLD void init_loaded (ModulatorTrack &track);

  void set_name (std::string_view name) { name_ = name; }

  ModulatorTrack * get_track () const { return track_; }

  std::string get_node_name () const override
  {
    return fmt::format ("{} Modulator Macro Processor", name_);
  }

  void process_block (EngineProcessTimeInfo time_nfo) override;

  void init_after_cloning (
    const ModulatorMacroProcessor &other,
    ObjectCloneType                clone_type) override;

  CVPort &get_cv_in_port ()
  {
    return *std::get<CVPort *> (port_registry_.find_by_id_or_throw (cv_in_id_));
  }

  CVPort &get_cv_out_port ()
  {
    return *std::get<CVPort *> (port_registry_.find_by_id_or_throw (cv_out_id_));
  }

  ControlPort &get_macro_port ()
  {
    return *std::get<ControlPort *> (
      port_registry_.find_by_id_or_throw (macro_id_));
  }

  DECLARE_DEFINE_FIELDS_METHOD ();

private:
  /**
   * Name to be shown in the modulators tab.
   *
   * @note This is only cosmetic and should not be used anywhere during
   * processing.
   */
  std::string name_;

  /** CV input port for connecting CV signals to. */
  PortUuid cv_in_id_;

  /**
   * CV output after macro is applied.
   *
   * This can be routed to other parameters to apply the macro.
   */
  PortUuid cv_out_id_;

  /** Control port controlling the amount. */
  PortUuid macro_id_;

  /** Pointer to owner track, if any. */
  ModulatorTrack * track_{};

  PortRegistry &port_registry_;
};

/**
 * @}
 */

#endif
