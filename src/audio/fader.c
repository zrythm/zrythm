/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "audio/fader.h"
#include "utils/math.h"

void
fader_init (
  Fader * self,
  Channel * ch)
{
  self->channel = ch;

  /* set volume, phase, pan */
  self->volume = 0.0f;
  self->amp = 1.0f;
  self->phase = 0.0f;
  self->pan = 0.5f;
  self->l_port_db = 0.f;
  self->r_port_db = 0.f;
}

/**
 * Sets the amplitude of the fader. (0.0 to 2.0)
 */
void
fader_set_amp (void * _fader, float amp)
{
  Fader * self = (Fader *) _fader;
  self->amp = amp;

  /* calculate volume */
  self->volume = math_amp_to_dbfs (amp);

  /* TODO update tooltip */
  /* FIXME DON'T DO THESE IN THE AUDIO THREAD */
  /*gtk_label_set_text (*/
    /*channel->widget->phase_reading,*/
    /*g_strdup_printf ("%.1f", channel->volume));*/
  /*g_idle_add ((GSourceFunc) redraw_fader_asnyc,*/
              /*channel);*/
}

float
fader_get_amp (void * _self)
{
  Fader * self = (Fader *) _self;
  return self->amp;
}
