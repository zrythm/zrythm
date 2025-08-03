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

  struct CommonFactoryDependencies
  {
    plugins::PluginRegistry                      &plugin_registry_;
    dsp::ProcessorBase::ProcessorBaseDependencies processor_base_dependencies_;
    plugins::Plugin::StateDirectoryParentPathProvider state_dir_path_provider_;
    plugins::JucePlugin::CreatePluginInstanceAsyncFunc
                                    create_plugin_instance_async_func_;
    std::function<sample_rate_t ()> sample_rate_provider_;
    std::function<nframes_t ()>     buffer_size_provider_;
    plugins::JucePlugin::TopLevelWindowProvider top_level_window_provider_;
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

  public:
    auto build ()
    {
      auto obj_ref = [&] () {
        plugins::PluginUuidReference ref =
          dependencies_.plugin_registry_.create_object<PluginT> (
            dependencies_.processor_base_dependencies_,
            dependencies_.state_dir_path_provider_,
            dependencies_.create_plugin_instance_async_func_,
            dependencies_.sample_rate_provider_,
            dependencies_.buffer_size_provider_,
            dependencies_.top_level_window_provider_);
        ref.template get_object_as<PluginT> ()->set_configuration (*setting_);
        return std::move (ref);
      }();

      // auto * obj = std::get<PluginT *> (obj_ref.get_object ());

      return obj_ref;
    }

  private:
    CommonFactoryDependencies              dependencies_;
    OptionalRef<gui::SettingsManager>      settings_manager_;
    OptionalRef<const PluginConfiguration> setting_;
  };

  template <typename PluginT> auto get_builder () const
  {
    auto builder =
      Builder<PluginT> (dependencies_).with_settings_manager (settings_manager_);
    return builder;
  }

public:
  plugins::PluginUuidReference
  create_plugin_from_setting (const PluginConfiguration &setting) const
  {
    auto obj_ref =
      get_builder<plugins::JucePlugin> ().with_setting (setting).build ();
    return obj_ref;
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
