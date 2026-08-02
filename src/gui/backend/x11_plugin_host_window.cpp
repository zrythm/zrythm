// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/backend/plugin_header_qml.h"
#include "gui/backend/x11_plugin_host_window.h"
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

#  include <charconv>
#  include <chrono>
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
    return std::lround (static_cast<float> (kHeaderHeight) * scale_factor_);
  }

  /** Sizes and positions the header strip and embed area for the given
   * plugin view size. */
  void layout_children (Display &dpy, int view_width, int view_height) const
  {
    const auto hh = static_cast<unsigned> (header_height_px ());
    XResizeWindow (&dpy, header_win_, static_cast<unsigned> (view_width), hh);
    XMoveResizeWindow (
      &dpy, embed_win_, 0, static_cast<int> (hh),
      static_cast<unsigned> (view_width), static_cast<unsigned> (view_height));
    if (header_ != nullptr)
      header_->resize (view_width, static_cast<int> (hh));
  }

  /**
   * @brief Forwards an X button event to the offscreen QML header as a
   * QMouseEvent.
   */
  void forward_button_event (const XEvent &ev)
  {
    if (header_ == nullptr || !header_->is_valid ())
      return;
    const bool press = ev.type == ButtonPress;
    button_pressed_ = press;
    const QPointF pos (ev.xbutton.x, ev.xbutton.y);
    const QPointF global_pos (ev.xbutton.x_root, ev.xbutton.y_root);
    QMouseEvent   mouse_ev (
      press ? QEvent::MouseButtonPress : QEvent::MouseButtonRelease, pos, pos,
      global_pos, Qt::LeftButton, press ? Qt::LeftButton : Qt::NoButton,
      Qt::NoModifier);
    QCoreApplication::sendEvent (header_->widget (), &mouse_ev);
  }

  /** Forwards X pointer motion to the offscreen QML header as a mouse move
   * (Qt Quick drives hover from plain mouse moves; QQuickWidget does not
   * forward QHoverEvent). */
  void forward_motion_event (int x, int y, int x_root, int y_root)
  {
    if (header_ == nullptr || !header_->is_valid ())
      return;
    const QPointF pos (x, y);
    QMouseEvent   move_ev (
      QEvent::MouseMove, pos, pos, QPointF (x_root, y_root), Qt::NoButton,
      button_pressed_ ? Qt::LeftButton : Qt::NoButton, Qt::NoModifier);
    QCoreApplication::sendEvent (header_->widget (), &move_ev);
  }

  /** Tells the offscreen QML header the pointer left the header strip. */
  void forward_leave_event ()
  {
    if (header_ == nullptr || !header_->is_valid ())
      return;
    QHoverEvent leave_ev (QEvent::HoverLeave, QPointF (-1, -1), last_hover_pos_);
    last_hover_pos_ = QPointF (-1, -1);
    QCoreApplication::sendEvent (header_->widget ()->quickWindow (), &leave_ev);
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
    const auto w = view_width_;
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

    if (gc_ == nullptr)
      gc_ = XCreateGC (&dpy, header_win_, 0, nullptr);

    auto * ximg = XCreateImage (
      &dpy, DefaultVisual (&dpy, DefaultScreen (&dpy)), 24, ZPixmap, 0,
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

  ::Window win_ = 0;
  ::Window header_win_ = 0;
  ::Window embed_win_ = 0;
  GC       gc_ = nullptr;
  /** Offscreen QML header blitted onto the header strip. */
  std::unique_ptr<OffscreenQmlHeader> header_;
  /** Whether the left mouse button is currently held over the header. */
  bool                    button_pressed_ = false;
  QPointF                 last_hover_pos_{ -1, -1 };
  Atom                    wm_delete_ = None;
  Atom                    xembed_atom_ = None;
  X11PluginHostWindow    &q_ptr_;
  float                   scale_factor_ = 1.F;
  int                     view_width_ = 0;
  int                     view_height_ = 0;
  std::vector<::Window>   initial_children_;
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
constexpr long kFocusIn = 4;
constexpr long kFocusCurrent = 0;

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
   * @brief Reads the Xft.dpi X resource as a scale factor (1.0 = 96 dpi).
   */
  static float query_scale_factor (Display &dpy)
  {
    static const bool xrm_initialized = [] {
      XrmInitialize ();
      return true;
    }();
    (void) xrm_initialized;

    if (auto * db = XrmGetDatabase (&dpy))
      {
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
      }
    return 1.F;
  }

private:
  X11DisplayManager () : dpy_ (XOpenDisplay (nullptr))
  {
    if (dpy_ == nullptr)
      {
        z_warning ("X11PluginHostWindow: could not open X display");
        return;
      }
    wm_delete_ = XInternAtom (dpy_.get (), "WM_DELETE_WINDOW", False);

    notifier_ = utils::make_qobject_unique<QSocketNotifier> (
      ConnectionNumber (dpy_.get ()), QSocketNotifier::Read, this);
    connect (notifier_.get (), &QSocketNotifier::activated, this, [this] {
      drain_events ();
    });
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
    ::Window target = ev.xany.window;
    if (ev.type == ReparentNotify)
      target = ev.xreparent.parent;

    const auto it = windows_.find (target);
    if (it == windows_.end ())
      return;
    it->second (ev);
  }

private:
  std::unique_ptr<Display, XCloseDisplayDeleter> dpy_;
  Atom                                           wm_delete_ = None;
  utils::QObjectUniquePtr<QSocketNotifier>       notifier_;
  std::unordered_map<::Window, EventHandler>     windows_;
};

} // namespace

void
X11PluginHostWindow::Impl::complete_native_embedding (
  Display &dpy,
  int      attempts_remaining)
{
  if (embedded_client_.has_value ())
    return;

  for (const auto child : child_windows (dpy, embed_win_))
    {
      if (
        std::ranges::find (initial_children_, child) == initial_children_.end ())
        {
          embedded_client_ = child;
          break;
        }
    }

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
        }
      return;
    }

  if (xembed_atom_ == None)
    xembed_atom_ = XInternAtom (&dpy, "_XEMBED", False);

  // EMBEDDED_NOTIFY: data1 = embedder window, data2 = protocol version (0 is
  // the only version)
  send_xembed_message (
    dpy, xembed_atom_, *embedded_client_, kEmbeddedNotify, 0,
    static_cast<long> (embed_win_), 0);
  send_xembed_message (dpy, xembed_atom_, *embedded_client_, kWindowActivate);
  send_xembed_message (
    dpy, xembed_atom_, *embedded_client_, kFocusIn, kFocusCurrent);
  XSync (&dpy, False);
}

X11PluginHostWindow::X11PluginHostWindow (plugins::Plugin &plugin)
    : plugins::PluginHostWindow (plugin), pimpl_ (std::make_unique<Impl> (*this))
{
  auto  &mgr = X11DisplayManager::instance ();
  auto * dpy = mgr.display ();
  if (dpy == nullptr)
    return;

  pimpl_->wm_delete_ = mgr.wm_delete_atom ();
  pimpl_->scale_factor_ = X11DisplayManager::query_scale_factor (*dpy);

  pimpl_->win_ = XCreateWindow (
    dpy, DefaultRootWindow (dpy), 0, 0, 640, 480, 0, CopyFromParent,
    InputOutput, CopyFromParent, 0, nullptr);
  pimpl_->view_width_ = 640;
  pimpl_->view_height_ = 480 - pimpl_->header_height_px ();

  // Header strip (host chrome, colored from the application palette) and
  // the embed area below it
  const auto header_background =
    QGuiApplication::palette ().color (QPalette::Window).rgb () & 0xFFFFFF;
  pimpl_->header_win_ = XCreateSimpleWindow (
    dpy, pimpl_->win_, 0, 0, 640,
    static_cast<unsigned> (pimpl_->header_height_px ()), 0, 0,
    header_background);
  XSelectInput (
    dpy, pimpl_->header_win_,
    ExposureMask | ButtonPressMask | ButtonReleaseMask | PointerMotionMask
      | LeaveWindowMask);
  pimpl_->embed_win_ = XCreateSimpleWindow (
    dpy, pimpl_->win_, 0, pimpl_->header_height_px (), 640,
    static_cast<unsigned> (480 - pimpl_->header_height_px ()), 0, 0, 0);
  XMapWindow (dpy, pimpl_->header_win_);
  XMapWindow (dpy, pimpl_->embed_win_);

  // Offscreen QML header, blitted by paint_header() and driven by
  // forwarding X events as Qt events to its scene
  pimpl_->header_ = std::make_unique<OffscreenQmlHeader> (*this, this);
  pimpl_->header_->resize (640, pimpl_->header_height_px ());
  connect (
    pimpl_->header_.get (), &OffscreenQmlHeader::repaintNeeded, this, [this] {
      auto * d = X11DisplayManager::instance ().display ();
      if (d != nullptr)
        pimpl_->paint_header (*d);
    });

  mgr.register_window (pimpl_->header_win_, [this] (const XEvent &ev) {
    auto * d = X11DisplayManager::instance ().display ();
    if (d == nullptr)
      return;
    bool needs_repaint = false;
    switch (ev.type)
      {
      case Expose:
        needs_repaint = ev.xexpose.count == 0;
        break;
      case ButtonPress:
      case ButtonRelease:
        if (ev.xbutton.button == Button1)
          {
            pimpl_->forward_button_event (ev);
            needs_repaint = true;
          }
        break;
      case MotionNotify:
        pimpl_->forward_motion_event (
          ev.xmotion.x, ev.xmotion.y, ev.xmotion.x_root, ev.xmotion.y_root);
        needs_repaint = true;
        break;
      case LeaveNotify:
        pimpl_->forward_leave_event ();
        needs_repaint = true;
        break;
      default:
        break;
      }
    if (needs_repaint)
      pimpl_->paint_header (*d);
  });

  // Repaint the header when the state shown in it changes
  connect (this, &plugins::PluginHostWindow::bypassedChanged, this, [this] {
    auto * d = X11DisplayManager::instance ().display ();
    if (d != nullptr)
      pimpl_->paint_header (*d);
  });
  connect (this, &plugins::PluginHostWindow::abStateChanged, this, [this] {
    auto * d = X11DisplayManager::instance ().display ();
    if (d != nullptr)
      pimpl_->paint_header (*d);
  });

  connect (this, &plugins::PluginHostWindow::titleChanged, this, [this] {
    auto * d = X11DisplayManager::instance ().display ();
    if (d != nullptr && pimpl_->win_ != 0)
      {
        const auto title_str = title ().toUtf8 ();
        XStoreName (d, pimpl_->win_, title_str.constData ());
        XFlush (d);
      }
  });

  // Group with the application (matches StartupWMClass in the desktop entry)
  XClassHint class_hint{};
  class_hint.res_name = const_cast<char *> ("zrythm");
  class_hint.res_class = const_cast<char *> ("zrythm");
  XSetClassHint (dpy, pimpl_->win_, &class_hint);

  // Keep plugin windows above the main window
  const auto net_wm_state = XInternAtom (dpy, "_NET_WM_STATE", False);
  const auto above = XInternAtom (dpy, "_NET_WM_STATE_ABOVE", False);
  XChangeProperty (
    dpy, pimpl_->win_, net_wm_state, XA_ATOM, 32, PropModeReplace,
    reinterpret_cast<const unsigned char *> (&above), 1);

  XSetWMProtocols (dpy, pimpl_->win_, &pimpl_->wm_delete_, 1);
  XSelectInput (dpy, pimpl_->win_, StructureNotifyMask | SubstructureNotifyMask);

  // Snapshot embed-area children so the plugin's client window can be
  // identified by diffing in completeNativeEmbedding()
  pimpl_->initial_children_ = child_windows (*dpy, pimpl_->embed_win_);

  const auto wm_protocols = XInternAtom (dpy, "WM_PROTOCOLS", True);
  mgr.register_window (pimpl_->win_, [this, wm_protocols] (const XEvent &ev) {
    if (
      ev.type == ClientMessage
      && static_cast<Atom> (ev.xclient.message_type) == wm_protocols
      && static_cast<Atom> (ev.xclient.data.l[0]) == pimpl_->wm_delete_)
      Q_EMIT closeRequested ();
  });
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

  const auto total_height = height + pimpl_->header_height_px ();
  const auto screen = DefaultScreen (dpy);
  const auto x = std::max (0, (DisplayWidth (dpy, screen) - width) / 2);
  const auto y = std::max (0, (DisplayHeight (dpy, screen) - total_height) / 2);
  XMoveResizeWindow (
    dpy, pimpl_->win_, x, y, static_cast<unsigned> (width),
    static_cast<unsigned> (total_height));
  pimpl_->view_width_ = width;
  pimpl_->view_height_ = height;
  pimpl_->layout_children (*dpy, width, height);
  pimpl_->paint_header (*dpy);
  XFlush (dpy);
}

void
X11PluginHostWindow::setSize (int width, int height)
{
  auto * dpy = X11DisplayManager::instance ().display ();
  if (dpy == nullptr || pimpl_->win_ == 0)
    return;
  XResizeWindow (
    dpy, pimpl_->win_, static_cast<unsigned> (width),
    static_cast<unsigned> (height + pimpl_->header_height_px ()));
  pimpl_->view_width_ = width;
  pimpl_->view_height_ = height;
  pimpl_->layout_children (*dpy, width, height);
  pimpl_->paint_header (*dpy);
  XFlush (dpy);
}

void
X11PluginHostWindow::setResizable (bool resizable)
{
  auto * dpy = X11DisplayManager::instance ().display ();
  if (dpy == nullptr || pimpl_->win_ == 0)
    return;

  XSizeHints hints{};
  hints.flags = PMinSize;
  // Bare minimum: the header strip with its controls
  hints.min_width =
    pimpl_->header_ != nullptr ? pimpl_->header_->controls_implicit_width () : 1;
  hints.min_height = pimpl_->header_height_px ();
  if (!resizable)
    {
      hints.flags |= PMaxSize;
      hints.min_width = hints.max_width = pimpl_->view_width_;
      hints.min_height = hints.max_height =
        pimpl_->view_height_ + pimpl_->header_height_px ();
    }
  XSetWMNormalHints (dpy, pimpl_->win_, &hints);
  XFlush (dpy);
}

void
X11PluginHostWindow::setVisible (bool shouldBeVisible)
{
  auto * dpy = X11DisplayManager::instance ().display ();
  if (dpy == nullptr || pimpl_->win_ == 0)
    return;
  if (shouldBeVisible)
    XMapWindow (dpy, pimpl_->win_);
  else
    XUnmapWindow (dpy, pimpl_->win_);
  XFlush (dpy);
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
  pimpl_->complete_native_embedding (*dpy, 20);
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
