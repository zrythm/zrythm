// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <filesystem>

#include "frontends/gui/qquick/qml_utils.h"
#include "utils/utf8_string.h"

#include <QUrl>

#include <gtest/gtest.h>

namespace zrythm::frontends::gui::qquick
{

// File-dialog URLs may end in a directory separator; the path string must
// still name its last component so basename-derived display names work
TEST (QmlUtilsTest, FolderUrlWithTrailingSeparatorYieldsPathWithFilename)
{
  const auto path =
    utils::Utf8String::from_qstring (
      QmlUtils::toPathString (QUrl::fromLocalFile ("/home/user/test1/")))
      .to_path ();
  EXPECT_EQ (path, std::filesystem::path ("/home/user/test1"));
  EXPECT_EQ (path.filename (), "test1");
}

TEST (QmlUtilsTest, FolderUrlWithoutTrailingSeparatorIsUnchanged)
{
  const auto path =
    utils::Utf8String::from_qstring (
      QmlUtils::toPathString (QUrl::fromLocalFile ("/home/user/test1")))
      .to_path ();
  EXPECT_EQ (path, std::filesystem::path ("/home/user/test1"));
}

TEST (QmlUtilsTest, FileUrlIsUnchanged)
{
  const auto path =
    utils::Utf8String::from_qstring (
      QmlUtils::toPathString (QUrl::fromLocalFile ("/home/user/song.wav")))
      .to_path ();
  EXPECT_EQ (path, std::filesystem::path ("/home/user/song.wav"));
}

TEST (QmlUtilsTest, RootUrlKeepsItsSeparator)
{
  const auto path =
    utils::Utf8String::from_qstring (
      QmlUtils::toPathString (QUrl::fromLocalFile ("/")))
      .to_path ();
  EXPECT_EQ (path, std::filesystem::path ("/"));
}

} // namespace zrythm::frontends::gui::qquick
