/*
 * audio/slot.c - a slot on a channel
 *
 * Copyright (C) 2018 Alexandros Theodotou
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

#include "audio/plugin.h"
#include "audio/slot.h"

/**
 * Normally, the channel will call the process func on each of its plugins
 * in order.
 */
void
process_impl (Slot * slot,  ///< slot
              nframes_t)    ///< sample count
{
  /* loop through each port of the plugin */
  for (int i = 0; i < MAX_IN_PORTS; i++)
    {
      Ports * incoming_ports
      while (
      Plugin * bp = slots[i].plugin;
      if (bp != NULL)
        {
          get_input_ports (
          /* TODO connect ports, but for now assume everything uses 2 channels */
          bp->process (


}

/*
 * Initializes a slot with default values
 */
void
init_slot (Slot * slot)
{
  slot->plugin = NULL;
  slot->process = process_impl;
}
