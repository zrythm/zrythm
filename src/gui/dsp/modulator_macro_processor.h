// SPDX-FileCopyrightText: © 2021-2022, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "gui/dsp/control_port.h"
#include "gui/dsp/cv_port.h"
#include "utils/types.h"

namespace zrythm::structure::tracks
{
class ModulatorTrack;
}

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
class ModulatorMacroProcessor final : public dsp::graph::IProcessable, public IPortOwner
{
  using ModulatorTrack = structure::tracks::ModulatorTrack;

public:
  ModulatorMacroProcessor (
    PortRegistry      &port_registry,
    ModulatorTrack *   track,
    std::optional<int> idx,
    bool               new_identity);

  void set_port_metadata_from_owner (dsp::PortIdentifier &id, PortRange &range)
    const override;

  utils::Utf8String
  get_full_designation_for_port (const dsp::PortIdentifier &id) const override;

  auto get_name () const { return name_; }

  [[gnu::cold]] void init_loaded (ModulatorTrack &track);

  void set_name (const utils::Utf8String &name) { name_ = name; }

  ModulatorTrack * get_track () const { return track_; }

  utils::Utf8String get_node_name () const override
  {
    return utils::Utf8String::from_utf8_encoded_string (
      fmt::format ("{} Modulator Macro Processor", name_));
    ;
  }

  void process_block (EngineProcessTimeInfo time_nfo) override;

  friend void init_from (
    ModulatorMacroProcessor       &obj,
    const ModulatorMacroProcessor &other,
    utils::ObjectCloneType         clone_type);

  CVPort &get_cv_in_port ()
  {
    return *std::get<CVPort *> (cv_in_id_->get_object ());
  }

  CVPort &get_cv_out_port ()
  {
    return *std::get<CVPort *> (cv_out_id_->get_object ());
  }

  ControlPort &get_macro_port ()
  {
    return *std::get<ControlPort *> (macro_id_->get_object ());
  }

private:
  static constexpr auto kNameKey = "name"sv;
  static constexpr auto kCVInKey = "cvIn"sv;
  static constexpr auto kCVOutKey = "cvOut"sv;
  static constexpr auto kMacroKey = "macro"sv;
  friend void to_json (nlohmann::json &j, const ModulatorMacroProcessor &p)
  {
    j = nlohmann::json{
      { kNameKey,  p.name_      },
      { kCVInKey,  p.cv_in_id_  },
      { kCVOutKey, p.cv_out_id_ },
      { kMacroKey, p.macro_id_  },
    };
  }
  friend void from_json (const nlohmann::json &j, ModulatorMacroProcessor &p)
  {
    j.at (kNameKey).get_to (p.name_);
    p.cv_in_id_.emplace (p.port_registry_);
    j.at (kCVInKey).get_to (*p.cv_in_id_);
    p.cv_out_id_.emplace (p.port_registry_);
    j.at (kCVOutKey).get_to (*p.cv_out_id_);
    p.macro_id_.emplace (p.port_registry_);
    j.at (kMacroKey).get_to (*p.macro_id_);
  }

private:
  PortRegistry &port_registry_;

  /**
   * Name to be shown in the modulators tab.
   *
   * @note This is only cosmetic and should not be used anywhere during
   * processing.
   */
  utils::Utf8String name_;

  /** CV input port for connecting CV signals to. */
  std::optional<PortUuidReference> cv_in_id_;

  /**
   * CV output after macro is applied.
   *
   * This can be routed to other parameters to apply the macro.
   */
  std::optional<PortUuidReference> cv_out_id_;

  /** Control port controlling the amount. */
  std::optional<PortUuidReference> macro_id_;

  /** Pointer to owner track, if any. */
  ModulatorTrack * track_{};
};

/**
 * @}
 */
