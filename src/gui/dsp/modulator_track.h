// SPDX-FileCopyrightText: © 2019-2020, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "gui/dsp/automatable_track.h"
#include "gui/dsp/carla_native_plugin.h"
#include "gui/dsp/modulator_macro_processor.h"
#include "gui/dsp/plugin_span.h"
#include "gui/dsp/processable_track.h"

/**
 * @addtogroup dsp
 *
 * @{
 */

#define P_MODULATOR_TRACK (TRACKLIST->modulator_track_)

/**
 * @brief A track that can host modulator plugins.
 */
class ModulatorTrack final
    : public QObject,
      public ProcessableTrack,
      public ICloneable<ModulatorTrack>,
      public utils::InitializableObject<ModulatorTrack>
{
  Q_OBJECT
  QML_ELEMENT
  DEFINE_TRACK_QML_PROPERTIES (ModulatorTrack)
  DEFINE_AUTOMATABLE_TRACK_QML_PROPERTIES (ModulatorTrack)

  friend class InitializableObject;
  friend struct ModulatorImportData;

  DECLARE_FINAL_TRACK_CONSTRUCTORS (ModulatorTrack)

public:
  using PluginBase = gui::old_dsp::plugins::Plugin;
  using PluginPtrVariant = gui::old_dsp::plugins::PluginPtrVariant;
  using PluginRegistry = gui::old_dsp::plugins::PluginRegistry;

  /**
   * Inserts and connects a Modulator to the Track.
   *
   * @param replace_mode Whether to perform the operation in replace mode
   * (replace current modulator if true, not touching other modulators, or push
   * other modulators forward if false).
   */
  PluginPtrVariant insert_modulator (
    plugins::PluginSlot::SlotNo slot,
    PluginUuidReference         modulator_id,
    bool                        replace_mode,
    bool                        confirm,
    bool                        gen_automatables,
    bool                        recalc_graph,
    bool                        pub_events);

  /**
   * Removes the modulator (plugin) at the given slot.
   */
  PluginPtrVariant remove_modulator (plugins::PluginSlot::SlotNo slot);

  std::optional<PluginPtrVariant>
  get_modulator (plugins::PluginSlot::SlotNo slot) const;

  plugins::PluginSlot get_plugin_slot (const PluginUuid &plugin_id) const;

  void
  init_loaded (PluginRegistry &plugin_registry, PortRegistry &port_registry)
    override;

  void
  init_after_cloning (const ModulatorTrack &other, ObjectCloneType clone_type)
    override;

  bool validate () const override;

  void
  append_ports (std::vector<Port *> &ports, bool include_plugins) const final;

  auto get_modulator_macro_processors () const
  {
    return std::span (modulator_macro_processors_);
  }

  auto get_modulator_span () const { return PluginSpan{ modulators_ }; }

private:
  static constexpr auto kModulatorsKey = "modulators"sv;
  static constexpr auto kModulatorMacroProcessorsKey = "modulatorMacros"sv;
  friend void           to_json (nlohmann::json &j, const ModulatorTrack &track)
  {
    to_json (j, static_cast<const Track &> (track));
    to_json (j, static_cast<const ProcessableTrack &> (track));
    to_json (j, static_cast<const AutomatableTrack &> (track));
    j[kModulatorsKey] = track.modulators_;
    j[kModulatorMacroProcessorsKey] = track.modulator_macro_processors_;
  }
  friend void from_json (const nlohmann::json &j, ModulatorTrack &track);

  bool initialize ();

private:
  /** Modulators. */
  std::vector<PluginUuidReference> modulators_;

  /** Modulator macros. */
  std::vector<std::unique_ptr<ModulatorMacroProcessor>>
    modulator_macro_processors_;
};

/**
 * @}
 */
