/*
 * inc/utils/selections.c - Selections
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "audio/channel.h"
#include "gui/widgets/midi_editor.h"
#include "utils/selections.h"

/**
 * Returns 1 if channel is currently selected
 */
int
selections_is_channel_selected (Channel * channel)
{

}

/**
 * Sets selection and notifies insterested parties.
 */
void
selections_set_channel (Channel * channel)
{
  /* if selection exists */
  if (SELECTIONS.num_channels > 0)
    {
      channel_reattach_midi_editor_manual_press_port (
                      SELECTIONS.channels[SELECTIONS.num_channels - 1], 0);
    }
  SELECTIONS.channels[0] = channel;
  SELECTIONS.num_channels = 1;
  channel_reattach_midi_editor_manual_press_port (
                  channel, 1);
}

/**
 * Adds channel to selections (eg. when shift/ctrl clicking)
 */
void
selections_add_channel (Channel * channel)
{

}
