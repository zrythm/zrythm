// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "plugins/plugin_library.h"

#include <QLibrary>
#include <QtSystemDetection>

#ifdef Q_OS_MACOS
#  include <juce_core/juce_core.h>
#endif

namespace zrythm::plugins
{

struct PluginLibrary::Impl
{
  QLibrary library;
};

PluginLibrary::PluginLibrary () : impl_ (std::make_unique<Impl> ()) { }
PluginLibrary::~PluginLibrary () = default;

PluginLibrary::PluginLibrary (PluginLibrary &&other) noexcept
    : impl_ (std::move (other.impl_))
{
}

PluginLibrary &
PluginLibrary::operator= (PluginLibrary &&other) noexcept
{
  if (this != &other)
    impl_ = std::move (other.impl_);
  return *this;
}

bool
PluginLibrary::load (const utils::Utf8String &path)
{
  utils::Utf8String load_path = path;

#ifdef Q_OS_MACOS
  // On macOS, plugins are directory bundles (e.g., .clap/, .vst3/).
  // QLibrary uses dlopen() which cannot load directories, so resolve
  // to the actual binary inside the bundle at Contents/MacOS/.
  const auto file = path.to_juce_file ();
  if (file.isDirectory ())
    {
      const auto macOSDir =
        file.getChildFile ("Contents").getChildFile ("MacOS");
      const auto binaries =
        macOSDir.findChildFiles (juce::File::findFiles, false);
      if (binaries.isEmpty ())
        {
          z_warning (
            "Plugin bundle '{}' has no binary in Contents/MacOS/", path);
          return false;
        }
      load_path = utils::Utf8String::from_juce_string (
        binaries.getFirst ().getFullPathName ());
    }
#endif

  impl_->library.setFileName (load_path.to_qstring ());
  impl_->library.setLoadHints (
    QLibrary::ResolveAllSymbolsHint
#if !defined(__has_feature) || !__has_feature(address_sanitizer)
    | QLibrary::DeepBindHint
#endif
  );
  return impl_->library.load ();
}

PluginLibrary::FunctionSymbol
PluginLibrary::resolve (std::string_view symbol_name)
{
  // QLibrary::resolve() takes const char*, so we need a null-terminated
  // string. std::string_view is not guaranteed to be null-terminated.
  const auto name = std::string (symbol_name);
  return impl_->library.resolve (name.c_str ());
}

void
PluginLibrary::unload ()
{
  impl_->library.unload ();
}

bool
PluginLibrary::is_loaded () const
{
  return impl_->library.isLoaded ();
}

utils::Utf8String
PluginLibrary::error_string () const
{
  return utils::Utf8String::from_qstring (impl_->library.errorString ());
}

} // namespace zrythm::plugins
