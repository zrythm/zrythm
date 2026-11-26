// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <tuple>

#include <fmt/std.h>

#include "utils/exceptions.h"
#include "utils/user_directories.h"

#include <QCoreApplication>
#include <QFile>
#include <QSettings>
#include <QStandardPaths>

#include <fmt/format.h>

namespace zrythm::utils
{

namespace
{

class QSettingsBackend : public ISettingsBackend
{
public:
  explicit QSettingsBackend (std::unique_ptr<QSettings> settings)
      : settings_ (std::move (settings))
  {
  }

  QVariant
  value (QAnyStringView key, const QVariant &defaultValue = {}) const override
  {
    return settings_->value (key, defaultValue);
  }

  void setValue (QAnyStringView key, const QVariant &value) override
  {
    settings_->setValue (key, value);
    settings_->sync ();
  }

private:
  std::unique_ptr<QSettings> settings_;
};

/**
 * Checks that the application has a name.
 *
 * @throw ZrythmException When the application has neither an application
 * name nor an organization name. An application without them gets a
 * directory that other applications share.
 */
void
require_named_application ()
{
  if (
    QCoreApplication::applicationName ().isEmpty ()
    && QCoreApplication::organizationName ().isEmpty ())
    {
      throw ZrythmException (
        "The application has no name: set an application name or an "
        "organization name before a location of the operating system is "
        "requested");
    }
}

/**
 * Returns a location of the operating system for this application.
 *
 * @throw ZrythmException When the application has neither an application
 * name nor an organization name, or when the operating system has no such
 * location.
 */
QString
application_location (QStandardPaths::StandardLocation type)
{
  require_named_application ();
  const auto location = QStandardPaths::writableLocation (type);
  // Qt returns an empty string when it cannot determine the location
  if (location.isEmpty ())
    {
      throw ZrythmException (
        fmt::format (
          "The operating system has no {} location for this application",
          Utf8String::from_qstring (QStandardPaths::displayName (type)).view ()));
    }
  return location;
}

/** Points the disk caches of Qt Quick to @p cache_dir. */
void
redirect_qt_caches (const std::filesystem::path &cache_dir)
{
  qputenv (
    "QML_DISK_CACHE_PATH",
    Utf8String::from_path (cache_dir / u8"qmlcache").to_qstring ().toUtf8 ());

  // Qt loads its own pipeline cache file when no file to load is named,
  // and warns when a named file is missing
  qputenv ("QSG_RHI_DISABLE_DISK_CACHE", "1");
  const auto pipeline_cache =
    Utf8String::from_path (cache_dir / u8"qtpipelinecache").to_qstring ();
  qputenv ("QSG_RHI_PIPELINE_CACHE_SAVE", pipeline_cache.toUtf8 ());
  if (QFile::exists (pipeline_cache))
    {
      qputenv ("QSG_RHI_PIPELINE_CACHE_LOAD", pipeline_cache.toUtf8 ());
    }
  else
    {
      qunsetenv ("QSG_RHI_PIPELINE_CACHE_LOAD");
    }
}

} // namespace

std::expected<UserDirectories, Utf8String>
UserDirectories::make_private (const std::filesystem::path &directory)
{
  if (directory.empty ())
    {
      return std::unexpected (Utf8String (u8"No directory was given"));
    }

  std::error_code ec;
  const auto      root =
    std::filesystem::absolute (directory, ec).lexically_normal ();
  if (ec)
    {
      return std::unexpected (
        Utf8String::from_utf8_encoded_string (
          fmt::format (
            "Cannot resolve the directory '{}': {}", directory, ec.message ())));
    }

  for (const auto &dir : { root, root / u8"data", root / u8"cache" })
    {
      if (!std::filesystem::create_directories (dir, ec) && ec)
        {
          return std::unexpected (
            Utf8String::from_utf8_encoded_string (
              fmt::format (
                "Cannot create the directory '{}': {}", dir, ec.message ())));
        }
    }

  redirect_qt_caches (root / u8"cache");

  UserDirectories directories;
  directories.private_dir_ = root;
  return directories;
}

bool
UserDirectories::is_private () const
{
  return !private_dir_.empty ();
}

std::unique_ptr<ISettingsBackend>
UserDirectories::make_settings_backend () const
{
  if (is_private ())
    {
      return std::make_unique<QSettingsBackend> (std::make_unique<QSettings> (
        Utf8String::from_path (private_dir_ / u8"settings.ini").to_qstring (),
        QSettings::IniFormat));
    }
  require_named_application ();
  return std::make_unique<QSettingsBackend> (std::make_unique<QSettings> ());
}

std::filesystem::path
UserDirectories::data_dir () const
{
  if (is_private ())
    {
      return private_dir_ / u8"data";
    }
  return Utf8String::
    from_qstring (application_location (QStandardPaths::AppDataLocation))
      .to_path ();
}

std::filesystem::path
UserDirectories::cache_dir () const
{
  if (is_private ())
    {
      return private_dir_ / u8"cache";
    }
  return Utf8String::
    from_qstring (application_location (QStandardPaths::CacheLocation))
      .to_path ();
}

std::filesystem::path
UserDirectories::plugin_scan_cache_dir () const
{
  if (is_private ())
    {
      return cache_dir ();
    }
  return Utf8String::
    from_qstring (application_location (QStandardPaths::AppLocalDataLocation))
      .to_path ();
}

std::filesystem::path
UserDirectories::default_new_project_dir () const
{
  const auto application_name = QCoreApplication::applicationName ();
  if (application_name.isEmpty ())
    {
      throw ZrythmException (
        "The application has no name: set an application name before the "
        "default directory for new projects is requested");
    }
  return Utf8String::from_qstring (
           application_location (QStandardPaths::DocumentsLocation))
           .to_path ()
         / Utf8String::from_qstring (application_name).to_path ();
}

void
UserDirectories::check_locations () const
{
  std::ignore = data_dir ();
  std::ignore = cache_dir ();
  std::ignore = plugin_scan_cache_dir ();
  std::ignore = default_new_project_dir ();
}

} // namespace zrythm::utils
