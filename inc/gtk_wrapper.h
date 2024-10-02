// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __GTK_WRAPPER_H__
#define __GTK_WRAPPER_H__

#include "zrythm-config.h"

// NOLINTBEGIN

#include <QtGui>
#include <QtWidgets>

// Save Qt's macros
#define QT_SIGNALS signals
#define QT_SLOTS slots
#define QT_EMIT emit
#define QT_FOREACH foreach

// Undefine Qt's macros
#undef signals
#undef slots
#undef emit
#undef foreach

#include <glibmm.h>

#ifdef __GNUC__
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wredundant-decls"
#endif

#include <giomm.h>

#ifdef __GNUC__
#  pragma GCC diagnostic pop
#endif

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
#include <gtk/gtk.h>
G_GNUC_END_IGNORE_DEPRECATIONS

#if HAVE_X11
/* hack to drop dumb typedefs and macros defined by X11 in the public namespace */
#  define Region RegionForX11
#  define None NoneForX11
#  define Status StatusForX11
#  define DestroyNotify DestroyNotifyForX11
#  define Color ColorForX11
#  define Time TimeForX11
#  define Bool BoolForX11
#  include <gdk/x11/gdkx.h>
#  undef Region
#  undef None
#  undef Status
#  undef DestroyNotify
#  undef Color
#  undef Time
#  undef Bool
#endif

/* another hack for dumb macros by Windows */
#ifdef IN
#  define WINDOWS_MACRO_IN_DEFINED
#  undef IN
#endif
#ifdef OUT
#  define WINDOWS_MACRO_OUT_DEFINED
#  undef OUT
#endif
#ifdef UNICODE
#  define WINDOWS_MACRO_UNICODE_DEFINED
#  undef UNICODE
#endif
#ifdef WINDING
#  define WINDOWS_WINDING WINDING
#  undef WINDING
#endif
#ifdef IGNORE
#  define WINDOWS_IGNORE IGNORE
#  undef IGNORE
#endif
#ifdef near
#  undef near
#endif
#ifdef NEAR
#  define WINDOWS_NEAR_DEFINED
#  undef NEAR
/* we need NEAR to be defined. by default it's defined as `near`, but we don't
 * want `near` to be defined (because it's used by graphene), so we define NEAR
 * to be empty (`near` was empty too anyway) */
#  define NEAR
#endif

#include <gtkmm.h>

#ifdef WINDOWS_MACRO_IN_DEFINED
#  define IN
#  undef WINDOWS_MACRO_IN_DEFINED
#endif
#ifdef WINDOWS_MACRO_OUT_DEFINED
#  define OUT
#  undef WINDOWS_MACRO_OUT_DEFINED
#endif
#ifdef WINDOWS_MACRO_UNICODE_DEFINED
#  define UNICODE
#  undef WINDOWS_MACRO_UNICODE_DEFINED
#endif
#ifdef WINDOWS_WINDING
#  define WINDING WINDOWS_WINDING
#  undef WINDOWS_WINDING
#endif
#ifdef WINDOWS_IGNORE
#  define IGNORE WINDOWS_IGNORE
#  undef WINDOWS_IGNORE
#endif

#ifdef _WIN32
#  include <gdk/win32/gdkwin32.h>
#endif

// Redefine Qt's macros
#define signals QT_SIGNALS
#define slots QT_SLOTS
#define emit QT_EMIT
#define foreach QT_FOREACH

// NOLINTEND

#endif // __GTK_WRAPPER_H__