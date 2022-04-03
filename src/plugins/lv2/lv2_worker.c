/*
 * Copyright (C) 2020 Alexandros Theodotou <alex@zrythm.org>
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
  Copyright 2007-2016 David Robillard <http://drobilla.net>

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

#include "audio/engine.h"
#include "plugins/lv2/lv2_worker.h"
#include "plugins/lv2_plugin.h"
#include "project.h"
#include "zrythm_app.h"

static LV2_Worker_Status
lv2_worker_respond (
  LV2_Worker_Respond_Handle handle,
  uint32_t                  size,
  const void *              data)
{
  Lv2Worker * worker = (Lv2Worker *) handle;
  zix_ring_write (
    worker->responses, (const char *) &size,
    sizeof (size));
  zix_ring_write (
    worker->responses, (const char *) data, size);
  return LV2_WORKER_SUCCESS;
}

/**
 * Worker logic running in a separate thread (if
 * worker is threaded).
 */
static void *
worker_func (void * data)
{
  Lv2Worker * worker = (Lv2Worker *) data;
  Lv2Plugin * plugin = worker->plugin;
  void *      buf = NULL;
  while (true)
    {
      zix_sem_wait (&worker->sem);
      if (plugin->exit)
        {
          break;
        }

      uint32_t size = 0;
      zix_ring_read (
        worker->requests, (char *) &size,
        sizeof (size));

      if (!(buf = realloc (buf, size)))
        {
          g_warning ("error: realloc() failed");
          free (buf);
          return NULL;
        }

      zix_ring_read (
        worker->requests, (char *) buf, size);

      zix_sem_wait (&plugin->work_lock);
      char pl_str[700];
      plugin_print (plugin->plugin, pl_str, 700);
      if (DEBUGGING)
        {
          g_debug (
            "running work (threaded) for plugin %s",
            pl_str);
        }
      worker->iface->work (
        plugin->instance->lv2_handle,
        lv2_worker_respond, worker, size, buf);
      zix_sem_post (&plugin->work_lock);
    }

  free (buf);
  return NULL;
}

void
lv2_worker_init (
  Lv2Plugin *                  plugin,
  Lv2Worker *                  worker,
  const LV2_Worker_Interface * iface,
  bool                         threaded)
{
  g_message (
    "initializing worker for LV2 plugin %s",
    plugin->plugin->setting->descr->name);
  g_return_if_fail (plugin && worker && iface);
  worker->iface = iface;
  worker->threaded = threaded;
  if (threaded)
    {
      zix_thread_create (
        &worker->thread, 4096, worker_func, worker);
      worker->requests = zix_ring_new (4096);
      zix_ring_mlock (worker->requests);
    }
  worker->responses = zix_ring_new (4096);
  worker->response = malloc (4096);
  zix_ring_mlock (worker->responses);
}

/**
 * Stops the worker and frees resources.
 */
void
lv2_worker_finish (Lv2Worker * worker)
{
  if (worker->requests)
    {
      if (worker->threaded)
        {
          zix_sem_post (&worker->sem);
          zix_thread_join (worker->thread, NULL);
          zix_ring_free (worker->requests);
        }
      zix_ring_free (worker->responses);
      free (worker->response);
    }
}

/**
 * Called from plugins during run() to request that
 * Zrythm calls the work() method in a non-realtime
 * context with the given arguments.
 */
LV2_Worker_Status
lv2_worker_schedule (
  LV2_Worker_Schedule_Handle handle,
  uint32_t                   size,
  const void *               data)
{
  Lv2Worker * worker = (Lv2Worker *) handle;
  Lv2Plugin * plugin = worker->plugin;
  if (!worker->threaded || AUDIO_ENGINE->exporting)
    {
      /* Execute work immediately in this thread */
      zix_sem_wait (&plugin->work_lock);
      char pl_str[700];
      plugin_print (plugin->plugin, pl_str, 700);
      if (!worker->iface)
        {
          g_warning (
            "Worker interface for %s is NULL",
            pl_str);
          return LV2_WORKER_ERR_UNKNOWN;
        }
      g_debug (
        "running work (threaded %d) immediately "
        "for plugin %s",
        worker->threaded, pl_str);
      worker->iface->work (
        plugin->instance->lv2_handle,
        lv2_worker_respond, worker, size, data);
      zix_sem_post (&plugin->work_lock);
    }
  else
    {
      /* Schedule a request to be executed by the
       * worker thread */
      zix_ring_write (
        worker->requests, (const char *) &size,
        sizeof (size));
      zix_ring_write (
        worker->requests, (const char *) data, size);
      zix_sem_post (&worker->sem);
    }
  return LV2_WORKER_SUCCESS;
}

/**
 * Called during run() to process worker replies.
 *
 * Internally calls work_response in
 * https://lv2plug.in/doc/html/group__worker.html.
 */
void
lv2_worker_emit_responses (
  Lv2Worker *    worker,
  LilvInstance * instance)
{
  if (worker->responses)
    {
      uint32_t read_space =
        zix_ring_read_space (worker->responses);
      while (read_space)
        {
          uint32_t size = 0;
          zix_ring_read (
            worker->responses, (char *) &size,
            sizeof (size));

          zix_ring_read (
            worker->responses,
            (char *) worker->response, size);

          worker->iface->work_response (
            instance->lv2_handle, size,
            worker->response);

          read_space -= sizeof (size) + size;
        }
    }
}
