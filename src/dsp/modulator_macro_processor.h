// SPDX-FileCopyrightText: © 2021-2022, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "dsp/graph_node.h"
#include "dsp/parameter.h"
#include "dsp/port_fwd.h"
#include "dsp/processor_base.h"
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
class ModulatorMacroProcessor final : public QObject, public dsp::ProcessorBase
{
  Q_OBJECT
  Q_PROPERTY (QString name READ name CONSTANT)
  QML_ELEMENT
  QML_UNCREATABLE ("")

public:
  ModulatorMacroProcessor (
    ProcessorBaseDependencies dependencies,
    int                       idx,
    QObject *                 parent = nullptr);

  // ========================================================================
  // QML Interface
  // ========================================================================

  QString name () const { return name_.to_qstring (); }

  // ========================================================================

  utils::Utf8String get_full_designation_for_port (const dsp::Port &port) const;

  auto get_name () const { return name_; }

  // void set_name (const utils::Utf8String &name) { name_ = name; }

  void custom_process_block (EngineProcessTimeInfo time_nfo) noexcept override;

  friend void init_from (
    ModulatorMacroProcessor       &obj,
    const ModulatorMacroProcessor &other,
    utils::ObjectCloneType         clone_type);

  /** CV input port for connecting CV signals to. */
  auto &get_cv_in_port ()
  {
    return *get_input_ports ().front ().get_object_as<dsp::CVPort> ();
  }

  /**
   * CV output after macro is applied.
   *
   * This can be routed to other parameters to apply the macro.
   */
  auto &get_cv_out_port ()
  {
    return *get_output_ports ().front ().get_object_as<dsp::CVPort> ();
  }

  /** Control port controlling the amount. */
  auto &get_macro_param ()
  {
    return *get_parameters ().front ().get_object_as<dsp::ProcessorParameter> ();
  }

private:
  static constexpr auto kNameKey = "name"sv;
  friend void to_json (nlohmann::json &j, const ModulatorMacroProcessor &p)
  {
    to_json (j, static_cast<const dsp::ProcessorBase &> (p));
    j[kNameKey] = p.name_;
  }
  friend void from_json (const nlohmann::json &j, ModulatorMacroProcessor &p)
  {
    from_json (j, static_cast<dsp::ProcessorBase &> (p));
    j.at (kNameKey).get_to (p.name_);
  }

private:
  ProcessorBaseDependencies dependencies_;

  /**
   * Name to be shown in the modulators tab.
   *
   * @note This is only cosmetic and should not be used anywhere during
   * processing.
   */
  utils::Utf8String name_;

  BOOST_DESCRIBE_CLASS (ModulatorMacroProcessor, (), (), (), (name_))
};

} // namespace zrythm::dsp
