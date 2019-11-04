/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of ZPlugins
 *
 * ZPlugins is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * ZPlugins is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU General Affero Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
/*
  Copyright 2012-2019 David Robillard <http://drobilla.net>

  Permission to use, copy, modify, and/or distribute this software for any
  purpose with or without fee is hereby granted, provided that the above
  copyright notice and this permission notice appear in all copies.

  THIS SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

#include "config.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "zlfo_common.h"
#include "ztoolkit/ztk.h"

#include <cairo.h>
#include <pugl/pugl.h>
#include <pugl/pugl_cairo_backend.h>

#include "lv2/lv2plug.in/ns/ext/atom/atom.h"
#include "lv2/lv2plug.in/ns/ext/atom/forge.h"
#include "lv2/lv2plug.in/ns/ext/atom/util.h"
#include "lv2/lv2plug.in/ns/ext/log/log.h"
#include "lv2/lv2plug.in/ns/ext/patch/patch.h"
#include "lv2/lv2plug.in/ns/ext/urid/urid.h"
#include "lv2/lv2plug.in/ns/extensions/ui/ui.h"

#define TITLE "ZLFO"

/** Width and height of the window. */
#define WIDTH 320
#define HEIGHT 120

#define GET_HANDLE \
  ZLfoUi * self = (ZLfoUi *) puglGetHandle (view);

typedef struct ZtkApp ZtkApp;

typedef struct ZLfoUi
{
  /** Port values. */
  float                freq;
  float                phase;

  LV2UI_Write_Function write;
  LV2UI_Controller     controller;

  /** Map feature. */
  LV2_URID_Map *       map;

  /** Atom forge. */
  LV2_Atom_Forge       forge;

  /** Log feature. */
  LV2_Log_Log *        log;

  /** URIs. */
  ZLfoUris             uris;

  /**
   * This is the window passed in the features from
	 * the host.
   *
   * The pugl window will be wrapped in here.
   */
  void *               parent_window;

  /**
   * Resize handle for the parent window.
   */
  LV2UI_Resize*        resize;

  ZtkApp *             app;
} ZLfoUi;

#define SEND_PORT_EVENT(_self,idx,val) \
  _self->write ( \
     _self->controller, (uint32_t) idx, \
     sizeof (float), 0, &val)

static void
set_freq (
  void *   obj,
  float    val)
{
  ZLfoUi * self = (ZLfoUi *) obj;
  self->freq = val;
  SEND_PORT_EVENT (self, LFO_FREQ, self->freq);
}

static float
get_freq (
  void *   obj)
{
  ZLfoUi * self = (ZLfoUi *) obj;
  return self->freq;
}

static void
create_ui (
  ZLfoUi * self)
{
	PuglWorld * world = puglNewWorld ();

  /* resize the host's window. */
  self->resize->ui_resize (
    self->resize->handle, WIDTH, HEIGHT);

  self->app = ztk_app_new (
    world, TITLE,
    (PuglNativeWindow) self->parent_window,
    WIDTH, HEIGHT);

  int num_controls = 1;

#define PADDING 10.0
#define BOX_DIM 32.0

  /* get available space for each ctrl */
  double available_space_for_ctrl =
    WIDTH / num_controls;

  /* add each ctrl */
  for (int i = 0; i < num_controls; i++)
    {
      /* add frequency knob */
      PuglRect rect = {
        (i * available_space_for_ctrl +
          available_space_for_ctrl / 2.0) -
          BOX_DIM / 2.0,
        HEIGHT / 2 - 30 / 2,
        BOX_DIM,
        BOX_DIM };
      ZtkKnob * knob =
        ztk_knob_new (
          &rect, get_freq, set_freq, self,
          0.f, 20.f, 0.f);
      ZtkWidget * w =
        (ZtkWidget *) knob;
      ztk_app_add_widget (
        self->app, w, 2);

      /* add label */
      char name[500];
      sprintf (name, "Frequency");
      ZtkLabel * lbl =
        ztk_label_new  (
          24.0, 24.0, 14,
          &self->app->theme.bright_orange,
          name);

      ZtkKnobWithLabel * knob_with_label =
        ztk_knob_with_label_new (
          &rect, knob, lbl);
      w = (ZtkWidget *) knob_with_label;
      ztk_app_add_widget (
        self->app, w, 2);
    }
}

static LV2UI_Handle
instantiate (
  const LV2UI_Descriptor*   descriptor,
  const char*               plugin_uri,
  const char*               bundle_path,
  LV2UI_Write_Function      write_function,
  LV2UI_Controller          controller,
  LV2UI_Widget*             widget,
  const LV2_Feature* const* features)
{
  ZLfoUi * self = calloc (1, sizeof (ZLfoUi));
  self->write = write_function;
  self->controller = controller;

#define HAVE_FEATURE(x) \
  (!strcmp(features[i]->URI, x))

  for (int i = 0; features[i]; ++i)
    {
      if (HAVE_FEATURE (LV2_UI__parent))
        {
          self->parent_window = features[i]->data;
        }
      else if (HAVE_FEATURE (LV2_UI__resize))
        {
          self->resize =
            (LV2UI_Resize*)features[i]->data;
        }
      else if (HAVE_FEATURE (LV2_URID__map))
        {
          self->map =
            (LV2_URID_Map *) features[i]->data;
        }
      else if (HAVE_FEATURE (LV2_LOG__log))
        {
          self->log =
            (LV2_Log_Log *) features[i]->data;
        }
    }

#undef HAVE_FEATURE

  if (!self->map)
    {
			log_error (
				self->log, &self->uris,
				"Missing feature urid:map");
    }

  /* map uris */
  map_uris (self->map, &self->uris);

  lv2_atom_forge_init (
    &self->forge, self->map);

  /* create UI and set the native window to the
   * widget */
  create_ui (self);
  *widget =
    (LV2UI_Widget)
    puglGetNativeWindow (self->app->view);

  return self;
}

static void
cleanup (LV2UI_Handle handle)
{
  ZLfoUi * self = (ZLfoUi *) handle;

  ztk_app_free (self->app);

  free (self);
}

/**
 * Port event from the plugin.
 */
static void
port_event (
  LV2UI_Handle handle,
  uint32_t     port_index,
  uint32_t     buffer_size,
  uint32_t     format,
  const void*  buffer)
{
  ZLfoUi * self = (ZLfoUi *) handle;

  /* check type of data received
   *  format == 0: [float] control-port event
   *  format > 0: message
   *  Every event message is sent as separate
   *  port-event
   */
  if (format == 0)
    {
      switch (port_index)
        {
        case LFO_FREQ:
          self->freq = * (const float *) buffer;
          break;
        default:
          break;
        }
      puglPostRedisplay (self->app->view);
    }
  else if (format == self->uris.atom_eventTransfer)
    {
      const LV2_Atom* atom =
        (const LV2_Atom*) buffer;
      if (lv2_atom_forge_is_object_type (
            &self->forge, atom->type))
        {
          /*const LV2_Atom_Object* obj =*/
            /*(const LV2_Atom_Object*) atom;*/

          /*const char* uri =*/
            /*(const char*) LV2_ATOM_BODY_CONST (*/
              /*obj);*/
        }
      else
        {
					log_error (
						self->log, &self->uris,
						"Unknown message type");
        }
    }
  else
    {
			log_error (
				self->log, &self->uris,
				"Unknown format");
    }
}

/* Optional non-embedded UI show interface. */
/*static int*/
/*ui_show (LV2UI_Handle handle)*/
/*{*/
  /*printf ("show called\n");*/
  /*ZLfoUi * self = (ZLfoUi *) handle;*/
  /*ztk_app_show_window (self->app);*/
  /*return 0;*/
/*}*/

/* Optional non-embedded UI hide interface. */
/*static int*/
/*ui_hide (LV2UI_Handle handle)*/
/*{*/
  /*printf ("hide called\n");*/
  /*ZLfoUi * self = (ZLfoUi *) handle;*/
  /*ztk_app_hide_window (self->app);*/

  /*return 0;*/
/*}*/

/* Idle interface for optional non-embedded UI. */
static int
ui_idle (LV2UI_Handle handle)
{
  ZLfoUi * self = (ZLfoUi *) handle;

  ztk_app_idle (self->app);

  return 0;
}

// LV2 resize interface to host
static int
ui_resize (
  LV2UI_Feature_Handle handle, int w, int h)
{
  ZLfoUi * self = (ZLfoUi *) handle;
  self->resize->ui_resize (
    self->resize->handle, WIDTH, HEIGHT);
  return 0;
}

// connect idle and resize functions to host
static const void*
extension_data (const char* uri)
{
  static const LV2UI_Idle_Interface idle = {
    ui_idle };
  static const LV2UI_Resize resize = {
    0 ,ui_resize };
  if (!strcmp(uri, LV2_UI__idleInterface))
    {
      return &idle;
    }
  if (!strcmp(uri, LV2_UI__resize))
    {
      return &resize;
    }
  return NULL;
}

static const LV2UI_Descriptor descriptor = {
  LFO_UI_URI,
  instantiate,
  cleanup,
  port_event,
  extension_data,
};

LV2_SYMBOL_EXPORT
const LV2UI_Descriptor*
lv2ui_descriptor (uint32_t index)
{
  switch (index)
    {
    case 0:
      return &descriptor;
    default:
      return NULL;
    }
}
