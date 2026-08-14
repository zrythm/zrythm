// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/backend/plugin_header_qml.h"
#include "gui/backend/x11_plugin_host_window.h"
#include "plugins/host_window_units.h"
#include "plugins/plugin.h"
#include "utils/logger.h"
#include "utils/qt.h"

#if defined(__SANITIZE_ADDRESS__)
#  define ZRYTHM_LSAN_ACTIVE 1
#elif defined(__has_feature)
#  if __has_feature(address_sanitizer) || __has_feature(leak_sanitizer)
#    define ZRYTHM_LSAN_ACTIVE 1
#  endif
#endif

#ifdef ZRYTHM_LSAN_ACTIVE
#  include <sanitizer/lsan_interface.h>
#endif

#ifdef __linux__

#  include <algorithm>
#  include <array>
#  include <cassert>
#  include <chrono>
#  include <cmath>
#  include <cstdlib>
#  include <mutex>
#  include <optional>
#  include <ranges>
#  include <unordered_map>
#  include <vector>

#  include <QCoreApplication>
#  include <QGuiApplication>
#  include <QHoverEvent>
#  include <QImage>
#  include <QMouseEvent>
#  include <QPainter>
#  include <QPalette>
#  include <QQuickWindow>
#  include <QSocketNotifier>
#  include <QTimer>

#  include <X11/X.h>
#  include <X11/Xatom.h>
#  include <X11/Xlib.h>
#  include <X11/Xresource.h>
#  include <X11/Xutil.h>

namespace zrythm::gui
{

class X11PluginHostWindow::Impl
{
public:
  explicit Impl (X11PluginHostWindow &q_ptr) : q_ptr_ (q_ptr) { }

  /**
   * @brief Finds the plugin's client window among the embed area's
   * children and completes the XEmbed handshake, retrying while it has not
   * appeared yet.
   */
  void complete_native_embedding (Display &dpy, int attempts_remaining);

  /** Header strip height in physical pixels. */
  int header_height_px () const
  {
    const int logical_height =
      header_ != nullptr
        ? header_->implicit_height ()
        : static_cast<int> (kHeaderHeight);
    return plugins::host_window_logical_to_physical (
      logical_height, scale_factor_);
  }

  /** Plugin view width/height in physical pixels. */
  int physical_view_width () const
  {
    return plugins::host_window_logical_to_physical (
      view_width_logical_, scale_factor_);
  }
  int physical_view_height () const
  {
    return plugins::host_window_logical_to_physical (
      view_height_logical_, scale_factor_);
  }

  /** Sizes and positions the header strip and embed area for the given
   * plugin view size in logical pixels. */
  void
  layout_children (Display &dpy, int view_width_logical, int view_height_logical)
    const
  {
    const auto hh = static_cast<unsigned> (header_height_px ());
    const auto w = static_cast<unsigned> (std::max (
      plugins::host_window_logical_to_physical (
        view_width_logical, scale_factor_),
      1));
    const auto h = static_cast<unsigned> (std::max (
      plugins::host_window_logical_to_physical (
        view_height_logical, scale_factor_),
      1));
    XResizeWindow (&dpy, header_win_, w, hh);
    XMoveResizeWindow (&dpy, embed_win_, 0, static_cast<int> (hh), w, h);
    if (header_ != nullptr)
      {
        header_->set_device_pixel_ratio (scale_factor_);
        header_->resize (view_width_logical, header_->implicit_height ());
      }
  }

  /**
   * @brief Forwards an X button event to the offscreen QML header as a
   * QMouseEvent.
   *
   * X coordinates are physical pixels; the scene uses logical pixels.
   */
  void forward_button_event (const XEvent &ev)
  {
    if (header_ == nullptr || !header_->is_valid ())
      return;
    const bool press = ev.type == ButtonPress;
    button_pressed_ = press;
    const QPointF pos (
      ev.xbutton.x / scale_factor_, ev.xbutton.y / scale_factor_);
    const QPointF global_pos (
      ev.xbutton.x_root / scale_factor_, ev.xbutton.y_root / scale_factor_);
    QMouseEvent mouse_ev (
      press ? QEvent::MouseButtonPress : QEvent::MouseButtonRelease, pos, pos,
      global_pos, Qt::LeftButton, press ? Qt::LeftButton : Qt::NoButton,
      Qt::NoModifier);
    QCoreApplication::sendEvent (header_->quick_window (), &mouse_ev);
  }

  /** Forwards X pointer motion to the offscreen QML header as a mouse move
   * (Qt Quick drives hover from plain mouse moves; coordinates are
   * converted from physical to logical pixels). */
  void forward_motion_event (int x, int y, int x_root, int y_root)
  {
    if (header_ == nullptr || !header_->is_valid ())
      return;
    const QPointF pos (x / scale_factor_, y / scale_factor_);
    last_hover_pos_ = pos;
    QMouseEvent move_ev (
      QEvent::MouseMove, pos, pos,
      QPointF (x_root / scale_factor_, y_root / scale_factor_), Qt::NoButton,
      button_pressed_ ? Qt::LeftButton : Qt::NoButton, Qt::NoModifier);
    QCoreApplication::sendEvent (header_->quick_window (), &move_ev);
  }

  /** Tells the offscreen QML header the pointer left the header strip. */
  void forward_leave_event ()
  {
    if (header_ == nullptr || !header_->is_valid ())
      return;
    const QPointF outside_header (-1, -1);
    QHoverEvent   leave_ev (
      QEvent::HoverLeave, outside_header, outside_header, last_hover_pos_);
    last_hover_pos_ = outside_header;
    QCoreApplication::sendEvent (header_->quick_window (), &leave_ev);
  }

  /**
   * @brief Repaints the header strip.
   *
   * Fills the background from the application palette and blits the latest
   * offscreen QML header frame with XPutImage.
   */
  void paint_header (Display &dpy)
  {
    if (header_win_ == 0 || header_ == nullptr)
      return;
    const auto w = physical_view_width ();
    const auto h = header_height_px ();
    if (w <= 0 || h <= 0)
      return;

    const auto palette = QGuiApplication::palette ();
    QImage     img (w, h, QImage::Format_RGB32);
    img.fill (palette.color (QPalette::Window));
    if (header_->is_valid ())
      {
        const auto frame = header_->grab_frame ();
        if (!frame.isNull ())
          {
            QPainter painter (&img);
            painter.drawImage (0, 0, frame);
          }
      }

    // The windows inherit the root's visual (CopyFromParent), so the blit
    // format is the default one. QImage::Format_RGB32 only matches a
    // 24/32-bit TrueColor visual; on anything else the header keeps its
    // plain background color instead of being painted with wrong pixels
    const auto   screen = DefaultScreen (&dpy);
    const auto   depth = DefaultDepth (&dpy, screen);
    auto * const visual = DefaultVisual (&dpy, screen);
    if (
      (depth != 24 && depth != 32) || visual->red_mask != 0xFF0000
      || visual->green_mask != 0xFF00 || visual->blue_mask != 0xFF)
      {
        if (!unsupported_visual_logged_)
          {
            unsupported_visual_logged_ = true;
            z_warning (
              "X11PluginHostWindow: cannot paint the header on a depth-{} "
              "visual with masks {:#x}/{:#x}/{:#x}",
              depth, visual->red_mask, visual->green_mask, visual->blue_mask);
          }
        return;
      }

    if (gc_ == nullptr)
      gc_ = XCreateGC (&dpy, header_win_, 0, nullptr);

    auto * ximg = XCreateImage (
      &dpy, visual, static_cast<unsigned> (depth), ZPixmap, 0,
      reinterpret_cast<char *> (img.bits ()), static_cast<unsigned> (w),
      static_cast<unsigned> (h), 32, static_cast<int> (img.bytesPerLine ()));
    if (ximg == nullptr)
      return;
    ximg->byte_order = LSBFirst;
    ximg->red_mask = 0xFF0000;
    ximg->green_mask = 0xFF00;
    ximg->blue_mask = 0xFF;
    XPutImage (
      &dpy, header_win_, gc_, ximg, 0, 0, 0, 0, static_cast<unsigned> (w),
      static_cast<unsigned> (h));
    // The pixel buffer is owned by the QImage
    ximg->data = nullptr;
    XDestroyImage (ximg);
    XFlush (&dpy);
  }

  /**
   * @brief Tells the window manager the size range the window accepts.
   *
   * A fixed-size window pins min and max to its current size, so the hints
   * have to follow every size and scale change rather than being set once.
   */
  void apply_size_hints (Display &dpy) const
  {
    XSizeHints hints{};
    hints.flags = PMinSize;
    // Bare minimum: the header strip with its controls
    hints.min_width =
      header_ != nullptr
        ? plugins::host_window_logical_to_physical (
            header_->controls_implicit_width (), scale_factor_)
        : 1;
    hints.min_height = header_height_px ();
    if (!resizable_)
      {
        hints.flags |= PMaxSize;
        hints.min_width = hints.max_width = physical_view_width ();
        hints.min_height = hints.max_height =
          physical_view_height () + header_height_px ();
      }
    XSetWMNormalHints (&dpy, win_, &hints);
  }

  /**
   * @brief Schedules a header repaint, coalescing bursts of requests (e.g.
   * pointer motion) into a single grab+blit per interval.
   */
  void schedule_header_repaint ();

  /**
   * @brief Sends an XEmbed focus message to the embedded client, if any.
   */
  void send_focus_to_client (long opcode, long detail);

  /**
   * @brief Sends an XEmbed focus message to the embedded client under an
   * X error trap.
   *
   * The client window is foreign and may vanish at any time; without a
   * trap, an X error on the dead window would abort the process.
   */
  void forward_focus_to_client_trapped (long opcode, long detail);

  ::Window win_ = 0;
  ::Window header_win_ = 0;
  ::Window embed_win_ = 0;
  GC       gc_ = nullptr;
  /** Offscreen QML header blitted onto the header strip. */
  utils::QObjectUniquePtr<OffscreenQmlHeader> header_;
  /** Whether the left mouse button is currently held over the header. */
  bool button_pressed_ = false;
  bool repaint_pending_ = false;
  /** Whether the unpaintable-visual warning was already logged. */
  bool unsupported_visual_logged_ = false;
  /** Last pointer position forwarded to the header, in logical pixels. */
  QPointF              last_hover_pos_{ -1, -1 };
  Atom                 wm_delete_ = None;
  Atom                 xembed_atom_ = None;
  X11PluginHostWindow &q_ptr_;
  /** Whether the user may resize the window (see apply_size_hints()). */
  bool  resizable_ = true;
  float scale_factor_ = 1.F;
  /** Plugin view size in logical pixels (per the PluginHostWindow
   * contract; physical sizes are derived via the scale factor). */
  int                     view_width_logical_ = 0;
  int                     view_height_logical_ = 0;
  std::optional<::Window> embedded_client_;
};

namespace
{

struct XFreeDeleter
{
  void operator() (void * ptr) const
  {
    if (ptr != nullptr)
      XFree (ptr);
  }
};

struct XrmDatabaseDeleter
{
  void operator() (XrmDatabase * db) const
  {
    if (db != nullptr)
      XrmDestroyDatabase (*db);
  }
};

struct XCloseDisplayDeleter
{
  void operator() (Display * dpy) const
  {
    if (dpy != nullptr)
      XCloseDisplay (dpy);
  }
};

// XEmbed message opcodes and details from the freedesktop XEmbed spec
constexpr long kEmbeddedNotify = 0;
constexpr long kWindowActivate = 1;
constexpr long kRequestFocus = 3;
constexpr long kFocusIn = 4;
constexpr long kFocusOut = 5;
constexpr long kFocusNext = 6;
constexpr long kFocusPrev = 7;
constexpr long kFocusCurrent = 0;
constexpr long kFocusFirst = 1;
constexpr long kFocusLast = 2;

/// Client-window wait budget for the XEmbed handshake (x 50ms between
/// attempts): generous for plugin UIs that are slow to map their window
constexpr int kEmbeddingRetryAttempts = 100;

/**
 * @brief Sends the EWMH client message asking the window manager to keep
 * the toplevel window above others (_NET_WM_STATE_ABOVE).
 *
 * Mapped windows request state changes with this message rather than by
 * setting _NET_WM_STATE directly: once a window is managed, the WM owns
 * that property, and the above state does not survive the window being
 * withdrawn (unmapped), so it must be re-requested after every map.
 */
void
request_above_state (Display &display, ::Window win)
{
  XClientMessageEvent msg{};
  msg.type = ClientMessage;
  msg.window = win;
  msg.message_type = XInternAtom (&display, "_NET_WM_STATE", False);
  msg.format = 32;
  msg.data.l[0] = 1; // _NET_WM_STATE_ADD
  msg.data.l[1] =
    static_cast<long> (XInternAtom (&display, "_NET_WM_STATE_ABOVE", False));
  msg.data.l[3] = 2; // source indication: normal application
  XSendEvent (
    &display, DefaultRootWindow (&display), False,
    SubstructureNotifyMask | SubstructureRedirectMask,
    reinterpret_cast<XEvent *> (&msg));
}

void
send_xembed_message (
  Display &display,
  Atom     xembed_atom,
  ::Window client,
  long     opcode,
  long     detail = 0,
  long     data1 = 0,
  long     data2 = 0)
{
  XClientMessageEvent msg{};
  msg.type = ClientMessage;
  msg.window = client;
  msg.message_type = xembed_atom;
  msg.format = 32;
  msg.data.l[0] = CurrentTime;
  msg.data.l[1] = opcode;
  msg.data.l[2] = detail;
  msg.data.l[3] = data1;
  msg.data.l[4] = data2;
  XSendEvent (
    &display, client, False, NoEventMask, reinterpret_cast<XEvent *> (&msg));
}

std::vector<::Window>
child_windows (Display &dpy, ::Window win)
{
  std::vector<::Window> result;
  ::Window              root = 0;
  ::Window              parent = 0;
  ::Window *            children_raw = nullptr;
  unsigned int          count = 0;
  if (XQueryTree (&dpy, win, &root, &parent, &children_raw, &count) == 0)
    return result;

  const std::unique_ptr<::Window[], XFreeDeleter> children (children_raw);
  if (children == nullptr)
    return result;

  result.reserve (count);
  for (const auto i : std::views::iota (0u, count))
    result.push_back (children[i]);
  return result;
}

/**
 * @brief Scoped X error trap for calls touching foreign windows.
 *
 * Xlib reports errors asynchronously and the default error handler aborts
 * the process; while a trap is active, errors about @p tolerated_window
 * are recorded instead and can be inspected via error_code(). Errors
 * about anything else are forwarded to the previously installed handler
 * — with XInitThreads, X errors from plugin toolkit threads also arrive
 * through this process-global handler, and swallowing them would hide
 * failures from their callers.
 *
 * Only used from the main thread. Not reentrant (asserted).
 */
class ScopedXErrorTrap
{
public:
  ScopedXErrorTrap (Display &dpy, ::Window tolerated_window)
      : dpy_ (dpy), tolerated_window_ (tolerated_window)
  {
    assert (active_trap_ == nullptr && "ScopedXErrorTrap is not reentrant");
    XSync (&dpy_, False);
    // Set before installing the handler: another thread's error can arrive
    // at the handler as soon as it is installed
    active_trap_ = this;
    previous_handler_ = XSetErrorHandler (&error_handler);
  }

  ~ScopedXErrorTrap ()
  {
    XSync (&dpy_, False);
    XSetErrorHandler (previous_handler_);
    active_trap_ = nullptr;
  }

  int error_code () const { return trapped_error_code_; }

private:
  static int error_handler (Display * dpy, XErrorEvent * ev)
  {
    auto * self = active_trap_;
    assert (self != nullptr);
    // Only swallow errors from our own connection on the tolerated window:
    // Xlib error handlers are process-global, so a plugin toolkit's error
    // on its own connection must not be mistaken for a handshake failure
    // (nor hidden from its own handler)
    if (dpy == &self->dpy_ && ev->resourceid == self->tolerated_window_)
      {
        self->trapped_error_code_ = ev->error_code;
        return 0;
      }
    // Not an error from the guarded calls (possibly another thread's):
    // forward to the previous handler. XSetErrorHandler() reports a null
    // previous handler when Xlib's built-in one is installed, which aborts
    // the process - log and continue instead
    if (self->previous_handler_ == nullptr)
      {
        std::array<char, 256> text{};
        XGetErrorText (
          dpy, ev->error_code, text.data (), static_cast<int> (text.size ()));
        z_warning (
          "X error {} on resource {:#x} (request {}.{}): {}", ev->error_code,
          ev->resourceid, ev->request_code, ev->minor_code, text.data ());
        return 0;
      }
    return self->previous_handler_ (dpy, ev);
  }

  Display &dpy_;
  ::Window tolerated_window_;
  int      trapped_error_code_ = Success;
  int (*previous_handler_) (Display *, XErrorEvent *) = nullptr;
  static inline ScopedXErrorTrap * active_trap_ = nullptr;
};

/**
 * @brief Process-wide X11 display connection shared by all plugin host
 * windows, pumped from Qt's event loop via QSocketNotifier.
 */
class X11DisplayManager final : public QObject
{
public:
  using EventHandler = std::function<void (const XEvent &)>;

  /**
   * @brief Returns the process-wide manager.
   *
   * This is an immortal singleton: the raw pointer is intentionally never
   * deleted so that no destructor ever runs. This avoids Qt shutdown issues
   * (the manager is a QObject owning a QSocketNotifier, and multiple
   * QCoreApplications may come and go per process) and destruction-order
   * issues (host windows may access the manager and its X connection from
   * late destructors).
   */
  static X11DisplayManager &instance ()
  {
#  ifdef ZRYTHM_LSAN_ACTIVE
    // intentional process-lifetime allocation - exempt from leak checking
    __lsan::ScopedDisabler disabler;
#  endif
    static auto * mgr = new X11DisplayManager ();
    mgr->rebind_notifier_to_current_app ();
    return *mgr;
  }

  Display * display () const { return dpy_.get (); }
  Atom      wm_delete_atom () const { return wm_delete_; }

  void register_window (::Window win, EventHandler handler)
  {
    windows_[win] = std::move (handler);
  }

  void unregister_window (::Window win) { windows_.erase (win); }

  /**
   * @brief Registers @p fn to be called when the root window's
   * RESOURCE_MANAGER property changes (e.g., Xft.dpi updated via xrdb),
   * for as long as @p context lives.
   */
  void
  add_resource_manager_listener (QObject * context, std::function<void ()> fn)
  {
    resource_manager_listeners_.emplace_back (context, std::move (fn));
  }

  /**
   * @brief Reads the Xft.dpi X resource as a scale factor (1.0 = 96 dpi).
   *
   * This is the scale the X11 environment expects X11 clients to render
   * at: compositors that downscale or upscale XWayland surfaces set it
   * accordingly (e.g., GNOME/mutter with fractional scaling advertises 2x,
   * presenting X11 windows at output_scale/2), and plain X11 sessions get
   * it from xrdb. Falls back to 1.0 (96 dpi) when unset.
   */
  static float query_scale_factor (Display &dpy)
  {
    static const bool xrm_initialized = [] {
      XrmInitialize ();
      return true;
    }();
    (void) xrm_initialized;

    // Read the root window's RESOURCE_MANAGER property directly (what xrdb
    // shows); XResourceManagerString() is cached at connection-open time and
    // owned by Xlib
    Atom            actual_type = None;
    int             actual_format = 0;
    unsigned long   nitems = 0;
    unsigned long   bytes_after = 0;
    unsigned char * prop = nullptr;
    const auto      status = XGetWindowProperty (
      &dpy, DefaultRootWindow (&dpy), XA_RESOURCE_MANAGER, 0, 0x7FFFFFFF, False,
      XA_STRING, &actual_type, &actual_format, &nitems, &bytes_after, &prop);
    const std::unique_ptr<char, XFreeDeleter> resource_manager (
      reinterpret_cast<char *> (prop));
    // Only a STRING/8 property is a valid X resource database
    if (
      status != Success || resource_manager == nullptr
      || actual_type != XA_STRING || actual_format != 8)
      {
        return 1.F;
      }
    auto * db = XrmGetStringDatabase (resource_manager.get ());
    if (db == nullptr)
      return 1.F;
    const std::unique_ptr<XrmDatabase, XrmDatabaseDeleter> db_guard (&db);

    XrmValue value{};
    char *   type = nullptr;
    if (
      XrmGetResource (db, "Xft.dpi", "Xft.Dpi", &type, &value)
      && value.addr != nullptr)
      {
        const auto dpi = std::strtof (value.addr, nullptr);
        if (dpi > 0.F)
          return dpi / 96.F;
      }
    return 1.F;
  }

private:
  X11DisplayManager () : dpy_ (open_display ())
  {
    if (dpy_ == nullptr)
      {
        z_warning ("X11PluginHostWindow: could not open X display");
        return;
      }
    wm_delete_ = XInternAtom (dpy_.get (), "WM_DELETE_WINDOW", False);

    // Watch for X resource changes (e.g., Xft.dpi) on the root window
    const auto root = DefaultRootWindow (dpy_.get ());
    XSelectInput (dpy_.get (), root, PropertyChangeMask);
    register_window (root, [this] (const XEvent &ev) {
      if (ev.type != PropertyNotify || ev.xproperty.atom != XA_RESOURCE_MANAGER)
        return;
      std::erase_if (resource_manager_listeners_, [] (const auto &listener) {
        return listener.first.isNull ();
      });
      for (const auto &[context, fn] : resource_manager_listeners_)
        fn ();
    });
  }

  /**
   * @brief Re-creates the X event notifier if the QCoreApplication
   * changed since it was created.
   *
   * A QSocketNotifier stays bound to the event dispatcher that was
   * current at its construction, so a new QCoreApplication installing a
   * new dispatcher on the same thread leaves it dead. This does not
   * happen in production (one QApplication per process) — only in tests,
   * which create one QCoreApplication per fixture in a shared process.
   */
  void rebind_notifier_to_current_app ()
  {
    auto * app = QCoreApplication::instance ();
    if (dpy_ == nullptr || app == bound_app_)
      return;
    bound_app_ = app;
    notifier_.reset ();
    if (app == nullptr)
      return;
    notifier_ = utils::make_qobject_unique<QSocketNotifier> (
      ConnectionNumber (dpy_.get ()), QSocketNotifier::Read, this);
    connect (notifier_.get (), &QSocketNotifier::activated, this, [this] {
      drain_events ();
    });
  }

  static Display * open_display ()
  {
    // Plugin toolkits may call Xlib from their own threads; locking must be
    // enabled before the first connection is opened
    static std::once_flag threads_init_flag;
    std::call_once (threads_init_flag, [] { XInitThreads (); });
    return XOpenDisplay (nullptr);
  }

  void drain_events ()
  {
    while (XPending (dpy_.get ()) > 0)
      {
        XEvent ev{};
        XNextEvent (dpy_.get (), &ev);
        dispatch (ev);
      }
  }

  void dispatch (const XEvent &ev)
  {
    const auto it = windows_.find (ev.xany.window);
    if (it == windows_.end ())
      return;
    // Handlers may unregister themselves (or other windows) while running
    const auto fn = it->second;
    fn (ev);
  }

private:
  std::unique_ptr<Display, XCloseDisplayDeleter> dpy_;
  Atom                                           wm_delete_ = None;
  utils::QObjectUniquePtr<QSocketNotifier>       notifier_;

  /** QCoreApplication the notifier is registered with. Tracked via
   * QPointer (not a raw address) so a new application allocated at a
   * destroyed one's address is still detected as a change. */
  QPointer<const QCoreApplication>           bound_app_;
  std::unordered_map<::Window, EventHandler> windows_;
  std::vector<std::pair<QPointer<QObject>, std::function<void ()>>>
    resource_manager_listeners_;
};

/**
 * @brief Scale factor for the X11 window, from the X11 environment.
 *
 * This window class is only used under Wayland sessions, where the raw X11
 * window is an XWayland surface. Its presentation scale is governed by the
 * X11 side, not the Wayland side: compositors tell X11 clients which scale
 * to render at via Xft.dpi (e.g., GNOME/mutter with fractional scaling
 * advertises 2x and presents X11 surfaces downscaled to the output scale),
 * so the X resource database is the correct cross-DE source.
 */
static float
x11_scale_factor ()
{
  auto * dpy = X11DisplayManager::instance ().display ();
  if (dpy == nullptr)
    return 1.F;
  return X11DisplayManager::query_scale_factor (*dpy);
}

} // namespace

void
X11PluginHostWindow::Impl::send_focus_to_client (long opcode, long detail)
{
  if (!embedded_client_.has_value () || xembed_atom_ == None)
    return;
  auto * d = X11DisplayManager::instance ().display ();
  if (d == nullptr)
    return;
  send_xembed_message (*d, xembed_atom_, *embedded_client_, opcode, detail);
  XFlush (d);
}

void
X11PluginHostWindow::Impl::forward_focus_to_client_trapped (
  long opcode,
  long detail)
{
  if (!embedded_client_.has_value ())
    return;
  auto * d = X11DisplayManager::instance ().display ();
  if (d == nullptr)
    return;
  const ScopedXErrorTrap trap (*d, *embedded_client_);
  send_focus_to_client (opcode, detail);
}

void
X11PluginHostWindow::Impl::schedule_header_repaint ()
{
  if (repaint_pending_)
    return;
  repaint_pending_ = true;
  QTimer::singleShot (std::chrono::milliseconds{ 16 }, &q_ptr_, [this] {
    repaint_pending_ = false;
    auto * d = X11DisplayManager::instance ().display ();
    if (d != nullptr)
      paint_header (*d);
  });
}

void
X11PluginHostWindow::Impl::complete_native_embedding (
  Display &dpy,
  int      attempts_remaining)
{
  if (embedded_client_.has_value ())
    return;

  // The embed area is created by us and starts out empty, so any child is
  // the plugin's client window
  const auto children = child_windows (dpy, embed_win_);
  if (!children.empty ())
    embedded_client_ = children.front ();

  if (!embedded_client_.has_value ())
    {
      if (attempts_remaining > 0)
        {
          QTimer::singleShot (
            std::chrono::milliseconds{ 50 }, &q_ptr_,
            [this, &dpy, attempts_remaining] {
              complete_native_embedding (dpy, attempts_remaining - 1);
            });
        }
      else
        {
          z_warning ("X11PluginHostWindow: plugin client window did not appear");
          q_ptr_.setVisible (false);
          Q_EMIT q_ptr_.embeddingFailed ();
          // Listeners may queue the window's destruction - do not touch
          // members from here on
        }
      return;
    }

  if (xembed_atom_ == None)
    xembed_atom_ = XInternAtom (&dpy, "_XEMBED", False);

  // The client window is foreign and may vanish mid-handshake
  const ScopedXErrorTrap trap (dpy, *embedded_client_);

  // EMBEDDED_NOTIFY: data1 = embedder window, data2 = protocol version (0 is
  // the only version)
  send_xembed_message (
    dpy, xembed_atom_, *embedded_client_, kEmbeddedNotify, 0,
    static_cast<long> (embed_win_), 0);
  send_xembed_message (dpy, xembed_atom_, *embedded_client_, kWindowActivate);
  send_xembed_message (
    dpy, xembed_atom_, *embedded_client_, kFocusIn, kFocusCurrent);
  XSync (&dpy, False);

  if (trap.error_code () != Success)
    {
      z_warning (
        "X11PluginHostWindow: X error {} during embedding handshake",
        trap.error_code ());
      embedded_client_.reset ();
      q_ptr_.setVisible (false);
      Q_EMIT q_ptr_.embeddingFailed ();
      // Listeners may queue the window's destruction - do not touch
      // members from here on
      return;
    }

  // The client window is foreign and may be destroyed or reparented away
  // by the plugin at any time: stop focus forwarding when that happens,
  // or requests on the dead window would raise X errors that abort the
  // process (SubstructureNotifyMask is selected on the embed area, so its
  // children's DestroyNotify/ReparentNotify are dispatched under the
  // client's own window ID)
  X11DisplayManager::instance ().register_window (
    *embedded_client_, [this] (const XEvent &ev) {
      if (ev.type != DestroyNotify && ev.type != ReparentNotify)
        return;
      z_debug ("X11PluginHostWindow: embedded client window went away");
      X11DisplayManager::instance ().unregister_window (ev.xany.window);
      embedded_client_.reset ();
    });
}

X11PluginHostWindow::X11PluginHostWindow (plugins::Plugin &plugin)
    : plugins::PluginHostWindow (plugin), pimpl_ (std::make_unique<Impl> (*this))
{
  auto  &mgr = X11DisplayManager::instance ();
  auto * dpy = mgr.display ();
  if (dpy == nullptr)
    return;

  pimpl_->wm_delete_ = mgr.wm_delete_atom ();
  pimpl_->scale_factor_ = x11_scale_factor ();

  // Offscreen QML header, blitted by paint_header() and driven by
  // forwarding X events as Qt events to its scene. Created before the X
  // windows so sizes can be derived from the QML scene
  pimpl_->header_ =
    utils::make_qobject_unique<OffscreenQmlHeader> (this->plugin (), this);

  pimpl_->view_width_logical_ = 640;
  pimpl_->view_height_logical_ =
    std::max (1, 480 - pimpl_->header_->implicit_height ());

  pimpl_->win_ = XCreateWindow (
    dpy, DefaultRootWindow (dpy), 0, 0,
    static_cast<unsigned> (pimpl_->physical_view_width ()),
    static_cast<unsigned> (
      pimpl_->physical_view_height () + pimpl_->header_height_px ()),
    0, CopyFromParent, InputOutput, CopyFromParent, 0, nullptr);

  // Header strip (host chrome, colored from the application palette) and
  // the embed area below it
  const auto header_background =
    QGuiApplication::palette ().color (QPalette::Window).rgb () & 0xFFFFFF;
  pimpl_->header_win_ = XCreateSimpleWindow (
    dpy, pimpl_->win_, 0, 0,
    static_cast<unsigned> (pimpl_->physical_view_width ()),
    static_cast<unsigned> (pimpl_->header_height_px ()), 0, 0,
    header_background);
  XSelectInput (
    dpy, pimpl_->header_win_,
    ExposureMask | ButtonPressMask | ButtonReleaseMask | PointerMotionMask
      | LeaveWindowMask);
  pimpl_->embed_win_ = XCreateSimpleWindow (
    dpy, pimpl_->win_, 0, pimpl_->header_height_px (),
    static_cast<unsigned> (pimpl_->physical_view_width ()),
    static_cast<unsigned> (pimpl_->physical_view_height ()), 0, 0, 0);
  // Lifetime of the embedded client (a child of the embed area) is tracked
  // via these events; see complete_native_embedding()
  XSelectInput (dpy, pimpl_->embed_win_, SubstructureNotifyMask);
  XMapWindow (dpy, pimpl_->header_win_);
  XMapWindow (dpy, pimpl_->embed_win_);

  pimpl_->header_->set_device_pixel_ratio (pimpl_->scale_factor_);
  pimpl_->header_->resize (
    pimpl_->view_width_logical_, pimpl_->header_->implicit_height ());
  connect (
    pimpl_->header_.get (), &OffscreenQmlHeader::repaintNeeded, this, [this] {
      auto * d = X11DisplayManager::instance ().display ();
      if (d != nullptr)
        pimpl_->paint_header (*d);
    });
  connect (
    pimpl_->header_.get (), &OffscreenQmlHeader::implicitHeightChanged, this,
    [this] {
      auto * d = X11DisplayManager::instance ().display ();
      if (d == nullptr || pimpl_->win_ == 0)
        return;
      // Keep the plugin view size, resizing the toplevel around the new
      // header height
      pimpl_->apply_size_hints (*d);
      XResizeWindow (
        d, pimpl_->win_, static_cast<unsigned> (pimpl_->physical_view_width ()),
        static_cast<unsigned> (
          pimpl_->physical_view_height () + pimpl_->header_height_px ()));
      pimpl_->layout_children (
        *d, pimpl_->view_width_logical_, pimpl_->view_height_logical_);
      pimpl_->paint_header (*d);
      XFlush (d);
    });

  mgr.register_window (pimpl_->header_win_, [this] (const XEvent &ev) {
    auto * d = X11DisplayManager::instance ().display ();
    if (d == nullptr)
      return;
    switch (ev.type)
      {
      case Expose:
        if (ev.xexpose.count == 0)
          pimpl_->paint_header (*d);
        break;
      case ButtonPress:
      case ButtonRelease:
        if (ev.xbutton.button == Button1)
          {
            pimpl_->forward_button_event (ev);
            pimpl_->paint_header (*d);
          }
        break;
      case MotionNotify:
        pimpl_->forward_motion_event (
          ev.xmotion.x, ev.xmotion.y, ev.xmotion.x_root, ev.xmotion.y_root);
        pimpl_->schedule_header_repaint ();
        break;
      case LeaveNotify:
        pimpl_->forward_leave_event ();
        pimpl_->paint_header (*d);
        break;
      default:
        break;
      }
  });

  // Repaint the header when the state shown in it changes
  connect (&this->plugin (), &plugins::Plugin::bypassedChanged, this, [this] {
    auto * d = X11DisplayManager::instance ().display ();
    if (d != nullptr)
      pimpl_->paint_header (*d);
  });
  connect (&this->plugin (), &plugins::Plugin::abActiveChanged, this, [this] {
    auto * d = X11DisplayManager::instance ().display ();
    if (d != nullptr)
      pimpl_->paint_header (*d);
  });

  connect (this, &plugins::PluginHostWindow::titleChanged, this, [this] {
    auto * d = X11DisplayManager::instance ().display ();
    if (d != nullptr && pimpl_->win_ != 0)
      {
        const auto title_str = title ().toUtf8 ();
        // WM_NAME (XStoreName) is latin-1 only; _NET_WM_NAME carries the
        // UTF-8 title for window managers that read it
        XStoreName (d, pimpl_->win_, title_str.constData ());
        const auto net_wm_name = XInternAtom (d, "_NET_WM_NAME", False);
        const auto utf8_string = XInternAtom (d, "UTF8_STRING", False);
        XChangeProperty (
          d, pimpl_->win_, net_wm_name, utf8_string, 8, PropModeReplace,
          reinterpret_cast<const unsigned char *> (title_str.constData ()),
          static_cast<int> (title_str.size ()));
        XFlush (d);
      }
  });

  // Group with the application (matches StartupWMClass in the desktop entry)
  XClassHint class_hint{};
  class_hint.res_name = const_cast<char *> ("zrythm");
  class_hint.res_class = const_cast<char *> ("zrythm");
  XSetClassHint (dpy, pimpl_->win_, &class_hint);

  // Keep plugin windows above the main window: this property is the
  // initial-state hint read when the window is first mapped. WMs drop the
  // above state when the window is withdrawn, so setVisible() re-requests
  // it via a _NET_WM_STATE client message on every map
  const auto net_wm_state = XInternAtom (dpy, "_NET_WM_STATE", False);
  const auto above = XInternAtom (dpy, "_NET_WM_STATE_ABOVE", False);
  XChangeProperty (
    dpy, pimpl_->win_, net_wm_state, XA_ATOM, 32, PropModeReplace,
    reinterpret_cast<const unsigned char *> (&above), 1);

  const auto net_wm_window_type =
    XInternAtom (dpy, "_NET_WM_WINDOW_TYPE", False);
  const auto net_wm_window_type_normal =
    XInternAtom (dpy, "_NET_WM_WINDOW_TYPE_NORMAL", False);
  XChangeProperty (
    dpy, pimpl_->win_, net_wm_window_type, XA_ATOM, 32, PropModeReplace,
    reinterpret_cast<const unsigned char *> (&net_wm_window_type_normal), 1);

  XSetWMProtocols (dpy, pimpl_->win_, &pimpl_->wm_delete_, 1);
  XSelectInput (
    dpy, pimpl_->win_,
    StructureNotifyMask | SubstructureNotifyMask | FocusChangeMask);

  const auto wm_protocols = XInternAtom (dpy, "WM_PROTOCOLS", False);
  mgr.register_window (pimpl_->win_, [this, wm_protocols] (const XEvent &ev) {
    if (
      ev.type == ClientMessage
      && static_cast<Atom> (ev.xclient.message_type) == wm_protocols
      && static_cast<Atom> (ev.xclient.data.l[0]) == pimpl_->wm_delete_)
      {
        Q_EMIT closeRequested ();
        return;
      }
    if (
      (ev.type == FocusIn || ev.type == FocusOut)
      && ev.xfocus.mode != NotifyGrab && ev.xfocus.mode != NotifyUngrab)
      {
        // Forward toplevel focus transitions to the embedded client, or it
        // never learns about focus changes after the initial handshake
        pimpl_->forward_focus_to_client_trapped (
          ev.type == FocusIn ? kFocusIn : kFocusOut, kFocusCurrent);
        return;
      }
    if (
      ev.type == ConfigureNotify && ev.xconfigure.window == pimpl_->win_
      && ev.xconfigure.event == pimpl_->win_)
      {
        // Track external (WM-initiated) resizes of the toplevel window,
        // converting the physical toplevel size to the logical view size
        // (SubstructureNotifyMask also delivers ConfigureNotify for child
        // windows - those are not toplevel resizes)
        const auto scale = pimpl_->scale_factor_;
        const auto new_view_width = std::max (
          1,
          static_cast<int> (
            std::lround (static_cast<float> (ev.xconfigure.width) / scale)));
        const auto new_view_height = std::max (
          1,
          static_cast<int> (std::lround (
            static_cast<float> (ev.xconfigure.height - pimpl_->header_height_px ())
            / scale)));
        if (
          new_view_width != pimpl_->view_width_logical_
          || new_view_height != pimpl_->view_height_logical_)
          {
            pimpl_->view_width_logical_ = new_view_width;
            pimpl_->view_height_logical_ = new_view_height;
            auto * d = X11DisplayManager::instance ().display ();
            if (d != nullptr)
              {
                pimpl_->layout_children (
                  *d, pimpl_->view_width_logical_, pimpl_->view_height_logical_);
                pimpl_->paint_header (*d);
              }
            Q_EMIT embedSizeChanged (new_view_width, new_view_height);
          }
      }
  });

  // XEmbed client->embedder requests (focus management) are addressed to
  // the embed area, the client window's parent
  mgr.register_window (pimpl_->embed_win_, [this] (const XEvent &ev) {
    if (
      ev.type != ClientMessage || pimpl_->xembed_atom_ == None
      || static_cast<Atom> (ev.xclient.message_type) != pimpl_->xembed_atom_
      || !pimpl_->embedded_client_.has_value ())
      return;
    auto * d = X11DisplayManager::instance ().display ();
    if (d == nullptr)
      return;
    switch (ev.xclient.data.l[1])
      {
      case kRequestFocus:
        // The client window is foreign and may vanish at any time
        {
          const ScopedXErrorTrap trap (*d, *pimpl_->embedded_client_);
          XSetInputFocus (
            d, *pimpl_->embedded_client_, RevertToParent, CurrentTime);
          pimpl_->send_focus_to_client (kFocusIn, kFocusCurrent);
        }
        break;
      case kFocusNext:
      case kFocusPrev:
        // The host window has no other focusable widgets (the header is
        // passive chrome): wrap focus back into the client
        pimpl_->forward_focus_to_client_trapped (
          kFocusIn,
          ev.xclient.data.l[1] == kFocusNext ? kFocusFirst : kFocusLast);
        break;
      default:
        break;
      }
  });

  // Re-query the X11 scale when X resources change
  mgr.add_resource_manager_listener (this, [this] { refresh_scale_factor (); });
  XFlush (dpy);
}

X11PluginHostWindow::~X11PluginHostWindow ()
{
  auto  &mgr = X11DisplayManager::instance ();
  auto * dpy = mgr.display ();
  if (dpy != nullptr && pimpl_->win_ != 0)
    {
      mgr.unregister_window (pimpl_->win_);
      mgr.unregister_window (pimpl_->header_win_);
      mgr.unregister_window (pimpl_->embed_win_);
      // Destroying the toplevel recursively destroys the embedded client,
      // whose events would otherwise dispatch to a dangling handler
      if (pimpl_->embedded_client_.has_value ())
        mgr.unregister_window (*pimpl_->embedded_client_);
      if (pimpl_->gc_ != nullptr)
        XFreeGC (dpy, pimpl_->gc_);
      XDestroyWindow (dpy, pimpl_->win_);
      XFlush (dpy);
    }
}

bool
X11PluginHostWindow::is_valid () const
{
  return pimpl_->win_ != 0;
}

void
X11PluginHostWindow::setSizeAndCenter (int width, int height)
{
  auto * dpy = X11DisplayManager::instance ().display ();
  if (dpy == nullptr || pimpl_->win_ == 0)
    return;

  // Plugin view sizes are validated at the plugin boundary
  assert (width > 0 && height > 0);
  pimpl_->view_width_logical_ = width;
  pimpl_->view_height_logical_ = height;
  const auto physical_width = pimpl_->physical_view_width ();
  const auto total_height =
    pimpl_->physical_view_height () + pimpl_->header_height_px ();
  const auto screen = DefaultScreen (dpy);
  const auto x = std::max (0, (DisplayWidth (dpy, screen) - physical_width) / 2);
  const auto y = std::max (0, (DisplayHeight (dpy, screen) - total_height) / 2);
  pimpl_->apply_size_hints (*dpy);
  XMoveResizeWindow (
    dpy, pimpl_->win_, x, y, static_cast<unsigned> (physical_width),
    static_cast<unsigned> (total_height));
  pimpl_->layout_children (
    *dpy, pimpl_->view_width_logical_, pimpl_->view_height_logical_);
  pimpl_->paint_header (*dpy);
  XFlush (dpy);
}

void
X11PluginHostWindow::setSize (int width, int height)
{
  auto * dpy = X11DisplayManager::instance ().display ();
  if (dpy == nullptr || pimpl_->win_ == 0)
    return;
  // Plugin view sizes are validated at the plugin boundary
  assert (width > 0 && height > 0);
  pimpl_->view_width_logical_ = width;
  pimpl_->view_height_logical_ = height;
  pimpl_->apply_size_hints (*dpy);
  XResizeWindow (
    dpy, pimpl_->win_, static_cast<unsigned> (pimpl_->physical_view_width ()),
    static_cast<unsigned> (
      pimpl_->physical_view_height () + pimpl_->header_height_px ()));
  pimpl_->layout_children (
    *dpy, pimpl_->view_width_logical_, pimpl_->view_height_logical_);
  pimpl_->paint_header (*dpy);
  XFlush (dpy);
}

void
X11PluginHostWindow::setResizable (bool resizable)
{
  auto * dpy = X11DisplayManager::instance ().display ();
  if (dpy == nullptr || pimpl_->win_ == 0)
    return;

  pimpl_->resizable_ = resizable;
  pimpl_->apply_size_hints (*dpy);
  XFlush (dpy);
}

void
X11PluginHostWindow::setVisible (bool shouldBeVisible)
{
  auto * dpy = X11DisplayManager::instance ().display ();
  if (dpy == nullptr || pimpl_->win_ == 0)
    return;
  if (shouldBeVisible)
    {
      // The compositor scale may have changed since the window was created
      refresh_scale_factor ();
      XMapWindow (dpy, pimpl_->win_);
      request_above_state (*dpy, pimpl_->win_);
      // Mapping does not reliably generate an Expose event under composited
      // X servers (contents are considered preserved), and the paints issued
      // while the window was unmapped are discarded, so repaint explicitly
      pimpl_->paint_header (*dpy);
    }
  else
    {
      XUnmapWindow (dpy, pimpl_->win_);
    }
  XFlush (dpy);
}

void
X11PluginHostWindow::refresh_scale_factor ()
{
  auto * dpy = X11DisplayManager::instance ().display ();
  if (dpy == nullptr || pimpl_->win_ == 0)
    return;

  const auto new_scale = x11_scale_factor ();
  if (new_scale == pimpl_->scale_factor_)
    return;

  pimpl_->scale_factor_ = new_scale;
  pimpl_->apply_size_hints (*dpy);
  XResizeWindow (
    dpy, pimpl_->win_, static_cast<unsigned> (pimpl_->physical_view_width ()),
    static_cast<unsigned> (
      pimpl_->physical_view_height () + pimpl_->header_height_px ()));
  pimpl_->layout_children (
    *dpy, pimpl_->view_width_logical_, pimpl_->view_height_logical_);
  pimpl_->paint_header (*dpy);
  XFlush (dpy);
  Q_EMIT contentScaleFactorChanged (new_scale);
}

WId
X11PluginHostWindow::getEmbedWindowId () const
{
  return static_cast<WId> (pimpl_->embed_win_);
}

float
X11PluginHostWindow::contentScaleFactor () const
{
  return pimpl_->scale_factor_;
}

void
X11PluginHostWindow::completeNativeEmbedding ()
{
  auto * dpy = X11DisplayManager::instance ().display ();
  if (dpy == nullptr || pimpl_->win_ == 0)
    return;
  pimpl_->complete_native_embedding (*dpy, kEmbeddingRetryAttempts);
}

} // namespace zrythm::gui

#else // !__linux__

namespace zrythm::gui
{

class X11PluginHostWindow::Impl
{
public:
  explicit Impl (X11PluginHostWindow &) { }
};

X11PluginHostWindow::X11PluginHostWindow (plugins::Plugin &plugin)
    : plugins::PluginHostWindow (plugin), pimpl_ (std::make_unique<Impl> (*this))
{
}

X11PluginHostWindow::~X11PluginHostWindow () = default;

bool
X11PluginHostWindow::is_valid () const
{
  return false;
}
void
X11PluginHostWindow::setSizeAndCenter (int, int)
{
}
void
X11PluginHostWindow::setSize (int, int)
{
}
void
X11PluginHostWindow::setResizable (bool)
{
}
void
X11PluginHostWindow::setVisible (bool)
{
}
void
X11PluginHostWindow::refresh_scale_factor ()
{
}
WId
X11PluginHostWindow::getEmbedWindowId () const
{
  return 0;
}
float
X11PluginHostWindow::contentScaleFactor () const
{
  return 1.F;
}
void
X11PluginHostWindow::completeNativeEmbedding ()
{
}

} // namespace zrythm::gui

#endif // __linux__
