// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <utility>

#include "gui/backend/backend/settings_manager.h"
#include "plugins/plugin_all.h"
#include "structure/tracks/track_all.h"

/**
 * @brief Factory for plugins.
 *
 * @note API that starts with `add` adds the object to the project and should be
 * used in most cases. API that starts with `create` only creates and registers
 * the object but does not add it to the project (this should only be used
 * internally).
 */
class PluginFactory : public QObject
{
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE ("")

public:
  using PluginConfiguration = zrythm::plugins::PluginConfiguration;

  // A handler that will be called on the main thread either asynchronously or
  // synchronously (depending on the plugin format) when the plugin has finished
  // instantiation
  using InstantiationFinishedHandler =
    std::function<void (plugins::PluginUuidReference)>;

  struct InstantiationFinishOptions
  {
    InstantiationFinishedHandler handler_;
    QObject *                    handler_context_{};
  };

  /**
   * @brief Function that returns whether the caller is an audio DSP thread.
   */
  using AudioThreadChecker = std::function<bool ()>;

  struct CommonFactoryDependencies
  {
    plugins::PluginRegistry                      &plugin_registry_;
    dsp::ProcessorBase::ProcessorBaseDependencies processor_base_dependencies_;
    plugins::Plugin::StateDirectoryParentPathProvider state_dir_path_provider_;
    plugins::JucePlugin::CreatePluginInstanceAsyncFunc
                                    create_plugin_instance_async_func_;
    std::function<sample_rate_t ()> sample_rate_provider_;
    std::function<nframes_t ()>     buffer_size_provider_;
    plugins::JucePlugin::JucePluginTopLevelWindowProvider
                       top_level_window_provider_;
    AudioThreadChecker audio_thread_checker_;
  };

  PluginFactory () = delete;
  PluginFactory (
    CommonFactoryDependencies dependencies,
    gui::SettingsManager     &settings_mgr,
    QObject *                 parent = nullptr)
      : QObject (parent), dependencies_ (std::move (dependencies)),
        settings_manager_ (settings_mgr)
  {
  }

  static PluginFactory * get_instance ();

  template <typename PluginT> class Builder
  {
    friend class PluginFactory;

  private:
    explicit Builder (CommonFactoryDependencies dependencies)
        : dependencies_ (std::move (dependencies))
    {
    }

    Builder &with_settings_manager (gui::SettingsManager &settings_manager)
    {
      settings_manager_ = settings_manager;
      return *this;
    }

    Builder &with_setting (const PluginConfiguration &setting)
    {
      setting_ = setting;
      return *this;
    }

    Builder &with_instantiation_finished_options (
      InstantiationFinishOptions instantiation_finish_options)
    {
      instantiation_finish_options_.emplace (instantiation_finish_options);
      return *this;
    }

  public:
    auto build ()
    {
      auto obj_ref = [&] () {
        if constexpr (std::is_same_v<PluginT, plugins::ClapPlugin>)
          {
            return dependencies_.plugin_registry_.create_object<PluginT> (
              dependencies_.processor_base_dependencies_,
              dependencies_.state_dir_path_provider_,
              dependencies_.audio_thread_checker_);
          }
        else if constexpr (
          std::derived_from<PluginT, plugins::InternalPluginBase>)
          {
            return dependencies_.plugin_registry_.create_object<PluginT> (
              dependencies_.processor_base_dependencies_,
              dependencies_.state_dir_path_provider_);
          }
        else
          {
            return dependencies_.plugin_registry_.create_object<PluginT> (
              dependencies_.processor_base_dependencies_,
              dependencies_.state_dir_path_provider_,
              dependencies_.create_plugin_instance_async_func_,
              dependencies_.sample_rate_provider_,
              dependencies_.buffer_size_provider_,
              dependencies_.top_level_window_provider_);
          }
      }();

      if (instantiation_finish_options_.has_value ())
        {
          // set instantiation finished handler and apply configuration, which
          // will either fire the instantiation finished handler immediately or
          // later (depending on the plugin format)
          std::visit (
            [&] (auto &&pl) {
              const auto instantiation_finish_opts =
                instantiation_finish_options_.value ();
              QObject::connect (
                pl, &zrythm::plugins::Plugin::instantiationFinished,
                instantiation_finish_opts.handler_context_,
                [obj_ref, instantiation_finish_opts] () {
                  instantiation_finish_opts.handler_ (obj_ref);
                });
            },
            obj_ref.get_object ());
        }

      if (setting_.has_value ())
        {
          obj_ref.template get_object_as<PluginT> ()->set_configuration (
            *setting_);
        }

      return obj_ref;
    }

  private:
    CommonFactoryDependencies                 dependencies_;
    OptionalRef<gui::SettingsManager>         settings_manager_;
    OptionalRef<const PluginConfiguration>    setting_;
    std::optional<InstantiationFinishOptions> instantiation_finish_options_;
  };

  template <typename PluginT> auto get_builder () const
  {
    auto builder =
      Builder<PluginT> (dependencies_).with_settings_manager (settings_manager_);
    return builder;
  }

public:
  plugins::PluginUuidReference create_plugin_from_setting (
    const PluginConfiguration &setting,
    InstantiationFinishOptions instantiation_finish_options) const
  {
    const auto protocol = setting.descriptor ()->protocol_;
    if (protocol == plugins::Protocol::ProtocolType::CLAP)
      {
        return get_builder<plugins::ClapPlugin> ()
          .with_setting (setting)
          .with_instantiation_finished_options (instantiation_finish_options)
          .build ();
      }
    if (protocol == plugins::Protocol::ProtocolType::Internal)
      {
        return get_builder<plugins::InternalPluginBase> ()
          .with_setting (setting)
          .with_instantiation_finished_options (instantiation_finish_options)
          .build ();
      }

    return get_builder<plugins::JucePlugin> ()
      .with_setting (setting)
      .with_instantiation_finished_options (instantiation_finish_options)
      .build ();
  }

// TODO
#if 0
  template <typename PluginT>
  auto clone_new_object_identity (const PluginT &other) const
  {
    return plugin_registry_.clone_object (other, plugin_registry_);
  }

  template <typename PluginT>
  auto clone_object_snapshot (const PluginT &other, QObject &owner) const
  {
    PluginT * new_obj{};

    new_obj = utils::clone_qobject (
      other, &owner, utils::ObjectCloneType::Snapshot, plugin_registry_);
    return new_obj;
  }
#endif

private:
  CommonFactoryDependencies dependencies_;
  gui::SettingsManager     &settings_manager_;
};
