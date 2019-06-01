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
 * FIXME rename to something better.
 */
typedef enum ArrangerObjectInfoType
{
  /** This is the main (non-lane) object. */
  AOI_TYPE_MAIN,
  /** This is the transient of the main object. */
  AOI_TYPE_MAIN_TRANSIENT,
  /** This is the lane object. */
  AOI_TYPE_LANE,
  /** This is the lane transient object. */
  AOI_TYPE_LANE_TRANSIENT,
} ArrangerObjectInfoType;

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

} ArrangerObjectInfo;

#define arranger_object_info_init( \
  _self, _main, _main_trans, _lane, _lane_trans, \
  type) \
  _arranger_object_info_init ( \
    _self, (void *) _main, (void *) _main_trans, \
    (void *) _lane, (void *) _lane_trans, type)
#define arranger_object_info_init_main( \
  _self, _main, _main_trans, _lane, _lane_trans) \
  _arranger_object_info_init ( \
    _self, _main, _main_trans, _lane, _lane_trans, \
    AOI_TYPE_MAIN)

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
  ArrangerObjectInfoType type)
{
  self->main = main;
  self->main_trans = main_trans;
  self->lane = lane;
  self->lane_trans = lane_trans;
  self->type = type;
}

/**
 * Returns whether the object is transient or not.
 *
 * Transient objects are objects that are used
 * during moving operations.
 */
static inline int
arranger_object_info_is_transient (
  ArrangerObjectInfo * self)
{
  return (self->type == AOI_TYPE_MAIN_TRANSIENT ||
          self->type == AOI_TYPE_LANE_TRANSIENT);
}

/**
 * Returns whether the object is a lane object or not
 * (only applies to TimelineArrangerWidget objects.
 */
static inline int
arranger_object_info_is_lane (
  ArrangerObjectInfo * self)
{
  return (self->type == AOI_TYPE_LANE ||
          self->type == AOI_TYPE_LANE_TRANSIENT);
}

/**
 * Returns whether the object is a main object or not
 * (only applies to TimelineArrangerWidget objects.
 */
static inline int
arranger_object_info_is_main (
  ArrangerObjectInfo * self)
{
  return (self->type == AOI_TYPE_MAIN ||
          self->type == AOI_TYPE_MAIN_TRANSIENT);
}

#endif
