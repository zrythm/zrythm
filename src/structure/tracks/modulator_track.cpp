// SPDX-FileCopyrightText: © 2019-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "structure/tracks/modulator_track.h"

namespace zrythm::structure::tracks
{
ModulatorTrack::ModulatorTrack (FinalTrackDependencies dependencies)
    : Track (
        Track::Type::Modulator,
        std::nullopt,
        std::nullopt,
        TrackFeatures::Automation | TrackFeatures::Modulators,
        dependencies.to_base_dependencies ())
{
  main_height_ = DEF_HEIGHT / 2;

  color_ = Color (QColor ("#222222"));
  icon_name_ = u8"gnome-icon-library-encoder-knob-symbolic";

  /* set invisible */
  visible_ = false;

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
}

void
init_from (
  ModulatorTrack        &obj,
  const ModulatorTrack  &other,
  utils::ObjectCloneType clone_type)
{
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
}
}
