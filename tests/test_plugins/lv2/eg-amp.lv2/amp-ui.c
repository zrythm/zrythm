// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/* Stub X11UI for the eg-amp fixture plugin. It draws nothing; it exists
 * so UI hosting behaviors (session lifetime, control relay, writes) are
 * testable. The tests observe and drive it through dlopen(): the
 * counters and the amp_ui_*() functions below are the whole surface. */

#include "lv2/atom/atom.h"
#include "lv2/core/lv2.h"
#include "lv2/ui/ui.h"
#include "lv2/urid/urid.h"

#include <stdint.h>
#include <string.h>

static int g_instantiations = 0;
static int g_cleanups = 0;
static int g_port_events = 0;
static int g_idle_calls = 0;
static int g_show_calls = 0;
static int g_hide_calls = 0;
static int g_close_on_idle = 0;

static uint32_t g_last_event_port = UINT32_MAX;
static float g_last_event_value = 0.0f;

static LV2UI_Write_Function g_write = NULL;
static LV2UI_Controller g_controller = NULL;
static const LV2_URID_Map * g_map = NULL;

int
amp_ui_instantiations (void)
{
  return g_instantiations;
}

int
amp_ui_cleanups (void)
{
  return g_cleanups;
}

int
amp_ui_port_events (void)
{
  return g_port_events;
}

int
amp_ui_idle_calls (void)
{
  return g_idle_calls;
}

int
amp_ui_show_calls (void)
{
  return g_show_calls;
}

int
amp_ui_hide_calls (void)
{
  return g_hide_calls;
}

uint32_t
amp_ui_last_event_port (void)
{
  return g_last_event_port;
}

float
amp_ui_last_event_value (void)
{
  return g_last_event_value;
}

/* Makes the next idle() report that the UI wants its window closed
 * (the return of idle() is the LV2UI idle contract for that). */
void
amp_ui_set_close_on_idle (void)
{
  g_close_on_idle = 1;
}

/* Simulates the user turning the gain knob. */
void
amp_ui_write_gain (float value)
{
  if (g_write != NULL)
    {
      g_write (g_controller, 0, sizeof (float), 0, &value);
    }
}

/* Sends a string atom to the plugin's message port (index 4) via
 * atom:eventTransfer. */
void
amp_ui_write_message (void)
{
  if (g_write == NULL || g_map == NULL)
    {
      return;
    }

  struct
  {
    LV2_Atom atom;
    char     body[6];
  } event;
  event.atom.size = 6;
  event.atom.type = g_map->map (g_map->handle, LV2_ATOM__String);
  memcpy (event.body, "hello", 6);
  g_write (
    g_controller, 4, sizeof (event),
    g_map->map (g_map->handle, LV2_ATOM__eventTransfer), &event);
}

static LV2UI_Handle
ui_instantiate (
  const LV2UI_Descriptor *   descriptor,
  const char *               plugin_uri,
  const char *               bundle_path,
  LV2UI_Write_Function       write_function,
  LV2UI_Controller           controller,
  LV2UI_Widget *             widget,
  const LV2_Feature * const *features)
{
  (void) descriptor;
  (void) plugin_uri;
  (void) bundle_path;

  for (const LV2_Feature * const * f = features; *f != NULL; ++f)
    {
      if (strcmp ((*f)->URI, LV2_URID_MAP_URI) == 0)
        {
          g_map = (const LV2_URID_Map *) (*f)->data;
        }
    }

  g_write = write_function;
  g_controller = controller;
  g_last_event_port = UINT32_MAX;
  g_close_on_idle = 0;
  ++g_instantiations;

  /* dummy X11 window id; the host window never maps it */
  *widget = (LV2UI_Widget) (uintptr_t) 0x1234;
  return (LV2UI_Handle) &g_instantiations;
}

static void
ui_cleanup (LV2UI_Handle ui)
{
  (void) ui;
  g_write = NULL;
  g_controller = NULL;
  g_map = NULL;
  ++g_cleanups;
}

static void
ui_port_event (
  LV2UI_Handle ui,
  uint32_t     port_index,
  uint32_t     buffer_size,
  uint32_t     format,
  const void * buffer)
{
  (void) ui;
  if (format == 0 && buffer_size == sizeof (float))
    {
      g_last_event_port = port_index;
      g_last_event_value = *(const float *) buffer;
      ++g_port_events;
    }
}

static int
ui_idle (LV2UI_Handle ui)
{
  (void) ui;
  ++g_idle_calls;
  return g_close_on_idle ? 1 : 0;
}

static int
ui_show (LV2UI_Handle ui)
{
  (void) ui;
  ++g_show_calls;
  return 0;
}

static int
ui_hide (LV2UI_Handle ui)
{
  (void) ui;
  ++g_hide_calls;
  return 0;
}

static const LV2UI_Idle_Interface g_idle_interface = { ui_idle };
static const LV2UI_Show_Interface g_show_interface = { ui_show, ui_hide };

static const void *
ui_extension_data (const char * uri)
{
  if (strcmp (uri, LV2_UI__idleInterface) == 0)
    {
      return &g_idle_interface;
    }
  return NULL;
}

/* The float-window UI additionally provides ui:showInterface: it opens
 * and hides its own window instead of being embedded */
static const void *
ui_float_window_extension_data (const char * uri)
{
  if (strcmp (uri, LV2_UI__showInterface) == 0)
    {
      return &g_show_interface;
    }
  return ui_extension_data (uri);
}

static const LV2UI_Descriptor g_descriptor = {
  "http://lv2plug.in/plugins/eg-amp#ui",
  ui_instantiate,
  ui_cleanup,
  ui_port_event,
  ui_extension_data,
};

static const LV2UI_Descriptor g_float_window_descriptor = {
  "http://lv2plug.in/plugins/eg-amp-float-window#ui",
  ui_instantiate,
  ui_cleanup,
  ui_port_event,
  ui_float_window_extension_data,
};

LV2_SYMBOL_EXPORT const LV2UI_Descriptor *
lv2ui_descriptor (uint32_t index)
{
  switch (index)
    {
    case 0:
      return &g_descriptor;
    case 1:
      return &g_float_window_descriptor;
    default:
      return NULL;
    }
}
