// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "common/utils/file_path_list.h"

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
      paths.push_back (QString::fromStdString (path.string ()));
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
      add_path (path.toStdString ());
    }
}

Q_INVOKABLE void
FilePathList::addPath (const QString &path)
{
  add_path (path.toStdString ());
}

void
FilePathList::add_path (const fs::path &path)
{
  paths_.push_back (path);
}

void
FilePathList::add_paths (const juce::FileSearchPath &paths)
{
  for (int i = 0; i < paths.getNumPaths (); ++i)
    {
      add_path (paths.getRawString (i).toStdString ());
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