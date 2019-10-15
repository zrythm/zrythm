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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * \file
 *
 * Common data structures and functions for
 * *ArrangerSelections.
 */

#ifndef __GUI_BACKEND_ARRANGER_SELECTIONS_H__
#define __GUI_BACKEND_ARRANGER_SELECTIONS_H__

/**
 * @addtogroup gui_backend
 *
 * @{
 */

/**
 * Inits the selections after loading a project.
 */
#define ARRANGER_SELECTIONS_DECLARE_INIT_LOADED( \
  cc,sc) \
  void \
  sc##_selections_init_loaded ( \
    cc##Selections * self)

#define ARRANGER_SELECTIONS_DECLARE_RESET_COUNTERPARTS( \
  cc,sc) \
  /**
   * Resets the given counterparts from the other
   * counterparts.
   *
   * @param reset_trans 1 to reset the transient
   *   from main, 0 to reset main from transient.
   */ \
  void \
  sc##_selections_reset_counterparts ( \
    cc##Selections * self, \
    const int        reset_trans)

/**
 * Clone the struct for copying, undoing, etc.
 */
#define ARRANGER_SELECTIONS_DECLARE_CLONE(cc,sc) \
  cc##Selections * \
  sc##_selections_clone ( \
    const cc##Selections * mas)

/**
 * Returns if there are any selections.
 */
#define ARRANGER_SELECTIONS_DECLARE_HAS_ANY(cc,sc) \
  int \
  sc##_selections_has_any ( \
    cc##Selections * mas)

/**
 * Returns the position of the leftmost object.
 *
 * @param transient If 1, the transient objects are
 *   checked instead.
 * @param pos The return value will be stored here.
 * @param global Return global (timeline) Position,
 *   otherwise returns the local (from the start
 *   of the Region) Position.
 */
#define ARRANGER_SELECTIONS_DECLARE_GET_START_POS_W_GLOBAL( \
  cc,sc) \
  void \
  sc##_selections_get_start_pos ( \
    cc##Selections * mas, \
    Position *       pos, \
    int              transient, \
    int              global)

/**
 * Returns the position of the leftmost object.
 *
 * @param transient If 1, the transient objects are
 *   checked instead.
 * @param pos The return value will be stored here.
 */
#define ARRANGER_SELECTIONS_DECLARE_GET_START_POS( \
  cc,sc) \
  void \
  sc##_selections_get_start_pos ( \
    cc##Selections * mas, \
    Position *       pos, \
    int              transient)

/**
 * Returns the position of the rightmost object.
 *
 * @param pos The return value will be stored here.
 * @param transient If 1, the transient objects are
 * checked instead.
 * @param global Return global (timeline) Position,
 *   otherwise returns the local (from the start
 *   of the Region) Position.
 */
#define ARRANGER_SELECTIONS_DECLARE_GET_END_POS_W_GLOBAL( \
  cc,sc) \
  void \
  sc##_selections_get_end_pos ( \
    cc##Selections * mas, \
    Position *       pos, \
    int              transient, \
    int              global)

/**
 * Returns the position of the rightmost object.
 *
 * @param pos The return value will be stored here.
 * @param transient If 1, the transient objects are
 * checked instead.
 */
#define ARRANGER_SELECTIONS_DECLARE_GET_END_POS( \
  cc,sc) \
  void \
  sc##_selections_get_end_pos ( \
    cc##Selections * mas, \
    Position *       pos, \
    int              transient)

/**
 * Gets first object's widget.
 *
 * @param transient If 1, transient objects are
 *   checked instead.
 */
#define ARRANGER_SELECTIONS_DECLARE_GET_FIRST_OBJ( \
  cc,sc) \
  GtkWidget * \
  sc##_selections_get_first_object ( \
    cc##Selections * mas, \
    int              transient)

/**
 * Gets last object's widget.
 *
 * @param transient If 1, transient objects are
 *   checked instead.
 */
#define ARRANGER_SELECTIONS_DECLARE_GET_LAST_OBJ( \
  cc,sc) \
  GtkWidget * \
  sc##_selections_get_last_object ( \
    cc##Selections * mas, \
    int              transient)

/**
 * Pastes the given selections to the given Position.
 */
#define ARRANGER_SELECTIONS_DECLARE_PASTE_TO_POS( \
  cc,sc) \
  void \
  sc##_selections_paste_to_pos ( \
    cc##Selections * ts, \
    Position *       pos)

/**
 * Sets the cache Position's for each object in
 * the selection.
 *
 * Used by the ArrangerWidget's.
 */
#define ARRANGER_SELECTIONS_DECLARE_SET_CACHE_POSES( \
  cc,sc) \
  void \
  sc##_selections_set_cache_poses ( \
    cc##Selections * mas)

/**
 * Moves the selections by the given
 * amount of ticks.
 *
 * @param use_cached_pos Add the ticks to the cached
 *   Position's instead of the current Position's.
 * @param ticks Ticks to add.
 * @param update_flag ArrangerObjectUpdateFlag.
 */
#define ARRANGER_SELECTIONS_DECLARE_ADD_TICKS(cc,sc) \
  void \
  sc##_selections_add_ticks ( \
    cc##Selections *         mas, \
    long                     ticks, \
    int                      use_cached_pos, \
    ArrangerObjectUpdateFlag update_flag)

/**
 * Clears selections.
 */
#define ARRANGER_SELECTIONS_DECLARE_CLEAR(cc,sc) \
  void \
  sc##_selections_clear ( \
    cc##Selections * mas)

/**
 * Frees the selections.
 */
#define ARRANGER_SELECTIONS_DECLARE_FREE(cc,sc) \
  void \
  sc##_selections_free ( \
    cc##Selections * self)

/**
 * Frees all the objects as well.
 *
 * To be used in actions where the selections are
 * all clones.
 */
#define ARRANGER_SELECTIONS_DECLARE_FREE_FULL(cc,sc) \
  void \
  sc##_selections_free_full ( \
    cc##Selections * self)

/**
 * Declares all of the above functions.
 */
#define ARRANGER_SELECTIONS_DECLARE_FUNCS(cc,sc) \
  ARRANGER_SELECTIONS_DECLARE_RESET_COUNTERPARTS ( \
    cc, sc); \
  ARRANGER_SELECTIONS_DECLARE_INIT_LOADED (cc,sc); \
  ARRANGER_SELECTIONS_DECLARE_CLONE (cc,sc); \
  ARRANGER_SELECTIONS_DECLARE_HAS_ANY (cc,sc); \
  ARRANGER_SELECTIONS_DECLARE_GET_START_POS_W_GLOBAL (\
    cc,sc); \
  ARRANGER_SELECTIONS_DECLARE_GET_END_POS_W_GLOBAL (cc,sc); \
  ARRANGER_SELECTIONS_DECLARE_GET_FIRST_OBJ (cc,sc); \
  ARRANGER_SELECTIONS_DECLARE_GET_LAST_OBJ (cc,sc); \
  ARRANGER_SELECTIONS_DECLARE_PASTE_TO_POS (cc,sc); \
  ARRANGER_SELECTIONS_DECLARE_SET_CACHE_POSES ( \
    cc,sc); \
  ARRANGER_SELECTIONS_DECLARE_ADD_TICKS (cc,sc); \
  ARRANGER_SELECTIONS_DECLARE_CLEAR (cc,sc); \
  ARRANGER_SELECTIONS_DECLARE_FREE (cc,sc); \
  ARRANGER_SELECTIONS_DECLARE_FREE_FULL (cc,sc)

#define ARRANGER_SELECTIONS_DECLARE_TIMELINE_FUNCS( \
  cc,sc) \
  ARRANGER_SELECTIONS_DECLARE_RESET_COUNTERPARTS ( \
    cc, sc); \
  ARRANGER_SELECTIONS_DECLARE_INIT_LOADED (cc,sc); \
  ARRANGER_SELECTIONS_DECLARE_CLONE (cc,sc); \
  ARRANGER_SELECTIONS_DECLARE_HAS_ANY (cc,sc); \
  ARRANGER_SELECTIONS_DECLARE_GET_START_POS (cc,sc); \
  ARRANGER_SELECTIONS_DECLARE_GET_END_POS (cc,sc); \
  ARRANGER_SELECTIONS_DECLARE_GET_FIRST_OBJ (cc,sc); \
  ARRANGER_SELECTIONS_DECLARE_GET_LAST_OBJ (cc,sc); \
  ARRANGER_SELECTIONS_DECLARE_PASTE_TO_POS (cc,sc); \
  ARRANGER_SELECTIONS_DECLARE_SET_CACHE_POSES ( \
    cc,sc); \
  ARRANGER_SELECTIONS_DECLARE_ADD_TICKS (cc,sc); \
  ARRANGER_SELECTIONS_DECLARE_CLEAR (cc,sc); \
  ARRANGER_SELECTIONS_DECLARE_FREE (cc,sc); \
  ARRANGER_SELECTIONS_DECLARE_FREE_FULL (cc,sc)

/**
 * Adds the arranger object to the selections.
 */
#define ARRANGER_SELECTIONS_DECLARE_ADD_OBJ( \
  cc,sc,obj_cc,obj_sc) \
  void \
  sc##_selections_add_##obj_sc ( \
    cc##Selections * self, \
    obj_cc *         obj_sc)

/**
 * Returns if the arranger object is in the
 * selections or  not.
 *
 * The object must be the main object (see
 * ArrangerObjectInfo).
 */
#define ARRANGER_SELECTIONS_DECLARE_CONTAINS_OBJ( \
  cc,sc,obj_cc,obj_sc) \
  int \
  sc##_selections_contains_##obj_sc ( \
    cc##Selections * self, \
    obj_cc *        obj_sc)

/**
 * Removes the arranger object from the selections.
 */
#define ARRANGER_SELECTIONS_DECLARE_REMOVE_OBJ( \
  cc,sc,obj_cc,obj_sc) \
  void \
  sc##_selections_remove_##obj_sc ( \
    cc##Selections * ts, \
    obj_cc *         obj_sc)

/**
 * Declares the above arranger object-related
 * functions.
 */
#define ARRANGER_SELECTIONS_DECLARE_OBJ_FUNCS( \
  cc,sc,obj_cc,obj_sc) \
  ARRANGER_SELECTIONS_DECLARE_ADD_OBJ ( \
    cc, sc, obj_cc, obj_sc); \
  ARRANGER_SELECTIONS_DECLARE_CONTAINS_OBJ ( \
    cc, sc, obj_cc, obj_sc); \
  ARRANGER_SELECTIONS_DECLARE_REMOVE_OBJ ( \
    cc, sc, obj_cc, obj_sc)

#define ARRANGER_SELECTIONS_ADD_OBJECT( \
  _sel,caps,sc) \
  if (!array_contains ( \
         _sel->sc##s, _sel->num_##sc##s, sc)) \
    { \
      array_append ( \
        _sel->sc##s, _sel->num_##sc##s, sc); \
      EVENTS_PUSH ( \
        ET_ARRANGER_OBJECT_SELECTION_CHANGED, \
        &sc->obj_info); \
    }

#define ARRANGER_SELECTIONS_REMOVE_OBJECT( \
  _sel,caps,sc) \
  if (!array_contains ( \
         _sel->sc##s, _sel->num_##sc##s, sc)) \
    { \
      EVENTS_PUSH ( \
        ET_ARRANGER_OBJECT_SELECTION_CHANGED, \
        &sc->obj_info); \
      return; \
    } \
  array_delete ( \
    _sel->sc##s, _sel->num_##sc##s, sc)

/**
* @}
*/

#endif
