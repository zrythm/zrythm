// SPDX-FileCopyrightText: © 2019-2024 Alexandros Theodotou <alex@zrythm.org>
/* SPDX-License-Identifier: LicenseRef-ZrythmLicense */

#ifndef __ZRYTHM_APP_H__
#define __ZRYTHM_APP_H__

#include "zrythm-config.h"

#include <utility>

#include "common/utils/types.h"
#include "common/utils/ui.h"
#include "gui/backend/backend/project/project_init_flow_manager.h"
#include "gui/backend/gtk_widgets/gtk_wrapper.h"
#include "gui/backend/gtk_widgets/libadwaita_wrapper.h"

#define ZRYTHM_APP_IS_GTK_THREAD \
  (zrythm_app && zrythm_app->gtk_thread_id_ == current_thread_id.get ())

TYPEDEF_STRUCT_UNDERSCORED (MainWindowWidget);
TYPEDEF_STRUCT_UNDERSCORED (BugReportDialogWidget);
TYPEDEF_STRUCT_UNDERSCORED (GreeterWidget);
class DirectoryManager;

/**
 * @addtogroup general
 *
 * @{
 */

/**
 * UI message for the message queue.
 */
struct ZrythmAppUiMessage
{
  ZrythmAppUiMessage (GtkMessageType type, std::string msg)
      : type_ (type), msg_ (std::move (msg))
  {
  }
  GtkMessageType type_;
  std::string    msg_;
};

/**
 * The Zrythm GTK application.
 *
 * Contains data that is only relevant to the GUI or command line.
 */
class ZrythmApp final : public Gtk::Application
{
public:
  /**
   * @brief Creates the application.
   *
   * This also initializes the Zrythm class.
   *
   * @param argc
   * @param argv
   */
  ZrythmApp (int argc, const char ** argv);

  void set_font_scale (double font_scale);

  /**
   * Handles the logic for checking for updates on startup.
   */
  void check_for_updates ();

  /**
   * Unlike the init thread, this will run in the main GTK thread. Do not put
   * expensive logic here.
   *
   * This should be ran after the expensive initialization has finished.
   */
  int prompt_for_project_func ();

  /**
   * Shows the trial limitation error message.
   *
   * @return Whether the limit was reached.
   */
  bool check_and_show_trial_limit_error ();

  /**
   * Initializes the array of recent projects in
   * Zrythm app.
   */
  void init_recent_projects ();

  /**
   * Install accelerator for an action.
   */
  void install_action_accel (
    const std::string &primary,
    const std::string &secondary,
    const std::string &action_name);

  /**
   * Get the primary accelerator for an action.
   */
  std::string get_primary_accel_for_action (const std::string &action_name);

  /**
   * To be used to exit Zrythm using the "response" signal on a message dialog.
   */
  static void exit_response_callback (AdwDialog * dialog, gpointer user_data);

  void on_plugin_scan_finished ();

protected:
  /**
   * First function that gets called afted CLI args are parsed and processed.
   *
   * This gets called before open or activate.
   */
  void on_startup () override;

  /*
   * Called after startup if no filename is passed on command line.
   */
  void on_activate () override;

  /**
   * Called when a filename is passed to the command line instead of activate.
   *
   * Always gets called after startup and before the tasks.
   */
  void on_open (
    const Gio::Application::type_vec_files &files,
    const Glib::ustring                    &hint) override;

  /**
   * Called immediately after the main GTK loop
   * terminates.
   *
   * This is also called manually on SIGINT.
   */
  void on_shutdown () override;

private:
  /**
   * Add the option entries.
   *
   * Things that can be processed immediately should be set as callbacks here
   * (like --version).
   *
   * Things that require to know other options before running should be set as
   * NULL and processed in the handle-local-options handler.
   */
  void add_option_entries ();

  int
  on_handle_local_options (const Glib::RefPtr<Glib::VariantDict> &opts) override;

  void lock_memory ();

  void print_settings ();
  void reset_to_factory ();

  /**
   * Called after the main window and the project have been
   * initialized. Sets up the window using the backend.
   *
   * This is the final step of initialization.
   */
  void on_setup_main_window ();

  /**
   * Called before on_load_project.
   *
   * Checks if a project was given in the command line. If not, it prompts the
   * user for a project.
   */
  void on_prompt_for_project ();

  /**
   * Called after the main window has been initialized.
   *
   * Loads the project backend or creates the default one.
   */
  void on_load_project ();

public:
  /**
   * Default settings (got from gtk_settings_get_default()).
   */
  Glib::RefPtr<Gtk::Settings> default_settings_;

  /** Main window. */
  MainWindowWidget * main_window_ = nullptr;

  /**
   * The GTK thread where the main GUI loop runs.
   *
   * This is stored for identification purposes in other threads.
   */
  // GThread * gtk_thread;
  unsigned int gtk_thread_id_ = std::numeric_limits<unsigned int>::max ();

  std::unique_ptr<UiCaches> ui_caches_;

  /** Flag to set when initialization has finished. */
  bool init_finished_ = false;

  /** Greeter screen. */
  GreeterWidget * greeter_ = nullptr;

  /**
   * True if this is the first time Zrythm is runh
   *
   * This remains true even after setting the corresponding GSettings value.
   */
  bool is_first_run_ = false;

  bool have_svg_loader_ = false;

  /** Audio backend passed with --audio-backend=,
   * if any. */
  std::string audio_backend_;

  /** MIDI backend passed with --audio-backend=,
   * if any. */
  std::string midi_backend_;

  /** Buffer size passed with --buf-size=, if any. */
  int buf_size_ = 0;

  /** Samplerate passed with --samplerate=, if any. */
  int samplerate_ = 0;

  /** Messages to show when the main window is shown. */
  std::queue<std::string> startup_error_queue_;
  std::mutex              startup_error_queue_mutex_;

  /** Output file passed with --output. */
  std::string output_file_;

  /** Whether to pretty-print. */
  bool pretty_print_ = false;

  /** CLI args. */
  int     argc_ = 0;
  char ** argv_ = nullptr;

  /** AppImage runtime path, if AppImage build. */
  std::string appimage_runtime_path_;

  /** Flag used to only show the RT priority message once. */
  bool rt_priority_message_shown_ = false;

  /**
   * Queue for messages to be shown when the project loads.
   */
  std::queue<ZrythmAppUiMessage> project_load_message_queue_;
  std::mutex                     queue_mutex_;

  /** Currently opened bug report dialog. */
  BugReportDialogWidget * bug_report_dialog_ = nullptr;

  std::unique_ptr<ProjectInitFlowManager> project_init_flow_mgr_;

  guint project_autosave_source_id_ = 0;
};

/**
 * Global variable, should be available to all files.
 */
extern Glib::RefPtr<ZrythmApp> zrythm_app;

/**
 * @}
 */

#endif /* __ZRYTHM_APP_H__ */
