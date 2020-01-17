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

#include <gtk/gtk.h>

static UINT_PTR idle_timer_id   = 0;

static pthread_mutex_t plugin_mutex;
/* Head of linked list of all FSTs */
static VstPlugin * fst_first = NULL;

/* GDK exports this symbol publically but for some
 * reason it's not exposed in a public API, so
 * define it manually.
 * see https://discourse.gnome.org/t/getting-a-win32-window-handle-from-a-gtkwindow/2480 */
HGDIOBJ
gdk_win32_window_get_handle (GdkWindow *window);

static void
move_window_into_view (
  VstPlugin * self)
{
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
  if (self->gtk_window_parent)
    {
      int width = self->width + self->hoffset;
      int height = self->height + self->voffset;
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
             0, "VST", self->plugin->descr->name,
               hWndHost ?
                 WS_CHILD :
                 (WS_OVERLAPPEDWINDOW &
                  ~WS_THICKFRAME & ~WS_MAXIMIZEBOX),
            CW_USEDEFAULT, CW_USEDEFAULT,
            CW_USEDEFAULT, CW_USEDEFAULT,
            (HWND) hWndHost, NULL,
            hInst, NULL) ) == NULL)
      {
        g_critical (
          "Cannot create editor window");
        return 1;
      }

    if (!SetPropA (window, "fst_ptr", self))
      {
        g_warning (
          "Cannot set fst_ptr on window");
      }

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

    if (er != NULL)
      {
        self->width = er->right - er->left;
        self->height = er->bottom - er->top;
      }

    self->been_activated = 1;

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
        }

    idle_timer_add_plugin (self);
  }

  return self->windows_window == NULL ? -1 : 0;
}

#endif
