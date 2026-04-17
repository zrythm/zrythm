// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

// clang-format off
// Needs to be included before project.h
#include "utils/format_qt.h"
// clang-format on
#include "structure/project/project.h"
#include "utils/app_settings.h"
#include "utils/io_utils.h"

#include "helpers/mock_hardware_audio_interface_threaded.h"
#include "helpers/mock_settings_backend.h"
#include "helpers/scoped_juce_qapplication.h"

#include <gtest/gtest.h>

namespace zrythm::test_helpers
{

/**
 * @brief Shared test fixture providing a minimal project environment
 * (temp directory, mock hardware, settings, port/param registries, fader,
 * metronome) and a `create_minimal_project()` factory.
 */
class ProjectTestFixture : public ::testing::Test, protected ScopedJuceQApplication
{
protected:
  void SetUp () override
  {
    temp_dir_obj_ = utils::io::make_tmp_dir ();
    project_dir_ =
      utils::Utf8String::from_qstring (temp_dir_obj_->path ()).to_path ();

    hw_interface_ = std::make_unique<ThreadedMockHardwareAudioInterface> ();

    plugin_format_manager_ = std::make_shared<juce::AudioPluginFormatManager> ();
    juce::addDefaultFormatsToManager (*plugin_format_manager_);

    auto mock_backend = std::make_unique<MockSettingsBackend> ();
    mock_backend_ptr_ = mock_backend.get ();

    ON_CALL (*mock_backend_ptr_, value (testing::_, testing::_))
      .WillByDefault (testing::Return (QVariant ()));

    app_settings_ =
      std::make_unique<utils::AppSettings> (std::move (mock_backend));

    port_registry_ = std::make_unique<dsp::PortRegistry> (nullptr);
    param_registry_ = std::make_unique<dsp::ProcessorParameterRegistry> (
      *port_registry_, nullptr);
    monitor_fader_ = utils::make_qobject_unique<dsp::Fader> (
      dsp::ProcessorBase::ProcessorBaseDependencies{
        .port_registry_ = *port_registry_,
        .param_registry_ = *param_registry_,
      },
      dsp::PortType::Audio,
      true,  // hard_limit_output
      false, // make_params_automatable
      [] () -> utils::Utf8String { return u8"Test Control Room"; },
      [] (bool fader_solo_status) { return false; });

    juce::AudioSampleBuffer emphasis_sample (2, 512);
    juce::AudioSampleBuffer normal_sample (2, 512);
    metronome_ = utils::make_qobject_unique<dsp::Metronome> (
      dsp::ProcessorBase::ProcessorBaseDependencies{
        .port_registry_ = *port_registry_,
        .param_registry_ = *param_registry_,
      },
      emphasis_sample, normal_sample, true, 1.0f, nullptr);
  }

  void TearDown () override
  {
    metronome_.reset ();
    monitor_fader_.reset ();
    param_registry_.reset ();
    port_registry_.reset ();
    app_settings_.reset ();
    plugin_format_manager_.reset ();
    hw_interface_.reset ();
  }

  std::unique_ptr<structure::project::Project> create_minimal_project ()
  {
    using namespace zrythm::structure::project;
    using namespace zrythm::plugins;

    Project::ProjectDirectoryPathProvider path_provider =
      [this] (bool for_backup) {
        return for_backup ? project_dir_ / "backups" : project_dir_;
      };

    PluginHostWindowFactory window_factory =
      [] (Plugin &) -> std::unique_ptr<IPluginHostWindow> { return nullptr; };

    return std::make_unique<Project> (
      *app_settings_, path_provider, *hw_interface_, plugin_format_manager_,
      window_factory, *metronome_, *monitor_fader_);
  }

  std::unique_ptr<QTemporaryDir>                   temp_dir_obj_;
  std::filesystem::path                            project_dir_;
  std::unique_ptr<dsp::IHardwareAudioInterface>    hw_interface_;
  std::shared_ptr<juce::AudioPluginFormatManager>  plugin_format_manager_;
  MockSettingsBackend *                            mock_backend_ptr_{};
  std::unique_ptr<utils::AppSettings>              app_settings_;
  std::unique_ptr<dsp::PortRegistry>               port_registry_;
  std::unique_ptr<dsp::ProcessorParameterRegistry> param_registry_;
  utils::QObjectUniquePtr<dsp::Fader>              monitor_fader_;
  utils::QObjectUniquePtr<dsp::Metronome>          metronome_;
};

} // namespace zrythm::test_helpers
