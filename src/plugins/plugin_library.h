// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <memory>
#include <string_view>

#include "utils/utf8_string.h"

namespace zrythm::plugins
{

/**
 * @brief Platform-independent wrapper for loading plugin shared libraries.
 *
 * Encapsulates the platform-specific details of loading plugin binaries
 * (QLibrary on Linux/Windows, with macOS bundle resolution).
 *
 * TODO: Replace the internal QLibrary implementation with CFBundle on
 * macOS (like JUCE does for VST3) for proper bundle handling without
 * manual path resolution.
 */
class PluginLibrary
{
public:
  /// Function pointer type returned when resolving library symbols.
  using FunctionSymbol = void (*) ();
  PluginLibrary ();
  ~PluginLibrary ();

  PluginLibrary (const PluginLibrary &) = delete;
  PluginLibrary &operator= (const PluginLibrary &) = delete;
  PluginLibrary (PluginLibrary &&other) noexcept;
  PluginLibrary &operator= (PluginLibrary &&other) noexcept;

  /**
   * @brief Loads the plugin library at the given path.
   *
   * On macOS, if the path is a directory bundle (e.g., .clap/, .vst3/),
   * resolves to the binary inside Contents/MacOS/ before loading.
   */
  [[nodiscard]] bool load (const utils::Utf8String &path);

  /**
   * @brief Resolves a symbol from the loaded library.
   */
  FunctionSymbol resolve (std::string_view symbol_name);

  /**
   * @brief Unloads the library.
   */
  void unload ();

  /**
   * @brief Returns true if the library is currently loaded.
   */
  [[nodiscard]] bool is_loaded () const;

  /**
   * @brief Returns a human-readable description of the last error.
   */
  [[nodiscard]] utils::Utf8String error_string () const;

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

} // namespace zrythm::plugins
