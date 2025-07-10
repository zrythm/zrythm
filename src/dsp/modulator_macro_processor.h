// SPDX-FileCopyrightText: © 2021-2022, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "dsp/graph_node.h"
#include "dsp/parameter.h"
#include "dsp/port_fwd.h"
#include "utils/types.h"

namespace zrythm::dsp
{

/**
 * Modulator macro button processor.
 *
 * This class enables users to create macro controls that can scale modulation
 * signals or output fixed values. It contains 1 parameter to control scaling
 * (or set a fixed output CV value), many CV inputs and 1 CV output.
 *
 * Intended to be used in ModulatorTrack.
 */
class ModulatorMacroProcessor final : public QObject, public dsp::graph::IProcessable
{
  Q_OBJECT
  Q_PROPERTY (QString name READ name CONSTANT)
  QML_ELEMENT

public:
  ModulatorMacroProcessor (
    dsp::PortRegistry               &port_registry,
    dsp::ProcessorParameterRegistry &param_registry,
    int                              idx,
    QObject *                        parent = nullptr);

  // ========================================================================
  // QML Interface
  // ========================================================================

  QString name () const { return name_.to_qstring (); }

  // ========================================================================

  utils::Utf8String get_full_designation_for_port (const dsp::Port &port) const;

  auto get_name () const { return name_; }

  // void set_name (const utils::Utf8String &name) { name_ = name; }

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

  auto &get_cv_in_port () { return *cv_in_id_.get_object_as<dsp::CVPort> (); }

  auto &get_cv_out_port () { return *cv_out_id_.get_object_as<dsp::CVPort> (); }

  auto &get_macro_param ()
  {
    return *macro_id_.get_object_as<dsp::ProcessorParameter> ();
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
    j.at (kCVInKey).get_to (p.cv_in_id_);
    j.at (kCVOutKey).get_to (p.cv_out_id_);
    j.at (kMacroKey).get_to (p.macro_id_);
  }

private:
  dsp::PortRegistry               &port_registry_;
  dsp::ProcessorParameterRegistry &param_registry_;

  /**
   * Name to be shown in the modulators tab.
   *
   * @note This is only cosmetic and should not be used anywhere during
   * processing.
   */
  utils::Utf8String name_;

  /** CV input port for connecting CV signals to. */
  dsp::PortUuidReference cv_in_id_;

  /**
   * CV output after macro is applied.
   *
   * This can be routed to other parameters to apply the macro.
   */
  dsp::PortUuidReference cv_out_id_;

  /** Control port controlling the amount. */
  dsp::ProcessorParameterUuidReference macro_id_;

  BOOST_DESCRIBE_CLASS (
    ModulatorMacroProcessor,
    (),
    (),
    (),
    (name_, cv_in_id_, cv_out_id_, macro_id_))
};

} // namespace zrythm::dsp
