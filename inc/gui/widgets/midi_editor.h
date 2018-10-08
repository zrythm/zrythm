/*
 * inc/gui/widgets/editor_notebook.h - Editor notebook (bot of arranger)
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

#ifndef __GUI_WIDGETS_MIDI_EDITOR_H__
#define __GUI_WIDGETS_MIDI_EDITOR_H__

typedef struct Region Region;

/**
 * Sets up the MIDI editor for the given region.
 */
void
midi_editor_set_region (Region * region);

/**
 * Sets up the MIDI editor.
 */
void
midi_editor_setup ();

#endif
