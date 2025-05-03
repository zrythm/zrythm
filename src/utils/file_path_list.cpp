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
  for (const auto &path : paths_)
    {
      paths.push_back (utils::std_string_to_qstring (path.string ()));
    }
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
  for (const auto &path : paths)
    {
      add_path (utils::io::qstring_to_fs_path (path));
    }
}

Q_INVOKABLE void
FilePathList::addPath (const QString &path)
{
  add_path (utils::io::qstring_to_fs_path (path));
}

void
FilePathList::add_path (const fs::path &path)
{
  paths_.push_back (path);
}

void
FilePathList::add_paths (const juce::FileSearchPath &paths)
{
  for (const int i : std::views::iota (0, paths.getNumPaths ()))
    {
      add_path (utils::io::juce_string_to_fs_path (paths.getRawString (i)));
    }
}

juce::FileSearchPath
FilePathList::get_as_juce_file_search_path () const
{
  juce::FileSearchPath path;
  for (const auto &p : paths_)
    {
      path.add (juce::File (p.string ()));
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
