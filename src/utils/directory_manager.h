// SPDX-FileCopyrightText: © 2019-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __UTILS_DIRECTORY_MANAGER_H__
#define __UTILS_DIRECTORY_MANAGER_H__

#include <utility>

#include "juce_wrapper.h"
#include "utils/types.h"

class IDirectoryManager
{
public:
  /**
   * Type of directory.
   *
   * System* directories are directories that are determined during
   * installation that contain immutable data.
   *
   * User* directories are directories determined based on the user or the
   * user's preferences that contain user-modifiable data.
   */
  enum class DirectoryType
  {
    /**
     * The prefix, or in the case of windows installer the root dir
     * (C/program files/zrythm), or in the case of macos installer the
     * bundle path.
     *
     * In all cases, "share" is expected to be found in this dir.
     */
    SYSTEM_PREFIX,

    /** "bin" under @ref SYSTEM_PREFIX. */
    SYSTEM_BINDIR,

    /** "share" under @ref SYSTEM_PREFIX. */
    SYSTEM_PARENT_DATADIR,

    /** libdir name under
     * @ref SYSTEM_PREFIX. */
    SYSTEM_PARENT_LIBDIR,

    /** libdir/zrythm */
    SYSTEM_ZRYTHM_LIBDIR,

    /** libdir/zrythm/lv2 */
    SYSTEM_BUNDLED_PLUGINSDIR,

    /** Localization under "share". */
    SYSTEM_LOCALEDIR,

    /**
     * "gtksourceview-5/language-specs" under "share".
     */
    SYSTEM_SOURCEVIEW_LANGUAGE_SPECS_DIR,

    /**
     * "gtksourceview-5/language-specs" under "share/zrythm".
     */
    SYSTEM_BUNDLED_SOURCEVIEW_LANGUAGE_SPECS_DIR,

    /** share/zrythm */
    SYSTEM_ZRYTHM_DATADIR,

    /** Samples. */
    SYSTEM_SAMPLESDIR,

    /** Scripts. */
    SYSTEM_SCRIPTSDIR,

    /** Themes. */
    SYSTEM_THEMESDIR,

    /** CSS themes. */
    SYSTEM_THEMES_CSS_DIR,

    /** Icon themes. */
    SYSTEM_THEMES_ICONS_DIR,

    /**
     * Special external Zrythm plugins path (not part of the Zrythm source
     * code).
     *
     * Used for ZLFO and other plugins.
     */
    SYSTEM_SPECIAL_LV2_PLUGINS_DIR,

    /** The directory fonts/zrythm under datadir. */
    SYSTEM_FONTSDIR,

    /** Project templates. */
    SYSTEM_TEMPLATES,

    /* ************************************ */

    /** Main zrythm directory from gsettings. */
    USER_TOP,

    /** Subdirs of @ref USER_TOP. */
    USER_PROJECTS,
    USER_TEMPLATES,
    USER_THEMES,

    /** User CSS themes. */
    USER_THEMES_CSS,

    /** User icon themes. */
    USER_THEMES_ICONS,

    /** User scripts. */
    USER_SCRIPTS,

    /** Log files. */
    USER_LOG,

    /** Profiling files. */
    USER_PROFILING,

    /** Gdb backtrace files. */
    USER_GDB,

    /** Backtraces. */
    USER_BACKTRACE,
  };

  /**
   * Returns the prefix or in the case of Windows the root dir (C/program
   * files/zrythm) or in the case of macos the bundle path.
   *
   * In all cases, "share" is expected to be found in this dir.
   *
   * @return A newly allocated string.
   */
  virtual fs::path get_prefix () const = 0;

  /**
   * Gets the zrythm directory, either from the settings if non-empty, or the
   * default ($XDG_DATA_DIR/zrythm).
   *
   * @param force_default Ignore the settings and get the default dir.
   */
  virtual fs::path get_user_dir (bool force_default) = 0;

  /**
   * Returns the default user "zrythm" dir.
   *
   * This is used when resetting or when the dir is not selected by the user
   * yet.
   */
  virtual fs::path get_default_user_dir () = 0;

  /**
   * Returns a Zrythm directory specified by @ref type.
   *
   * @return A newly allocated string.
   */
  virtual fs::path get_dir (DirectoryType type);

  virtual ~IDirectoryManager () = default;
};

/**
 * @brief This can just be created on the stack as needed since it uses globally
 * available information.
 */
class DirectoryManager : public IDirectoryManager
{
public:
  using UserDirProvider = std::function<fs::path ()>;
  using DefaultUserDirProvider = std::function<fs::path ()>;
  using ApplicationDirPathProvider = std::function<fs::path ()>;

public:
  DirectoryManager (
    UserDirProvider            user_dir_provider,
    DefaultUserDirProvider     default_user_dir_provider,
    ApplicationDirPathProvider application_dir_path_provider)
      : user_dir_provider_ (std::move (user_dir_provider)),
        default_user_dir_provider_ (std::move (default_user_dir_provider)),
        application_dir_path_provider_ (std::move (application_dir_path_provider))
  {
  }
  ~DirectoryManager () override = default;

  fs::path get_prefix () const override;
  fs::path get_user_dir (bool force_default) override;
  fs::path get_default_user_dir () override;

private:
  UserDirProvider            user_dir_provider_;
  DefaultUserDirProvider     default_user_dir_provider_;
  ApplicationDirPathProvider application_dir_path_provider_;
};

struct TestingDirectoryManager : public IDirectoryManager
{
  TestingDirectoryManager () = default;
  Q_DISABLE_COPY_MOVE (TestingDirectoryManager)
  ~TestingDirectoryManager () override { remove_testing_dir (); }

  fs::path get_user_dir (bool force_default) override;

  /**
   * @brief Returns the current testing dir.
   *
   * If empty, this creates a new testing dir on the disk.
   */
  const fs::path &get_testing_dir ();

  /** Clears @ref "testing_dir" and removes the testing dir from the disk. */
  void remove_testing_dir ();

  fs::path get_prefix () const override;
  fs::path get_default_user_dir () override;

  /** Zrythm directory used during unit tests. */
  fs::path testing_dir_;
};

#endif // __UTILS_DIRECTORY_MANAGER_H__
