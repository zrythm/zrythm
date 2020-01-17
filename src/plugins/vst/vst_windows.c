/*
 * Copyright (C) 2020 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 *
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 * Copyright (C) 2004 Paul Davis
 * Based on code by Paul Davis, Torben Hohn as part of FST
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "config.h"

#ifdef _WIN32

#include <windows.h>
#include <pthread.h>

#include "plugins/plugin.h"
#include "plugins/vst_plugin.h"
#include "plugins/vst/vst_windows.h"
#include "utils/windows_errors.h"

#include <gtk/gtk.h>

static UINT_PTR idle_timer_id   = 0;

static pthread_mutex_t plugin_mutex;
/* Head of linked list of all FSTs */
static VstPlugin * fst_first = NULL;
static int              host_initialized = 0;

/* GDK exports this symbol publically but for some
 * reason it's not exposed in a public API, so
 * define it manually.
 * see https://discourse.gnome.org/t/getting-a-win32-window-handle-from-a-gtkwindow/2480 */
HGDIOBJ
gdk_win32_window_get_handle (GdkWindow *window);

static LRESULT WINAPI
vstedit_wndproc (
  HWND w,
  UINT msg,
  WPARAM wp,
  LPARAM lp)
{
  switch (msg)
    {
    case WM_KEYUP:
    case WM_KEYDOWN:
      break;

    case WM_SIZE:
      {
        LRESULT rv = DefWindowProcA (w, msg, wp, lp);
        RECT rect;
        GetClientRect(w, &rect);
        g_message (
          "VST WM_SIZE.. %ld %ld %ld %ld",
          rect.top, rect.left,
          (rect.right - rect.left),
          (rect.bottom - rect.top));
        VstPlugin * fst =
          (VstPlugin *) GetProp (w, "fst_ptr");
        if (fst)
          {
            int32_t width = (rect.right - rect.left);
            int32_t height = (rect.bottom - rect.top);
            if (width > 0 && height > 0)
              {
                /*fst->amc (fst->plugin, 15 [>audioMasterSizeWindow <], width, height, NULL, 0);*/
              }
          }
        return rv;
      }
      break;
    case WM_CLOSE:
      /* we don't care about windows closing ...
       * WM_CLOSE is used for minimizing the window.
       * Our window has no frame so it shouldn't ever
       * get sent - but if it does, we don't want our
       * window to get minimized!
       */
      return 0;
      break;

    case WM_DESTROY:
    case WM_NCDESTROY:
      /* we don't care about windows being
       * destroyed ... */
      return 0;
      break;

    default:
      break;
  }

  return DefWindowProcA (w, msg, wp, lp);
}

/**
 * Inits the windows VST subsystem.
 */
int
vst_windows_init (void)
{
  if (host_initialized) return 0;
  HMODULE hInst;

  if ((hInst = GetModuleHandleA (NULL)) == NULL)
    {
      g_critical ("can't get module handle");
      return -1;
    }

  WNDCLASSEX wclass;

  wclass.cbSize = sizeof(WNDCLASSEX);
  wclass.style = (CS_HREDRAW | CS_VREDRAW);
  wclass.hIcon = NULL;
  wclass.hCursor = LoadCursor(0, IDC_ARROW);
  wclass.hbrBackground =
    (HBRUSH) GetStockObject (BLACK_BRUSH);
  wclass.lpfnWndProc = vstedit_wndproc;
  wclass.cbClsExtra = 0;
  wclass.cbWndExtra = 0;
  wclass.hInstance = hInst;
  wclass.lpszMenuName = "MENU_FST";
  wclass.lpszClassName = "FST";
  wclass.hIconSm = 0;

  pthread_mutex_init (&plugin_mutex, NULL);
  host_initialized = -1;

  if (!RegisterClassExA(&wclass))
    {
      g_critical (
        "Error initializing windows VST subsystem: "
        "(class registration failed");
      return -1;
    }
  g_message ("VST host initialized");

  return 0;
}

static void
move_window_into_view (
  VstPlugin * self)
{
  g_message ("moving window into view");
  if (self->windows_window)
    {
      SetWindowPos (
        (HWND)(self->windows_window),
        HWND_TOP /*0*/,
        self->hoffset, self->voffset,
        self->width, self->height,
        SWP_NOACTIVATE | SWP_NOOWNERZORDER);
      ShowWindow (
        (HWND)(self->windows_window), SW_SHOWNA);
      UpdateWindow ((HWND) (self->windows_window));
    }
}

static void
resize_cb (
  VstPlugin * self)
{
  g_message ("resize cb on VST plugin");
  if (self->gtk_window_parent)
    {
      int width = self->width + self->hoffset;
      int height = self->height + self->voffset;
      g_message (
        "width %d height %d", width, height);
      gtk_widget_set_size_request (
        GTK_WIDGET (self->gtk_window_parent),
        width, height);
      move_window_into_view (self);
    }
}

/**
 * Packages the plugin into the given window.
 */
int
vst_windows_package (
  VstPlugin * self,
  GtkWindow * win)
{
  g_message ("package plugin");
  self->gtk_window_parent = win;

  resize_cb (self);

  return 0;
}

static void
idle_timer_add_plugin (
  VstPlugin * fst)
{
  pthread_mutex_lock (&plugin_mutex);

  if (fst_first == NULL)
    {
      fst_first = fst;
    }
  else
    {
      VstPlugin * p = fst_first;
      while (p->next)
        {
          p = p->next;
        }
      p->next = fst;
    }

  pthread_mutex_unlock (&plugin_mutex);

  g_message ("plugin added to timer");
}

/**
 * Timer callback.
 *
 * @param hwnd Handle to window for timer messages.
 * @param message WM_TIMER message.
 * @param idTimer timer identifier.
 * @param dwTime current system time.
 */
static VOID CALLBACK
idle_hands (
  HWND hwnd,
  UINT message,
  UINT idTimer,
  DWORD dwTime)
{
  /*g_message ("idle hands started");*/
  VstPlugin * fst;

  pthread_mutex_lock (&plugin_mutex);

  for (fst = fst_first; fst; fst = fst->next)
    {
      if (fst->gui_shown)
        {
          // this seems insane, but some plugins
          // will not draw their meters if you don't
          // call this every time.  Example
          // Ambience by Magnus @ Smartelectron:x
          fst->aeffect->dispatcher (
            fst->aeffect, effEditIdle, 0, 0,
            NULL, 0);

          if (fst->wantIdle)
            {
              fst->wantIdle =
                fst->aeffect->dispatcher (
                  fst->aeffect, effIdle, 0, 0,
                  NULL, 0);
            }
        }

      pthread_mutex_lock (&fst->lock);

      /* See comment for call below */
      /*vststate_maybe_set_program (fst);*/
      fst->want_program = -1;
      fst->want_chunk = 0;
      /* If we don't have an editor window yet, we still need to
       * set up the program, otherwise when we load a plugin without
       * opening its window it will sound wrong.  However, it seems
       * that if you don't also load the program after opening the GUI,
       * the GUI does not reflect the program properly.  So we'll not
       * mark that we've done this (ie we won't set want_program to -1)
       * and so it will be done again if and when the GUI arrives.
       */
#if 0
      if (fst->program_set_without_editor == 0) {
        vststate_maybe_set_program (fst);
        fst->program_set_without_editor = 1;
      }
#endif

      pthread_mutex_unlock (&fst->lock);
    }

  pthread_mutex_unlock (&plugin_mutex);
  /*g_message ("idle hands ended");*/
}

static void
idle_timer_remove_plugin (
  VstPlugin * fst)
{
  VstPlugin * p;
  VstPlugin * prev;

  pthread_mutex_lock (&plugin_mutex);

  for (p = fst_first, prev = NULL;
       p; prev = p, p = p->next)
    {
      if (p == fst)
        {
          if (prev)
            {
              prev->next = p->next;
            }
          break;
        }
      if (!p->next)
        {
          break;
        }
    }

  if (fst_first == fst)
    {
      fst_first = fst_first->next;
    }

  pthread_mutex_unlock (&plugin_mutex);
}

void
vst_windows_exit (void)
{
#if 0
  if (!host_initialized) return;
  VSTState* fst;
  // If any plugins are still open at this point, close them!
  while ((fst = fst_first))
    vst_windows_close (fst);

  if (idle_timer_id != 0) {
    KillTimer (NULL, idle_timer_id);
  }

  host_initialized = FALSE;
  pthread_mutex_destroy (&plugin_mutex);
#endif
}

void
vst_windows_destroy_editor (
  VstPlugin * fst)
{
  if (fst->windows_window)
    {
      g_message (
        "%s destroying edit window\n",
        fst->plugin->descr->name);

      idle_timer_remove_plugin (fst);
      fst->aeffect->dispatcher (
        fst->aeffect, effEditClose, 0, 0, NULL, 0.0);

      DestroyWindow ((HWND)(fst->windows_window));

      fst->windows_window = NULL;
    }

  fst->been_activated = 0;
}

/**
 * Adds a new plugin instance to the linked list.
 */
int
vst_windows_run_editor (
  VstPlugin * self,
  GtkWidget * win)
{
  gtk_widget_realize (win);
  void * hWndHost =
    gdk_win32_window_get_handle (
      gtk_widget_get_window (win));
  g_return_val_if_fail (hWndHost, -1);

  /* For safety, remove any pre-existing editor
   * window */
  vst_windows_destroy_editor (self);

  if (self->windows_window == NULL)
    {
      HMODULE hInst;
      HWND window;
      struct ERect* er = NULL;

      if (!(self->aeffect->flags &
              effFlagsHasEditor))
        {
          g_critical (
            "Plugin %s has no editor",
            self->plugin->descr->name);
          return -1;
        }

    if ((hInst = GetModuleHandleA (NULL)) == NULL)
      {
        g_critical (
          "Can't get module handle");
        return 1;
      }

    if ((window =
           CreateWindowExA (
             0, "FST", self->plugin->descr->name,
               hWndHost ?
                 WS_CHILD :
                 (WS_OVERLAPPEDWINDOW &
                  ~WS_THICKFRAME & ~WS_MAXIMIZEBOX),
            CW_USEDEFAULT, CW_USEDEFAULT,
            CW_USEDEFAULT, CW_USEDEFAULT,
            (HWND) hWndHost, NULL,
            hInst, NULL) ) == NULL)
      {
        char error_str[1000];
        windows_errors_get_last_error_str (error_str);
        g_critical (
          "Cannot create editor window: %s",
    error_str);
        return 1;
      }
    g_message ("VST window created");

    if (!SetPropA (window, "fst_ptr", self))
      {
        g_warning (
          "Cannot set fst_ptr on window");
      }
    g_message ("VST: set window pointer");

    self->windows_window = window;

    if (hWndHost)
      {
        // This is requiredv for some reason. Note the
        // parent is set above when the window
        // is created. Without this extra call the
        // actual plugin window will draw outside
        // of our plugin window.
        SetParent (
          (HWND) self->windows_window,
          (HWND) hWndHost);
        self->xid = 0;
      }

    // This is the suggested order of calls.
    self->aeffect->dispatcher (
      self->aeffect, effEditGetRect, 0, 0, &er, 0 );
    self->aeffect->dispatcher (
      self->aeffect, effEditOpen, 0, 0,
      self->windows_window, 0 );
    self->aeffect->dispatcher (
      self->aeffect, effEditGetRect, 0, 0, &er, 0 );

    if (er)
      {
        self->width = er->right - er->left;
        self->height = er->bottom - er->top;
      }
    else
    {
      g_warning ("No rect for plugin");
    }

    self->been_activated = 1;

    g_message ("VST window activated");
  }

  if (self->windows_window)
    {
      if (idle_timer_id == 0)
        {
          // Init the idle timer if needed, so that
          // the main window calls us.
          idle_timer_id =
            SetTimer (
              NULL, idle_timer_id, 50,
              (TIMERPROC) idle_hands);
          g_message ("idle timer initialized");
        }

    idle_timer_add_plugin (self);

  vst_windows_package (
    self, win);
  }

  return self->windows_window == NULL ? -1 : 0;
}

#endif
