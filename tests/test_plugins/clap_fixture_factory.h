// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * @file clap_fixture_factory.h
 *
 * Shared scaffolding for CLAP test fixtures: plugin base alias,
 * instantiation, the plugin factory and the module entry point.
 *
 * A fixture provides one or more plugin classes (derived from
 * ClapFixturePluginBase, constructible from `const clap_host *`) with a
 * `static const clap_plugin_descriptor * descriptor ()` member, then
 * exports the entry point:
 *
 * @code
 * extern "C" {
 * CLAP_EXPORT const clap_plugin_entry clap_entry =
 *   zrythm_test_plugins::clap_fixture_entry<MyPlugin>;
 * }
 * @endcode
 */

#pragma once

#include <array>
#include <cstring>

#include <clap/all.h>
#include <clap/helpers/plugin.hh>
#include <clap/helpers/plugin.hxx>

namespace zrythm_test_plugins
{

using ClapFixturePluginBase = clap::helpers::Plugin<
  clap::helpers::MisbehaviourHandler::Terminate,
  clap::helpers::CheckingLevel::Maximal>;

template <typename... Plugins>
const clap_plugin_descriptor *
clap_fixture_descriptor (const clap_plugin_factory *, uint32_t index)
{
  const std::array descriptors = { Plugins::descriptor ()... };
  return index < descriptors.size () ? descriptors[index] : nullptr;
}

template <typename... Plugins>
const clap_plugin *
clap_fixture_create (
  const clap_plugin_factory *,
  const clap_host * host,
  const char *      plugin_id)
{
  if (host == nullptr || !clap_version_is_compatible (host->clap_version))
    return nullptr;
  const clap_plugin * plugin = nullptr;
  // The plugin == nullptr guard stops the fold after the first match: two
  // fixture types sharing an id would otherwise both be created and one
  // would leak
  ((plugin == nullptr && std::strcmp (plugin_id, Plugins::descriptor ()->id) == 0
      ? (plugin = (new Plugins (host))->clapPlugin ())
      : nullptr),
   ...);
  return plugin;
}

template <typename... Plugins>
const clap_plugin_factory clap_fixture_factory = {
  .get_plugin_count = [] (const clap_plugin_factory *) -> uint32_t {
    return static_cast<uint32_t> (sizeof...(Plugins));
  },
  .get_plugin_descriptor = &clap_fixture_descriptor<Plugins...>,
  .create_plugin = &clap_fixture_create<Plugins...>,
};

template <typename... Plugins>
const clap_plugin_entry clap_fixture_entry = {
  .clap_version = CLAP_VERSION,
  .init = [] (const char *) -> bool { return true; },
  .deinit = [] () { },
  .get_factory = [] (const char * factory_id) -> const void * {
    return std::strcmp (factory_id, CLAP_PLUGIN_FACTORY_ID) == 0
             ? static_cast<const void *> (&clap_fixture_factory<Plugins...>)
             : nullptr;
  },
};

} // namespace zrythm_test_plugins
