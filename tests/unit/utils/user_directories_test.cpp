// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <array>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <ranges>
#include <string>
#include <vector>

#include "utils/exceptions.h"
#include "utils/io_utils.h"
#include "utils/user_directories.h"

#include <QCoreApplication>
#include <QScopeGuard>
#include <QStandardPaths>

#include "helpers/scoped_env_var.h"
#include "helpers/scoped_qcoreapplication.h"

#include <fmt/format.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#ifndef _WIN32
#  include <unistd.h>
#endif

namespace zrythm::utils
{

namespace
{

QString
to_qstring (const std::filesystem::path &path)
{
  return Utf8String::from_path (path).to_qstring ();
}

/** Characters that can be part of a C++ identifier. */
bool
is_identifier_char (char c)
{
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')
         || (c >= '0' && c <= '9') || c == '_';
}

/** Returns whether the quote at @p pos starts a character literal. */
bool
starts_character_literal (std::string_view code, std::size_t pos)
{
  if (pos == 0 || !is_identifier_char (code[pos - 1]))
    {
      return true;
    }
  // A literal such as u'a' has a prefix of its own
  return pos >= 2 && std::string_view ("uUL").contains (code[pos - 1])
         && !is_identifier_char (code[pos - 2]);
}

/** Returns whether the quote at @p pos opens a raw string literal. */
bool
starts_raw_string (std::string_view code, std::size_t pos)
{
  // The R of the prefix is the character before the quote
  if (pos == 0 || code[pos - 1] != 'R')
    {
      return false;
    }
  if (pos == 1 || !is_identifier_char (code[pos - 2]))
    {
      return true;
    }
  // A raw string such as u8R"(...)" has an encoding prefix of its own
  constexpr std::array prefixes{
    std::string_view{ "u8" }, std::string_view{ "u" }, std::string_view{ "U" },
    std::string_view{ "L" }
  };
  return std::ranges::any_of (prefixes, [code, pos] (std::string_view prefix) {
    if (pos <= prefix.size ())
      {
        return false;
      }
    const auto start = pos - prefix.size () - 1;
    return code.substr (start, prefix.size ()) == prefix
           && (start == 0 || !is_identifier_char (code[start - 1]));
  });
}

/**
 * Replaces comments and literals with spaces.
 *
 * String, character and raw string literals are replaced. Newlines are
 * kept, so each character keeps its line.
 */
std::string
without_comments_and_literals (std::string_view code)
{
  enum class State
  {
    Code,
    LineComment,
    BlockComment,
    String,
    Character,
    RawString,
  };

  std::string result (code);
  State       state = State::Code;
  std::string raw_end;

  for (std::size_t i = 0; i < code.size (); ++i)
    {
      const char c = code[i];
      const char next = i + 1 < code.size () ? code[i + 1] : '\0';
      const bool at_newline = c == '\n';
      switch (state)
        {
        case State::Code:
          if (c == '/' && (next == '/' || next == '*'))
            {
              result[i] = ' ';
              result[i + 1] = ' ';
              ++i;
              state = next == '/' ? State::LineComment : State::BlockComment;
            }
          else if (c == '"')
            {
              result[i] = ' ';
              const auto raw_open =
                starts_raw_string (code, i)
                  ? code.find ('(', i + 1)
                  : std::string_view::npos;
              if (raw_open == std::string_view::npos)
                {
                  state = State::String;
                }
              else
                {
                  // R"delimiter(...)delimiter"
                  const auto delimiter = code.substr (i + 1, raw_open - i - 1);
                  raw_end = ")" + std::string (delimiter) + "\"";
                  result.replace (i, raw_open + 1 - i, raw_open + 1 - i, ' ');
                  i = raw_open;
                  state = State::RawString;
                }
            }
          else if (c == '\'' && starts_character_literal (code, i))
            {
              result[i] = ' ';
              state = State::Character;
            }
          break;

        case State::LineComment:
          if (at_newline)
            {
              state = State::Code;
            }
          else
            {
              result[i] = ' ';
            }
          break;

        case State::BlockComment:
          if (c == '*' && next == '/')
            {
              result[i] = ' ';
              result[i + 1] = ' ';
              ++i;
              state = State::Code;
            }
          else if (!at_newline)
            {
              result[i] = ' ';
            }
          break;

        case State::String:
        case State::Character:
          if (c == '\\')
            {
              result[i] = ' ';
              if (i + 1 < code.size ())
                {
                  result[i + 1] = ' ';
                  ++i;
                }
            }
          else if (
            (state == State::String && c == '"')
            || (state == State::Character && c == '\''))
            {
              result[i] = ' ';
              state = State::Code;
            }
          else if (at_newline)
            {
              // A literal holds no newline
              state = State::Code;
            }
          else
            {
              result[i] = ' ';
            }
          break;

        case State::RawString:
          if (code.compare (i, raw_end.size (), raw_end) == 0)
            {
              result.replace (i, raw_end.size (), raw_end.size (), ' ');
              i += raw_end.size () - 1;
              state = State::Code;
            }
          else if (!at_newline)
            {
              result[i] = ' ';
            }
          break;
        }
    }
  return result;
}

/** Returns whether @p text holds @p word between non-identifier characters. */
bool
contains_word (std::string_view text, std::string_view word)
{
  std::size_t pos = text.find (word);
  while (pos != std::string_view::npos)
    {
      const auto after_pos = pos + word.size ();
      const bool bounded_before =
        pos == 0 || !is_identifier_char (text[pos - 1]);
      const bool bounded_after =
        after_pos == text.size () || !is_identifier_char (text[after_pos]);
      if (bounded_before && bounded_after)
        {
          return true;
        }
      pos = text.find (word, after_pos);
    }
  return false;
}

bool
contains_ignoring_case (std::string_view text, std::string_view part)
{
  const auto lowered =
    text | std::views::transform ([] (char c) {
      return static_cast<char> (std::tolower (static_cast<unsigned char> (c)));
    })
    | std::ranges::to<std::string> ();
  return lowered.contains (part);
}

std::string
read_file (const std::filesystem::path &path)
{
  std::ifstream file (path, std::ios::binary);
  return {
    std::istreambuf_iterator<char> (file), std::istreambuf_iterator<char> ()
  };
}

/** Words that only utils/user_directories.cpp may use. */
constexpr std::array forbidden_words{ "writableLocation", "QSettings" };

/**
 * Returns a "line and word" entry for each line of @p code that uses a
 * forbidden word, and a "#include" entry for each line that includes a
 * file with "qsettings" in its name and holds no forbidden word outside
 * the file name.
 *
 * @p file is named in each entry.
 */
std::vector<std::string>
findings_in (std::string_view file, std::string_view code)
{
  std::vector<std::string> findings;
  // Only a file that holds a word somewhere is examined in full
  if (
    !contains_ignoring_case (code, "qsettings")
    && !code.contains (forbidden_words[0]))
    {
      return findings;
    }

  const auto  masked = without_comments_and_literals (code);
  std::size_t start = 0;
  std::size_t line_number = 1;
  while (start < code.size ())
    {
      const auto newline = code.find ('\n', start);
      const auto end =
        newline == std::string_view::npos ? code.size () : newline;
      const std::string_view line (masked.data () + start, end - start);
      const std::string_view raw_line (code.data () + start, end - start);
      const auto             findings_before_line = findings.size ();
      for (const auto word : forbidden_words)
        {
          if (contains_word (line, word))
            {
              findings.push_back (
                fmt::format ("{}:{}: {}", file, line_number, word));
            }
        }
      // The name of an included file is a string literal, so it is not in
      // the masked line
      if (
        findings.size () == findings_before_line && line.contains ("#include")
        && contains_ignoring_case (raw_line, "qsettings"))
        {
          findings.push_back (
            fmt::format ("{}:{}: #include", file, line_number));
        }
      start = end + 1;
      ++line_number;
    }
  return findings;
}

} // namespace

class UserDirectoriesTest : public ::testing::Test
{
protected:
  void SetUp () override { tmp_dir_ = io::make_tmp_dir (); }

  /** Path of the temporary directory of the test. */
  [[nodiscard]] std::filesystem::path tmp_path () const
  {
    return Utf8String::from_qstring (tmp_dir_->path ()).to_path ();
  }

private:
  std::unique_ptr<QTemporaryDir> tmp_dir_{};
  test_helpers::QtCacheVariables qt_cache_variables_;
};

TEST_F (UserDirectoriesTest, DefaultLocationsComeFromTheOperatingSystem)
{
  test_helpers::ScopedQCoreApplication app;
  QCoreApplication::setApplicationName (QStringLiteral ("Zrythm-test"));
  QCoreApplication::setOrganizationName (QStringLiteral ("Zrythm.org-test"));

  const UserDirectories directories;

  EXPECT_NO_THROW (directories.check_locations ());
  EXPECT_FALSE (directories.is_private ());
  EXPECT_EQ (
    directories.data_dir (),
    Utf8String::from_qstring (
      QStandardPaths::writableLocation (QStandardPaths::AppDataLocation))
      .to_path ());
  EXPECT_EQ (
    directories.cache_dir (),
    Utf8String::from_qstring (
      QStandardPaths::writableLocation (QStandardPaths::CacheLocation))
      .to_path ());
  EXPECT_EQ (
    directories.plugin_scan_cache_dir (),
    Utf8String::from_qstring (
      QStandardPaths::writableLocation (QStandardPaths::AppLocalDataLocation))
      .to_path ());
  EXPECT_EQ (
    directories.default_new_project_dir (),
    Utf8String::from_qstring (
      QStandardPaths::writableLocation (QStandardPaths::DocumentsLocation))
        .to_path ()
      / u8"Zrythm-test");
}

TEST_F (UserDirectoriesTest, LocationWithoutANamedApplicationThrows)
{
  // No application object: Qt gives one the name of its executable
  const UserDirectories directories;

  EXPECT_THROW (directories.check_locations (), ZrythmException);
  EXPECT_THROW (std::ignore = directories.data_dir (), ZrythmException);
  EXPECT_THROW (std::ignore = directories.cache_dir (), ZrythmException);
  EXPECT_THROW (
    std::ignore = directories.plugin_scan_cache_dir (), ZrythmException);
  EXPECT_THROW (
    std::ignore = directories.default_new_project_dir (), ZrythmException);

  // An organization name alone is enough for the other locations
  QCoreApplication::setOrganizationName (QStringLiteral ("Zrythm.org-test"));
  EXPECT_NO_THROW (std::ignore = directories.data_dir ());
  EXPECT_THROW (
    std::ignore = directories.default_new_project_dir (), ZrythmException);
  EXPECT_THROW (directories.check_locations (), ZrythmException);
  QCoreApplication::setOrganizationName (QString ());
}

TEST_F (UserDirectoriesTest, PrivateDirectoriesAreCreatedUnderTheGivenDirectory)
{
  const auto profile = tmp_path () / "profile";

  const auto directories = UserDirectories::make_private (profile);

  ASSERT_TRUE (directories) << directories.error ().view ();
  EXPECT_TRUE (directories->is_private ());
  EXPECT_EQ (directories->data_dir (), profile / u8"data");
  EXPECT_TRUE (std::filesystem::is_directory (directories->data_dir ()));
  EXPECT_EQ (directories->cache_dir (), profile / u8"cache");
  EXPECT_TRUE (std::filesystem::is_directory (directories->cache_dir ()));
  EXPECT_EQ (directories->plugin_scan_cache_dir (), directories->cache_dir ());
}

TEST_F (UserDirectoriesTest, RelativeDirectoryIsBelowTheCurrentDirectory)
{
  // The current directory is put back afterwards
  const auto restore_current_path =
    qScopeGuard ([old = std::filesystem::current_path ()] {
      std::filesystem::current_path (old);
    });
  std::filesystem::current_path (tmp_path ());
  const std::filesystem::path relative = u8"profile";

  const auto directories = UserDirectories::make_private (relative);

  ASSERT_TRUE (directories) << directories.error ().view ();
  // The current directory is reported with its symbolic links resolved
  EXPECT_TRUE (
    std::filesystem::equivalent (
      directories->data_dir (), tmp_path () / relative / u8"data"));
}

TEST_F (UserDirectoriesTest, EmptyDirectoryIsRejected)
{
  const auto directories = UserDirectories::make_private ({});

  EXPECT_FALSE (directories);
  EXPECT_FALSE (directories.error ().empty ());
}

TEST_F (UserDirectoriesTest, DirectoryThatIsAFileIsRejected)
{
  const auto file = tmp_path () / "file";
  io::set_file_contents (file, Utf8String (u8"not a directory"));

  const auto directories = UserDirectories::make_private (file);

  EXPECT_FALSE (directories);
}

TEST_F (UserDirectoriesTest, DirectoryBelowAFileIsRejected)
{
  const auto file = tmp_path () / "file";
  io::set_file_contents (file, Utf8String (u8"not a directory"));

  const auto directories = UserDirectories::make_private (file / "sub");

  EXPECT_FALSE (directories);
}

#ifndef _WIN32
TEST_F (UserDirectoriesTest, UnwritableDirectoryIsRejected)
{
  if (::geteuid () == 0)
    {
      GTEST_SKIP () << "The root user ignores directory permissions";
    }

  const auto profile = tmp_path () / "profile";
  std::filesystem::create_directories (profile);
  std::filesystem::permissions (
    profile,
    std::filesystem::perms::owner_read | std::filesystem::perms::owner_exec);
  // The permissions go back, so that the temporary directory can be removed
  const auto restore_permissions = qScopeGuard ([profile] {
    std::filesystem::permissions (profile, std::filesystem::perms::owner_all);
  });

  const auto directories = UserDirectories::make_private (profile);

  EXPECT_FALSE (directories);
}
#endif

TEST_F (UserDirectoriesTest, PrivateSettingsAreStoredInTheDirectory)
{
  const auto profile = tmp_path () / "profile";
  const auto directories = UserDirectories::make_private (profile);
  ASSERT_TRUE (directories) << directories.error ().view ();

  directories->make_settings_backend ()->setValue ("General/theme", "dark");

  ASSERT_TRUE (std::filesystem::exists (profile / "settings.ini"));
  EXPECT_EQ (
    directories->make_settings_backend ()->value ("General/theme", {}),
    QVariant (QStringLiteral ("dark")));
}

TEST_F (
  UserDirectoriesTest,
  PrivateDirectoryPointsTheCachesOfQtQuickToItsCacheDirectory)
{
  const auto directories =
    UserDirectories::make_private (tmp_path () / "profile");
  ASSERT_TRUE (directories) << directories.error ().view ();
  const auto pipeline_cache = directories->cache_dir () / u8"qtpipelinecache";

  EXPECT_EQ (
    qEnvironmentVariable ("QML_DISK_CACHE_PATH"),
    to_qstring (directories->cache_dir () / u8"qmlcache"));
  EXPECT_EQ (qEnvironmentVariable ("QSG_RHI_DISABLE_DISK_CACHE"), "1");
  EXPECT_EQ (
    qEnvironmentVariable ("QSG_RHI_PIPELINE_CACHE_SAVE"),
    to_qstring (pipeline_cache));
  EXPECT_FALSE (qEnvironmentVariableIsSet ("QSG_RHI_PIPELINE_CACHE_LOAD"));
}

TEST_F (UserDirectoriesTest, PipelineCacheIsLoadedWhenItsFileExists)
{
  const auto profile = tmp_path () / "profile";
  ASSERT_TRUE (UserDirectories::make_private (profile));
  const auto pipeline_cache = profile / "cache" / "qtpipelinecache";
  io::set_file_contents (pipeline_cache, Utf8String (u8"cache data"));

  const auto directories = UserDirectories::make_private (profile);

  ASSERT_TRUE (directories) << directories.error ().view ();
  EXPECT_EQ (
    qEnvironmentVariable ("QSG_RHI_PIPELINE_CACHE_LOAD"),
    to_qstring (pipeline_cache));
}

TEST_F (UserDirectoriesTest, NewProjectDirectoryIsInTheDocumentsDirectory)
{
  test_helpers::ScopedQCoreApplication app;
  QCoreApplication::setApplicationName (QStringLiteral ("Zrythm-test"));
  const auto directories =
    UserDirectories::make_private (tmp_path () / "profile");
  ASSERT_TRUE (directories) << directories.error ().view ();
  EXPECT_NO_THROW (directories->check_locations ());

  const auto documents =
    Utf8String::from_qstring (
      QStandardPaths::writableLocation (QStandardPaths::DocumentsLocation))
      .to_path ();

  EXPECT_EQ (
    directories->default_new_project_dir (), documents / u8"Zrythm-test");
}

TEST (SourceScanTest, IgnoresCommentsAndLiterals)
{
  // Each line holds a word in a place that is not code
  const std::string code = R"scan(
// QSettings settings;
/* QStandardPaths::writableLocation (QStandardPaths::CacheLocation) */
/*
" QSettings
*/
const char * a = "QSettings";
const char * b = "not a comment: // \" QSettings";
const char * c = R"x(QSettings and writableLocation)x";
const char * d = R"x(")x" "writableLocation";
int          e = 1'000'000'000 + f (x);
char         g = '"';
char         h = '\'';
char         i = u'\"';
const char * j = u8R"x(" QSettings and writableLocation)x";
const char * k = LR"(QSettings)";
)scan";

  EXPECT_TRUE (findings_in ("file", code).empty ());
}

TEST (SourceScanTest, FindsCallsAndIncludes)
{
  // Code is examined after a number with digit separators, after a raw
  // string that holds a quote, after a comment that holds a quote, and
  // after a quote that follows a name ending in R. It is also examined on
  // a line that holds an include in a trailing comment
  const std::string calls =
    "auto a = QStandardPaths::\n  writableLocation (x);\n"
    "auto b = my_writableLocation (x);\n"
    "auto c = QSettingsBackend{};\n"
    "auto d = 1'000'000'000 + f (QSettings);\n"
    "auto e = R\"x(\")x\"; QSettings s;\n"
    "/*\n\" QSettings\n*/\nauto f = QSettings{};\n"
    "auto g = DIR\"x\" + QSettings{};\n"
    "auto h = f (QSettings{});\n"
    "auto i = QSettings{}; // #include <vector>\n"
    "auto j = QStandardPaths::writableLocation (x); // #include <vector>\n";
  EXPECT_THAT (
    findings_in ("file", calls),
    testing::ElementsAre (
      "file:2: writableLocation", "file:5: QSettings", "file:6: QSettings",
      "file:10: QSettings", "file:11: QSettings", "file:12: QSettings",
      "file:13: QSettings", "file:14: writableLocation"));

  EXPECT_THAT (
    findings_in (
      "file",
      "#include <QSettings>\n#include \"QSettings\"\n"
      "#include \"utils/app_settings.h\"\n"),
    testing::ElementsAre ("file:1: QSettings", "file:2: #include"));
}

TEST_F (UserDirectoriesTest, OnlyUserDirectoriesKnowsLocationsToWriteTo)
{
  const std::filesystem::path source_dir (ZRYTHM_SRC_DIR);
  const auto                  only_allowed =
    std::filesystem::path ("utils") / "user_directories.cpp";
  std::vector<std::string> findings;

  for (
    const auto &entry :
    std::filesystem::recursive_directory_iterator (source_dir))
    {
      std::error_code ec;
      if (!entry.is_regular_file (ec))
        {
          continue;
        }
      const auto extension = entry.path ().extension ();
      if (
        extension != ".h" && extension != ".hpp" && extension != ".cpp"
        && extension != ".cc")
        {
          continue;
        }
      const auto relative =
        std::filesystem::relative (entry.path (), source_dir);
      if (relative == only_allowed)
        {
          continue;
        }
      for (
        auto &finding :
        findings_in (relative.generic_string (), read_file (entry.path ())))
        {
          findings.push_back (std::move (finding));
        }
    }

  std::string listing;
  for (const auto &finding : findings)
    {
      listing += "  " + finding + "\n";
    }
  EXPECT_TRUE (findings.empty ())
    << "writableLocation and QSettings are only allowed in " << only_allowed
    << ". A location comes from utils::UserDirectories. Found:\n"
    << listing;
}

} // namespace zrythm::utils
