// SPDX-FileCopyrightText: © 2019-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <utility>

#include "engine/session/graph_dispatcher.h"
#include "gui/backend/backend/project.h"
#include "structure/tracks/automation_track.h"
#include "structure/tracks/modulator_track.h"
#include "structure/tracks/track.h"
#include "utils/logger.h"

namespace zrythm::structure::tracks
{
ModulatorTrack::ModulatorTrack (FinalTrackDependencies dependencies)
    : Track (
        Track::Type::Modulator,
        PortType::Unknown,
        PortType::Unknown,
        dependencies.to_base_dependencies ()),
      ProcessableTrack (
        dependencies.transport_,
        PortType::Unknown,
        Dependencies{
          dependencies.tempo_map_, dependencies.file_audio_source_registry_,
          dependencies.port_registry_, dependencies.param_registry_,
          dependencies.obj_registry_ }),
      modulators_ (
        utils::make_qobject_unique<
          plugins::PluginList> (dependencies.plugin_registry_, this))
{
  main_height_ = DEF_HEIGHT / 2;

  color_ = Color (QColor ("#222222"));
  icon_name_ = u8"gnome-icon-library-encoder-knob-symbolic";

  /* set invisible */
  visible_ = false;

  automationTracklist ()->setParent (this);
}

bool
ModulatorTrack::initialize ()
{
  constexpr int max_macros = 8;
  for (int i = 0; i < max_macros; i++)
    {
      modulator_macro_processors_.emplace_back (
        utils::make_qobject_unique<dsp::ModulatorMacroProcessor> (
          dsp::ModulatorMacroProcessor::ProcessorBaseDependencies{
            .port_registry_ = base_dependencies_.port_registry_,
            .param_registry_ = base_dependencies_.param_registry_ },
          i, this));
    }

  generate_automation_tracks (*this);

  return true;
}

void
init_from (
  ModulatorTrack        &obj,
  const ModulatorTrack  &other,
  utils::ObjectCloneType clone_type)
{
  init_from (
    static_cast<ProcessableTrack &> (obj),
    static_cast<const ProcessableTrack &> (other), clone_type);
  init_from (
    static_cast<Track &> (obj), static_cast<const Track &> (other), clone_type);
// TODO
#if 0
    if (clone_type == utils::ObjectCloneType::NewIdentity)
    {
      const auto clone_from_registry = [] (auto &vec, const auto &other_vec) {
        for (const auto &other_el : other_vec)
          {
            vec.push_back (other_el.clone_new_identity ());
          }
      };

      clone_from_registry (obj.modulators_, other.modulators_);
    }
  else if (clone_type == utils::ObjectCloneType::Snapshot)
    {
      obj.modulators_ = other.modulators_;
    }
#endif
  // obj.modulators_ = other.modulators_;
  utils::clone_unique_ptr_container (
    obj.modulator_macro_processors_, other.modulator_macro_processors_);
}

void
from_json (const nlohmann::json &j, ModulatorTrack &track)
{
  from_json (j, static_cast<Track &> (track));
  from_json (j, static_cast<ProcessableTrack &> (track));
  j.at (ModulatorTrack::kModulatorsKey).get_to (*track.modulators_);
  for (
    const auto &[index, macro_proc_json] : utils::views::enumerate (
      j.at (ModulatorTrack::kModulatorMacroProcessorsKey)))
    {
      auto macro_proc = utils::make_qobject_unique<dsp::ModulatorMacroProcessor> (
        dsp::ModulatorMacroProcessor::ProcessorBaseDependencies{
          .port_registry_ = track.base_dependencies_.port_registry_,
          .param_registry_ = track.base_dependencies_.param_registry_ },
        index, &track);
      from_json (macro_proc_json, *macro_proc);
      track.modulator_macro_processors_.push_back (std::move (macro_proc));
    }
}
}
