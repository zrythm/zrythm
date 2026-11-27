// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef ZRYTHM_TESTS_E2E_LAUNCHER_H
#define ZRYTHM_TESTS_E2E_LAUNCHER_H

/**
 * @file
 * Starts the application as a subprocess and reports how the run ended.
 *
 * Every run gets a private user data directory, a private home directory
 * and a private working directory below one temporary tree, so a run does
 * not read or write the files of the user who runs the tests.
 */

#include <algorithm>
#include <chrono>
#include <functional>
#include <ranges>
#include <stdexcept>
#include <string>

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QProcessEnvironment>
#include <QRegularExpression>
#include <QString>
#include <QStringList>
#include <QTemporaryDir>

#include <gtest/gtest.h>

namespace zrythm::e2e
{

/** Time for a run that ends before the event loop starts. */
inline constexpr auto kExitsWithoutEventLoop = std::chrono::seconds{ 30 };

/**
 * Time for a run with --quit-after-startup. The application quits by
 * itself within its startup limit (60 s) and its quit delay (5 s).
 */
inline constexpr auto kQuitAfterStartup = std::chrono::seconds{ 100 };

/** Time the launcher waits after terminate() and after kill(). */
inline constexpr auto kEndWait = std::chrono::seconds{ 5 };

/** Returns @p duration in milliseconds, the unit of the waits of QProcess. */
[[nodiscard]] inline int
to_msecs (std::chrono::milliseconds duration)
{
  return static_cast<int> (duration.count ());
}

/** Removes the color codes of the log from @p text. */
[[nodiscard]] inline QString
without_color_codes (QString text)
{
  static const QRegularExpression color_codes{
    QStringLiteral ("\x1b\\[[0-9;]*m")
  };
  return text.remove (color_codes);
}

/** Counts the lines of @p output that hold @p text. */
[[nodiscard]] inline int
count_lines_holding (const QString &output, const QString &text)
{
  return static_cast<int> (
    std::ranges::count_if (output.split (u'\n'), [&text] (const QString &line) {
      return line.contains (text);
    }));
}

/** True when @p output holds a log line of error or critical level. */
[[nodiscard]] inline bool
has_error_lines (const QString &output)
{
  const auto plain_output = without_color_codes (output);
  return count_lines_holding (plain_output, QStringLiteral ("[error]")) > 0
         || count_lines_holding (plain_output, QStringLiteral ("[critical]")) > 0;
}

/** Returns @p text in UTF-8, for the messages of GoogleTest. */
[[nodiscard]] inline std::string
to_utf8 (const QString &text)
{
  return text.toUtf8 ().toStdString ();
}

/** Output and end state of one run of the application. */
struct RunResult
{
  /** True when the operating system started the application. */
  bool                 started{};
  QProcess::ExitStatus exit_status{};
  int                  exit_code{};
  /** True when the launcher ended the application at the deadline. */
  bool                      ended_by_launcher{};
  std::chrono::milliseconds deadline{};
  QString                   standard_output;
  QString                   standard_error;
  QStringList               arguments;

  /** Returns the arguments, how the run ended and both outputs. */
  [[nodiscard]] std::string describe () const;
};

/**
 * Checks that the application started and ended by itself with @p
 * expected. Prints the run when a check fails.
 */
inline void
expect_exit_code (const RunResult &result, int expected)
{
  EXPECT_TRUE (result.started) << result.describe ();
  EXPECT_FALSE (result.ended_by_launcher) << result.describe ();
  EXPECT_EQ (result.exit_status, QProcess::NormalExit) << result.describe ();
  EXPECT_EQ (result.exit_code, expected) << result.describe ();
}

inline std::string
RunResult::describe () const
{
  QString text = QStringLiteral ("Arguments:");
  for (const auto &argument : arguments)
    {
      text += QStringLiteral (" \"%1\"").arg (argument);
    }
  text += u'\n';
  if (!started)
    {
      text += QStringLiteral ("The application did not start.\n");
    }
  else if (ended_by_launcher)
    {
      text +=
        QStringLiteral (
          "The application did not exit within %1 ms; the launcher "
          "terminated it.\n")
          .arg (to_msecs (deadline));
    }
  else
    {
      text +=
        QStringLiteral ("Exit status: %1, exit code: %2\n")
          .arg (
            exit_status == QProcess::NormalExit
              ? QStringLiteral ("normal")
              : QStringLiteral ("crash"))
          .arg (exit_code);
    }
  text += QStringLiteral ("--- Standard output ---\n");
  text += without_color_codes (standard_output);
  text += QStringLiteral ("\n--- Standard error ---\n");
  text += without_color_codes (standard_error);
  return to_utf8 (text);
}

/**
 * @brief Starts the application with private directories.
 *
 * The directories live in one temporary tree that is removed when the
 * launcher is destroyed.
 */
class ZrythmLauncher
{
public:
  ZrythmLauncher ();
  ZrythmLauncher (const ZrythmLauncher &) = delete;
  ZrythmLauncher &operator= (const ZrythmLauncher &) = delete;

  /**
   * @brief Runs the application and waits for its end.
   *
   * @param arguments Command line arguments, without the program name.
   * @param deadline Time the application gets to exit.
   * @return The output and the end state of the run.
   *
   * The run gets @c --user-data-dir with the private directory of the
   * launcher, and the application starts in the private working
   * directory. When the application still runs at @p deadline, the
   * launcher terminates it and the result says so.
   */
  [[nodiscard]] RunResult
  run (QStringList arguments, std::chrono::milliseconds deadline) const;

  /** @brief Same, with @p user_data_dir as the value of @c --user-data-dir. */
  [[nodiscard]] RunResult run (
    QStringList               arguments,
    std::chrono::milliseconds deadline,
    const QString            &user_data_dir) const;

  /** Creates an empty directory below the temporary tree. */
  [[nodiscard]] QString make_directory (const QString &name) const;

  /** Creates an empty file below the temporary tree. */
  [[nodiscard]] QString make_file (const QString &name) const;

  /** Returns the private directory of the settings, data and caches. */
  [[nodiscard]] QString user_data_dir () const;

  /** Returns the private home directory of the runs. */
  [[nodiscard]] QString home_dir () const;

  /** Returns the working directory of the runs. */
  [[nodiscard]] QString working_dir () const;

  /**
   * @brief Predicate for the entries tree_entries() leaves out.
   *
   * Returns true to leave the entry out. The argument is the path of
   * the entry relative to the listed directory.
   */
  using TreeEntryFilter = std::function<bool (const QString &relative_path)>;

  /**
   * @brief Returns one line per entry below @p directory.
   *
   * A line holds the relative path, the size and the modification time
   * in milliseconds of one entry. Hidden entries are included and the
   * lines are sorted.
   *
   * @param ignore Entries for which it returns true are left out.
   */
  [[nodiscard]] static QStringList
  tree_entries (const QString &directory, const TreeEntryFilter &ignore = {});

private:
  /** Returns the environment of the runs. */
  [[nodiscard]] QProcessEnvironment environment () const;

  QTemporaryDir temp_dir_;
  QString       user_data_dir_;
  QString       home_dir_;
  QString       working_dir_;
};

inline ZrythmLauncher::ZrythmLauncher ()
    : temp_dir_ (QDir::tempPath () + QStringLiteral ("/zrythm-e2e-XXXXXX"))
{
  if (!temp_dir_.isValid ())
    {
      throw std::runtime_error (
        "Cannot create the temporary tree of the end-to-end tests");
    }
  const auto root = temp_dir_.path ();
  user_data_dir_ = root + QStringLiteral ("/user-data");
  home_dir_ = root + QStringLiteral ("/home");
  working_dir_ = root + QStringLiteral ("/working-dir");

  const QDir dir;
  for (const auto &path : { user_data_dir_, home_dir_, working_dir_ })
    {
      if (!dir.mkpath (path))
        {
          throw std::runtime_error (
            "Cannot create a directory in the temporary tree of the "
            "end-to-end tests");
        }
    }
#if defined(Q_OS_LINUX)
  // the directories of the XDG variables, see environment()
  for (const auto &relative_path : { "/.config", "/.local/share", "/.cache" })
    {
      if (!dir.mkpath (home_dir_ + relative_path))
        {
          throw std::runtime_error (
            "Cannot create a directory in the temporary tree of the "
            "end-to-end tests");
        }
    }
#endif
}

inline RunResult
ZrythmLauncher::run (QStringList arguments, std::chrono::milliseconds deadline)
  const
{
  return run (std::move (arguments), deadline, user_data_dir_);
}

inline RunResult
ZrythmLauncher::run (
  QStringList               arguments,
  std::chrono::milliseconds deadline,
  const QString            &user_data_dir) const
{
  arguments.prepend (user_data_dir);
  arguments.prepend (QStringLiteral ("--user-data-dir"));

  QProcess process;
  process.setProcessEnvironment (environment ());
  process.setWorkingDirectory (working_dir_);

  RunResult result;
  result.arguments = arguments;
  result.deadline = deadline;
  process.start (QString::fromUtf8 (ZRYTHM_E2E_APP_PATH), arguments);
  result.started = process.waitForStarted ();
  if (!result.started)
    {
      // the start may have timed out after the application was created
      process.kill ();
      process.waitForFinished (to_msecs (kEndWait));
    }
  else if (!process.waitForFinished (to_msecs (deadline)))
    {
      result.ended_by_launcher = true;
      process.terminate ();
      process.waitForFinished (to_msecs (kEndWait));
      process.kill ();
      process.waitForFinished (to_msecs (kEndWait));
    }
  result.exit_status = process.exitStatus ();
  result.exit_code = process.exitCode ();
  result.standard_output = process.readAllStandardOutput ();
  result.standard_error = process.readAllStandardError ();
  return result;
}

inline QString
ZrythmLauncher::make_directory (const QString &name) const
{
  const QDir dir;
  const auto path = temp_dir_.filePath (name);
  if (!dir.mkpath (path))
    {
      throw std::runtime_error (
        "Cannot create a directory in the temporary tree of the "
        "end-to-end tests");
    }
  return path;
}

inline QString
ZrythmLauncher::make_file (const QString &name) const
{
  const QDir dir;
  const auto path = temp_dir_.filePath (name);
  dir.mkpath (QFileInfo (path).path ());
  QFile file (path);
  if (!file.open (QIODevice::WriteOnly))
    {
      throw std::runtime_error (
        "Cannot create a file in the temporary tree of the end-to-end tests");
    }
  return path;
}

inline QString
ZrythmLauncher::user_data_dir () const
{
  return user_data_dir_;
}

inline QString
ZrythmLauncher::home_dir () const
{
  return home_dir_;
}

inline QString
ZrythmLauncher::working_dir () const
{
  return working_dir_;
}

inline QStringList
ZrythmLauncher::tree_entries (
  const QString         &directory,
  const TreeEntryFilter &ignore)
{
  QStringList  entries;
  QDirIterator it (
    directory, QDir::AllEntries | QDir::Hidden | QDir::NoDotAndDotDot,
    QDirIterator::Subdirectories);
  const QDir base (directory);
  while (it.hasNext ())
    {
      it.next ();
      const auto info = it.fileInfo ();
      const auto relative_path =
        base.relativeFilePath (info.absoluteFilePath ());
      if (ignore && ignore (relative_path))
        continue;
      entries
        << QStringLiteral ("%1, %2, %3")
             .arg (
               relative_path, QString::number (info.size ()),
               QString::number (info.lastModified ().toMSecsSinceEpoch ()));
    }
  entries.sort ();
  return entries;
}

inline QProcessEnvironment
ZrythmLauncher::environment () const
{
  auto env = QProcessEnvironment::systemEnvironment ();
  env.insert (QStringLiteral ("QT_QPA_PLATFORM"), QStringLiteral ("offscreen"));
  env.insert (QStringLiteral ("ZRYTHM_FATAL_DIALOG"), QStringLiteral ("0"));
  env.insert (QStringLiteral ("ZRYTHM_SKIP_PLUGIN_SCAN"), QStringLiteral ("1"));
  // With the leak check of an AddressSanitizer build on, --help and
  // --version end with status 1 and without their text: the check runs in
  // an exit handler and ends the process before the output is flushed
  const auto inherited_asan_options =
    env.value (QStringLiteral ("ASAN_OPTIONS"));
  env.insert (
    QStringLiteral ("ASAN_OPTIONS"),
    inherited_asan_options.isEmpty ()
      ? QStringLiteral ("detect_leaks=0")
      : inherited_asan_options + QStringLiteral (":detect_leaks=0"));
#if defined(Q_OS_WIN)
  // the build tree holds only the Windows platform plugin; the Qt
  // installation also holds the offscreen plugin
  env.insert (
    QStringLiteral ("QT_PLUGIN_PATH"),
    QString::fromUtf8 (ZRYTHM_E2E_QT_PLUGINS_DIR));
#else
  env.insert (QStringLiteral ("HOME"), home_dir_);
#endif
#if defined(Q_OS_LINUX)
  env.insert (
    QStringLiteral ("XDG_CONFIG_HOME"), home_dir_ + QStringLiteral ("/.config"));
  env.insert (
    QStringLiteral ("XDG_DATA_HOME"),
    home_dir_ + QStringLiteral ("/.local/share"));
  env.insert (
    QStringLiteral ("XDG_CACHE_HOME"), home_dir_ + QStringLiteral ("/.cache"));
#endif
  return env;
}

} // namespace zrythm::e2e

#endif // ZRYTHM_TESTS_E2E_LAUNCHER_H
