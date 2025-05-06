// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <ranges>

#include "utils/file_path_list.h"
#include "utils/logger.h"

using namespace zrythm::utils;

FilePathList::FilePathList (QObject * parent) : QObject (parent) { }

FilePathList::FilePathList (juce::FileSearchPath paths, QObject * parent)
    : QObject (parent)
{
  add_paths (paths);
}

QStringList
FilePathList::getPaths () const
{
  QStringList paths;
  std::ranges::transform (
    paths_, std::back_inserter (paths), [] (const fs::path &path) {
      return utils::Utf8String::from_path (path).to_qstring ();
    });
  return paths;
}

void
FilePathList::clear ()
{
  paths_.clear ();
}

void
FilePathList::setPaths (const QStringList &paths)
{
  clear ();
  std::ranges::transform (
    paths, std::back_inserter (paths_), [] (const QString &path) {
      return utils::Utf8String::from_qstring (path).to_path ();
    });
}

Q_INVOKABLE void
FilePathList::addPath (const QString &path)
{
  add_path (utils::Utf8String::from_qstring (path));
}

void
FilePathList::add_path (const fs::path &path)
{
  paths_.push_back (path);
}

void
FilePathList::add_path (const Utf8String &path)
{
  paths_.push_back (path.to_path ());
}

void
FilePathList::add_paths (const juce::FileSearchPath &paths)
{
  for (const int i : std::views::iota (0, paths.getNumPaths ()))
    {
      add_path (utils::Utf8String::from_juce_string (paths.getRawString (i)));
    }
}

juce::FileSearchPath
FilePathList::get_as_juce_file_search_path () const
{
  juce::FileSearchPath path;
  for (const auto &p : paths_)
    {
      path.add (juce::File (Utf8String::from_path (p).to_juce_string ()));
    }
  return path;
}

void
FilePathList::print (const std::string &title) const
{
  std::string str = title + ":\n";
  for (const auto &path : paths_)
    {
      str += path.string () + "\n";
    }
  z_debug (str);
}
