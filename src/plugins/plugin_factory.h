// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <utility>

#include "plugins/plugin_all.h"
#include "utils/optional_ref.h"
#include "utils/registry_utils.h"

namespace zrythm::plugins
{

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

  struct CommonFactoryDependencies
  {
    utils::IObjectRegistry &registry;
    plugins::JucePlugin::CreatePluginInstanceAsyncFunc
                                           create_plugin_instance_async_func_;
    std::function<units::sample_rate_t ()> sample_rate_provider_;
    std::function<units::sample_u32_t ()>  buffer_size_provider_;
    plugins::PluginHostWindowFactory       top_level_window_provider_;
  };

  PluginFactory () = delete;
  PluginFactory (
    CommonFactoryDependencies dependencies,
    QObject *                 parent = nullptr)
      : QObject (parent), dependencies_ (std::move (dependencies))
  {
  }

private:
  template <typename PluginT> class Builder
  {
    friend class PluginFactory;

  private:
    explicit Builder (CommonFactoryDependencies dependencies)
        : dependencies_ (std::move (dependencies))
    {
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
            return utils::create_object<PluginT> (
              dependencies_.registry, dependencies_.registry,
              dependencies_.top_level_window_provider_);
          }
        else if constexpr (
          std::derived_from<PluginT, plugins::InternalPluginBase>)
          {
            return utils::create_object<PluginT> (
              dependencies_.registry, dependencies_.registry);
          }
        else
          {
            return utils::create_object<PluginT> (
              dependencies_.registry, dependencies_.registry,
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
          const auto instantiation_finish_opts =
            instantiation_finish_options_.value ();
          auto * plugin = obj_ref.get ();
          QObject::connect (
            plugin, &zrythm::plugins::Plugin::instantiationFinished,
            instantiation_finish_opts.handler_context_,
            [obj_ref, instantiation_finish_opts] () {
              instantiation_finish_opts.handler_ (obj_ref);
            });
        }

      if (setting_.has_value ())
        {
          obj_ref.template get_object_as<PluginT> ()->set_configuration (
            *setting_);
        }
      else
        {
          throw std::logic_error ("PluginConfiguration required");
        }

      return obj_ref;
    }

  private:
    CommonFactoryDependencies                     dependencies_;
    utils::OptionalRef<const PluginConfiguration> setting_;
    std::optional<InstantiationFinishOptions>     instantiation_finish_options_;
  };

  template <typename PluginT> auto get_builder () const
  {
    auto builder = Builder<PluginT> (dependencies_);
    return builder;
  }

public:
  template <typename PluginT>
  std::unique_ptr<PluginT> build_for_deserialization () const
  {
    if constexpr (std::is_same_v<PluginT, plugins::ClapPlugin>)
      {
        return std::make_unique<PluginT> (
          dependencies_.registry, dependencies_.top_level_window_provider_);
      }
    else if constexpr (std::derived_from<PluginT, plugins::InternalPluginBase>)
      {
        return std::make_unique<PluginT> (dependencies_.registry);
      }
    else
      {
        return std::make_unique<PluginT> (
          dependencies_.registry,
          dependencies_.create_plugin_instance_async_func_,
          dependencies_.sample_rate_provider_,
          dependencies_.buffer_size_provider_,
          dependencies_.top_level_window_provider_);
      }
  }

public:
  plugins::PluginUuidReference create_plugin_from_setting (
    const PluginConfiguration &setting,
    InstantiationFinishOptions instantiation_finish_options) const
  {
    const auto * descriptor = setting.descriptor ();
    if (descriptor == nullptr)
      {
        throw std::invalid_argument ("Setting with valid descriptor required");
      }

    const auto protocol = descriptor->protocol_;
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
};
}
