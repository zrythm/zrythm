// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

#include "gui/backend/xdg_desktop_portal.h"
#include "utils/io_utils.h"
#include "utils/utf8_string.h"

#include <QTemporaryDir>

#include <gtest/gtest.h>

namespace zrythm::gui
{

class XdgDesktopPortalTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    tmp_dir_ = utils::io::make_tmp_dir ();
    data_dir_ = utils::Utf8String::from_qstring (tmp_dir_->path ()).to_path ();
  }

  /// Returns the path a portal backend descriptor with the given file
  /// name uses below a data directory, creating the portals directory
  std::filesystem::path descriptor_path (
    const std::filesystem::path &data_dir,
    std::string_view             file_name)
  {
    const auto portals_dir = data_dir / "xdg-desktop-portal" / "portals";
    std::filesystem::create_directories (portals_dir);
    return portals_dir / file_name;
  }

  /// Writes a portal backend descriptor into a data directory
  void add_portal_descriptor (
    const std::filesystem::path &data_dir,
    std::string_view             file_name,
    std::string_view             interfaces)
  {
    write_descriptor (
      data_dir, file_name,
      std::string ("[portal]\n")
        + "DBusName=org.freedesktop.impl.portal.desktop.test\nInterfaces="
        + std::string (interfaces) + "\n");
  }

  /// Writes raw portal backend descriptor contents into a data directory
  void write_descriptor (
    const std::filesystem::path &data_dir,
    std::string_view             file_name,
    std::string_view             contents)
  {
    utils::io::set_file_contents (
      descriptor_path (data_dir, file_name),
      utils::Utf8String::from_utf8_encoded_string (std::string (contents)));
  }

  std::unique_ptr<QTemporaryDir> tmp_dir_;
  std::filesystem::path          data_dir_;
};

TEST_F (XdgDesktopPortalTest, FileChooserBackendListedInDescriptor)
{
  add_portal_descriptor (
    data_dir_, "test.portal",
    "org.freedesktop.impl.portal.FileChooser;org.freedesktop.impl.portal."
    "AppChooser");
  EXPECT_TRUE (file_chooser_backend_installed (std::vector{ data_dir_ }));
}

TEST_F (XdgDesktopPortalTest, DescriptorWithoutFileChooserInterfaceIsNotEnough)
{
  add_portal_descriptor (
    data_dir_, "test.portal", "org.freedesktop.impl.portal.ScreenCast");
  EXPECT_FALSE (file_chooser_backend_installed (std::vector{ data_dir_ }));
}

TEST_F (XdgDesktopPortalTest, OnlyPortalDescriptorsAreRead)
{
  add_portal_descriptor (
    data_dir_, "test.conf", "org.freedesktop.impl.portal.FileChooser");
  EXPECT_FALSE (file_chooser_backend_installed (std::vector{ data_dir_ }));
}

TEST_F (XdgDesktopPortalTest, MissingPortalsDirectoryMeansNoBackend)
{
  EXPECT_FALSE (file_chooser_backend_installed (std::vector{ data_dir_ }));
}

TEST_F (XdgDesktopPortalTest, EveryDataDirectoryIsSearched)
{
  const auto other_dir = data_dir_ / "other-data";
  std::filesystem::create_directories (other_dir);
  add_portal_descriptor (
    other_dir, "test.portal", "org.freedesktop.impl.portal.FileChooser");
  EXPECT_TRUE (
    file_chooser_backend_installed (std::vector{ data_dir_, other_dir }));
}

TEST_F (XdgDesktopPortalTest, SpacesAroundEqualsInDescriptorAreTolerated)
{
  write_descriptor (
    data_dir_, "test.portal",
    "[portal]\nDBusName = org.freedesktop.impl.portal.desktop.test\n"
    "Interfaces = org.freedesktop.impl.portal.FileChooser;\n");
  EXPECT_TRUE (file_chooser_backend_installed (std::vector{ data_dir_ }));
}

TEST_F (XdgDesktopPortalTest, CrlfDescriptorIsTolerated)
{
  write_descriptor (
    data_dir_, "test.portal",
    "[portal]\r\nDBusName=org.freedesktop.impl.portal.desktop.test\r\n"
    "Interfaces=org.freedesktop.impl.portal.ScreenCast;"
    "org.freedesktop.impl.portal.FileChooser;\r\n");
  EXPECT_TRUE (file_chooser_backend_installed (std::vector{ data_dir_ }));
}

TEST_F (XdgDesktopPortalTest, InterfacesOutsidePortalGroupIgnored)
{
  write_descriptor (
    data_dir_, "test.portal",
    "[Other]\nInterfaces=org.freedesktop.impl.portal.FileChooser;\n"
    "[portal]\nDBusName=org.freedesktop.impl.portal.desktop.test\n");
  EXPECT_FALSE (file_chooser_backend_installed (std::vector{ data_dir_ }));
}

TEST_F (XdgDesktopPortalTest, AvailabilityFollowsXdgDataEnvironment)
{
  const auto isolated_dir = data_dir_ / "isolated-data";
  std::filesystem::create_directories (isolated_dir);
  const auto isolated_dir_qstring =
    utils::Utf8String::from_path (isolated_dir).to_qstring ().toUtf8 ();

  const std::optional<QString> previous_data_home =
    qEnvironmentVariable ("XDG_DATA_HOME");
  const std::optional<QString> previous_data_dirs =
    qEnvironmentVariable ("XDG_DATA_DIRS");
  qputenv ("XDG_DATA_HOME", isolated_dir_qstring);
  qputenv ("XDG_DATA_DIRS", isolated_dir_qstring);

  EXPECT_FALSE (portal_file_dialogs_available ());

  add_portal_descriptor (
    isolated_dir, "test.portal", "org.freedesktop.impl.portal.FileChooser");
  EXPECT_TRUE (portal_file_dialogs_available ());

  const auto restore =
    [] (const char * name, const std::optional<QString> &previous) {
      if (previous)
        qputenv (name, previous->toUtf8 ());
      else
        qunsetenv (name);
    };
  restore ("XDG_DATA_HOME", previous_data_home);
  restore ("XDG_DATA_DIRS", previous_data_dirs);
}

} // namespace zrythm::gui
