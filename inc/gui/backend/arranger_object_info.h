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

/**
 * \file
 *
 * Info about objects shown in arrangers (Region's,
 * MidiNote's, etc.).
 */

#ifndef __GUI_BACKEND_ARRANGER_OBJECT_INFO_H__
#define __GUI_BACKEND_ARRANGER_OBJECT_INFO_H__

/**
 * The type of the object.
 */
typedef enum ArrangerObjectInfoType
{
  AOI_TYPE_REGION,
  AOI_TYPE_MIDI_NOTE,
  AOI_TYPE_CHORD_OBJECT,
  AOI_TYPE_SCALE_OBJECT,
  AOI_TYPE_MARKER,
  AOI_TYPE_AUTOMATION_POINT,
} ArrangerObjectInfoType;


typedef enum ArrangerObjectInfoCounterpart
{
  /** This is the main (non-lane) object. */
  AOI_COUNTERPART_MAIN,
  /** This is the transient of the main object. */
  AOI_COUNTERPART_MAIN_TRANSIENT,
  /** This is the lane object. */
  AOI_COUNTERPART_LANE,
  /** This is the lane transient object. */
  AOI_COUNTERPART_LANE_TRANSIENT,
} ArrangerObjectInfoCounterpart;

/**
 * Information about the objects associated with
 * the current object.
 *
 * Each arranger object (Region's, MidiNote's, etc.)
 * should have a copy of this struct and indicate
 * all associated objects here. The type is used to
 * determine which object the current one is.
 */
typedef struct ArrangerObjectInfo
{
  /** Main (non-lane) object. */
  void *         main;

  /** Transient of the main object. */
  void *         main_trans;

  /** Lane object. */
  void *         lane;

  /** Lane transient object. */
  void *         lane_trans;

  /** Type of object this is. */
  ArrangerObjectInfoType type;

  /** Type of object this is. */
  ArrangerObjectInfoCounterpart counterpart;

} ArrangerObjectInfo;

#define arranger_object_info_init( \
  _self, _main, _main_trans, _lane, _lane_trans, \
  _type, _counterpart) \
  _arranger_object_info_init ( \
    _self, (void *) _main, (void *) _main_trans, \
    (void *) _lane, (void *) _lane_trans, _type, \
    _counterpart)

/** Initializes each object starting from the
 * main. */
#define arranger_object_info_init_main( \
  _main, _main_trans, _lane, _lane_trans, _type) \
  _arranger_object_info_init ( \
    &_main->obj_info, \
    _main, _main_trans, _lane, _lane_trans, \
    _type, AOI_COUNTERPART_MAIN); \
  _arranger_object_info_init ( \
    &_main_trans->obj_info, \
    _main, _main_trans, _lane, _lane_trans, \
    _type, AOI_COUNTERPART_MAIN_TRANSIENT); \
  _arranger_object_info_init ( \
    &_lane->obj_info, \
    _main, _main_trans, _lane, _lane_trans, \
    _type, AOI_COUNTERPART_LANE); \
  _arranger_object_info_init ( \
    &_lane_trans->obj_info, \
    _main, _main_trans, _lane, _lane_trans, \
    _type, AOI_COUNTERPART_LANE_TRANSIENT)

/**
 * Sets the Position pos to the earliest or latest
 * object.
 *
 * @param _array The array of objects to check.
 * @param cc Camel case.
 * @param lc Lower case.
 * @param pos_name Variable name of position to check
 * @param bef_or_aft Before or after
 * @param widg Widget to set.
 */
#define SET_ARRANGER_OBJ_POS_TO(_array,cc,lc,pos_name,transient,bef_or_aft,widg) \
  cc * lc; \
  for (i = 0; i < _array->num_##lc##s; i++) \
    { \
      if (transient) \
        lc = _array->lc##s[i]->obj_info.main_trans; \
      else \
        lc = _array->lc##s[i]->obj_info.main; \
      if (position_is_##bef_or_aft ( \
            &lc->pos_name, pos)) \
        { \
          position_set_to_pos ( \
            pos, &lc->pos_name); \
          widget = GTK_WIDGET (lc->widget); \
        } \
    }

/**
 * Inits the ArrangerObjectInfo with the given
 * values.
 */
static inline void
_arranger_object_info_init (
  ArrangerObjectInfo * self,
  void * main,
  void * main_trans,
  void * lane,
  void * lane_trans,
  ArrangerObjectInfoType type,
  ArrangerObjectInfoCounterpart counterpart)
{
  self->main = main;
  self->main_trans = main_trans;
  self->lane = lane;
  self->lane_trans = lane_trans;
  self->type = type;
  self->counterpart = counterpart;
}

/**
 * Returns whether the object is transient or not.
 *
 * Transient objects are objects that are used
 * during moving operations.
 */
int
arranger_object_info_is_transient (
  ArrangerObjectInfo * self);

/**
 * Returns whether the object is a lane object or not
 * (only applies to TimelineArrangerWidget objects.
 */
int
arranger_object_info_is_lane (
  ArrangerObjectInfo * self);

/**
 * Returns whether the object is a main object or not
 * (only applies to TimelineArrangerWidget objects.
 */
static inline int
arranger_object_info_is_main (
  ArrangerObjectInfo * self)
{
  return (
    self->counterpart ==
      AOI_COUNTERPART_MAIN ||
    self->counterpart ==
      AOI_COUNTERPART_MAIN_TRANSIENT);
}

/**
 * Returns if the object represented by the
 * ArrrangerObjectInfo should be visible.
 *
 * This is counterpart-specific, ie. it checks
 * if the transient should be visible or lane
 * counterpart should be visible, etc., based on
 * what is given.
 */
int
arranger_object_info_should_be_visible (
  ArrangerObjectInfo * self);

/**
 * Gets the object the ArrangerObjectInfo
 * represents.
 */
static inline void *
arranger_object_info_get_object (
  ArrangerObjectInfo * self)
{
  switch (self->counterpart)
    {
    case AOI_COUNTERPART_MAIN:
      return self->main;
    case AOI_COUNTERPART_MAIN_TRANSIENT:
      return self->main_trans;
    case AOI_COUNTERPART_LANE:
      return self->lane;
    case AOI_COUNTERPART_LANE_TRANSIENT:
      return self->lane_trans;
    }

  g_return_val_if_reached (NULL);
}

/**
 * Sets the widget visibility to this counterparty
 * only, or to all counterparties if all is 1.
 */
void
arranger_object_info_set_widget_visibility (
  ArrangerObjectInfo * self,
  int                  all);

/**
 * Returns the first visible counterpart found.
 */
void *
arranger_object_info_get_visible_counterpart (
  ArrangerObjectInfo * self);

#endif
