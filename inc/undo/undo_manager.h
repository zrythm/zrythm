/*
 * undo/undo_manager.h - Undo Manager
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

#ifndef __UNDO_UNDO_MANAGER_H__
#define __UNDO_UNDO_MANAGER_H__

#include "utils/stack.h"

#define UNDO_MANAGER ZRYTHM->undo_manager

typedef struct UndoManager
{
  Stack                  undo_stack;
  Stack                  redo_stack;
} UndoManager;

UndoManager *
undo_manager_new ();

/**
 * Undo last action.
 */
void
undo_manager_undo (UndoManager * self);

/**
 * Redo last undone action.
 */
void
undo_manager_redo (UndoManager * self);

#endif
