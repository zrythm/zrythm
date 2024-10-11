// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

namespace zrythm::utils
{

class FilePathList final : public QObject
{
  Q_OBJECT
  Q_PROPERTY (
    QStringList paths READ getPaths WRITE setPaths NOTIFY pathsChanged FINAL)
  QML_ELEMENT

public:
  FilePathList (QObject * parent = nullptr);
  FilePathList (juce::FileSearchPath paths, QObject * parent = nullptr);

  Q_SIGNAL void pathsChanged ();

  QStringList getPaths () const;
  void        setPaths (const QStringList &paths);

  Q_INVOKABLE void addPath (const QString &path);
  Q_INVOKABLE void clear ();

  void                 add_path (const fs::path &path);
  void                 add_paths (const juce::FileSearchPath &paths);
  juce::FileSearchPath get_as_juce_file_search_path () const;

  /**
   * @brief For debugging.
   */
  void print (const std::string &title) const;

  auto begin () const { return paths_.begin (); }
  auto end () const { return paths_.end (); }
  auto begin () { return paths_.begin (); }
  auto end () { return paths_.end (); }

  bool empty () const { return paths_.empty (); }

private:
  std::vector<fs::path> paths_;
};

} // namespace zrythm::utils