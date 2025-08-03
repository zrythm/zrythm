// SPDX-FileCopyrightText: © 2019-2020, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "dsp/modulator_macro_processor.h"
#include "gui/dsp/plugin_span.h"
#include "structure/tracks/processable_track.h"

#define P_MODULATOR_TRACK (TRACKLIST->modulator_track_)

namespace zrythm::structure::tracks
{
/**
 * @brief A track that can host modulator plugins.
 *
 * FIXME: why is this a ProcessableTrack? It doesn't use the processor...
 */
class ModulatorTrack final
    : public QObject,
      public ProcessableTrack,
      public utils::InitializableObject<ModulatorTrack>
{
  Q_OBJECT
  QML_ELEMENT
  DEFINE_TRACK_QML_PROPERTIES (ModulatorTrack)
  DEFINE_PROCESSABLE_TRACK_QML_PROPERTIES (ModulatorTrack)

  friend class InitializableObject;
  friend struct ModulatorImportData;

  DECLARE_FINAL_TRACK_CONSTRUCTORS (ModulatorTrack)

public:
  /**
   * Inserts and connects a Modulator to the Track.
   *
   * @param replace_mode Whether to perform the operation in replace mode
   * (replace current modulator if true, not touching other modulators, or push
   * other modulators forward if false).
   */
  PluginPtrVariant insert_modulator (
    plugins::PluginSlot::SlotNo  slot,
    plugins::PluginUuidReference modulator_id,
    bool                         replace_mode,
    bool                         confirm,
    bool                         gen_automatables,
    bool                         recalc_graph,
    bool                         pub_events);

  std::optional<PluginPtrVariant>
  get_modulator (plugins::PluginSlot::SlotNo slot) const;

  plugins::PluginSlot get_plugin_slot (const PluginUuid &plugin_id) const;

  /**
   * Returns the plugin at the given slot, if any.
   */
  std::optional<PluginPtrVariant> get_plugin_at_slot (plugins::PluginSlot slot)
  {
    if (
      slot.is_modulator ()
      && slot.get_slot_with_index ().second < (int) modulators_.size ())
      {
        return modulators_[slot.get_slot_with_index ().second].get_object ();
      }
    return std::nullopt;
  }

  void init_loaded (
    PluginRegistry                  &plugin_registry,
    dsp::PortRegistry               &port_registry,
    dsp::ProcessorParameterRegistry &param_registry) override;

  friend void init_from (
    ModulatorTrack        &obj,
    const ModulatorTrack  &other,
    utils::ObjectCloneType clone_type);

  auto get_modulator_macro_processors () const
  {
    return std::span (modulator_macro_processors_);
  }

  auto get_modulator_span () const { return PluginSpan{ modulators_ }; }

  void temporary_virtual_method_hack () const override { }

private:
  static constexpr auto kModulatorsKey = "modulators"sv;
  static constexpr auto kModulatorMacroProcessorsKey = "modulatorMacros"sv;
  friend void           to_json (nlohmann::json &j, const ModulatorTrack &track)
  {
    to_json (j, static_cast<const Track &> (track));
    j[kModulatorsKey] = track.modulators_;
    j[kModulatorMacroProcessorsKey] = track.modulator_macro_processors_;
  }
  friend void from_json (const nlohmann::json &j, ModulatorTrack &track);

  bool initialize ();

public:
  /** Modulators. */
  std::vector<plugins::PluginUuidReference> modulators_;

  /** Modulator macros. */
  std::vector<utils::QObjectUniquePtr<dsp::ModulatorMacroProcessor>>
    modulator_macro_processors_;
};
}
