/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
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
 */

#include "audio/channel.h"
#include "audio/engine.h"
#include "audio/passthrough_processor.h"
#include "audio/track.h"
#include "project.h"
#include "utils/math.h"

/**
 * Inits passthrough_processor to default values.
 *
 * This assumes that the channel has no plugins.
 *
 * @param self The PassthroughProcessor to init.
 * @param ch Channel.
 */
void
passthrough_processor_init (
  PassthroughProcessor * self,
  Channel * ch)
{
  self->channel = ch;

  self->l_port_db = 0.f;
  self->r_port_db = 0.f;

  /* stereo in */
  char * pll =
    g_strdup_printf (
      "%s Pre-Fader in L",
      ch->track->name);
  char * plr =
    g_strdup_printf (
      "%s Pre-Fader in R",
      ch->track->name);
  self->stereo_in =
    stereo_ports_new (
      port_new_with_type (TYPE_AUDIO,
                          FLOW_INPUT,
                          pll),
      port_new_with_type (TYPE_AUDIO,
                          FLOW_INPUT,
                          plr));
  self->stereo_in->l->identifier.flags |=
    PORT_FLAG_STEREO_L;
  self->stereo_in->r->identifier.flags |=
    PORT_FLAG_STEREO_R;
  self->stereo_in->l->identifier.owner_type =
    PORT_OWNER_TYPE_PREFADER;
  self->stereo_in->r->identifier.owner_type =
    PORT_OWNER_TYPE_PREFADER;

  /* stereo out */
  pll =
    g_strdup_printf ("%s Pre-Fader out L",
                     ch->track->name);
  plr =
    g_strdup_printf ("%s Pre-Fader out R",
                     ch->track->name);
  self->stereo_out =
    stereo_ports_new (
     port_new_with_type (TYPE_AUDIO,
                         FLOW_OUTPUT,
                         pll),
     port_new_with_type (TYPE_AUDIO,
                         FLOW_OUTPUT,
                         plr));
  self->stereo_out->l->identifier.flags |=
    PORT_FLAG_STEREO_L;
  self->stereo_out->r->identifier.flags |=
    PORT_FLAG_STEREO_R;

  port_set_owner_prefader (
    self->stereo_in->l, self);
  port_set_owner_prefader (
    self->stereo_in->r, self);
  port_set_owner_prefader (
    self->stereo_out->l, self);
  port_set_owner_prefader (
    self->stereo_out->r, self);
}

/**
 * Process the PassthroughProcessor.
 *
 * @param start_frame The local offset in this
 *   cycle.
 * @param nframes The number of frames to process.
 */
void
passthrough_processor_process (
  PassthroughProcessor * self,
  long    start_frame,
  int     nframes)
{
  /* copy the input to output */
  for (int i = start_frame;
       i < start_frame + nframes; i++)
    {
      self->stereo_out->l->buf[i] =
        self->stereo_in->l->buf[i];
      self->stereo_out->r->buf[i] =
        self->stereo_in->r->buf[i];
    }
}
