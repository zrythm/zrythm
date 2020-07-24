/*
 * Copyright (C) 2018-2020 Alexandros Theodotou <alex at zrythm dot org>
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

#ifndef __AUDIO_AUTOMATION_TRACK_H__
#define __AUDIO_AUTOMATION_TRACK_H__

#include "audio/automation_point.h"
#include "audio/port.h"
#include "audio/position.h"
#include "audio/region.h"

#define MAX_AUTOMATION_POINTS 1200

/** Relase time in ms when in touch record mode. */
#define AUTOMATION_RECORDING_TOUCH_REL_MS 800

typedef struct Port Port;
typedef struct _AutomationTrackWidget
  AutomationTrackWidget;
typedef struct Track Track;
typedef struct AutomationTracklist
  AutomationTracklist;
typedef struct CustomButtonWidget
  CustomButtonWidget;
typedef struct AutomationModeWidget
  AutomationModeWidget;

typedef enum AutomationMode
{
  AUTOMATION_MODE_READ,
  AUTOMATION_MODE_RECORD,
  AUTOMATION_MODE_OFF,
  NUM_AUTOMATION_MODES,
} AutomationMode;

static const cyaml_strval_t
automation_mode_strings[] =
{
  { "Read",     AUTOMATION_MODE_READ    },
  { "Rec",    AUTOMATION_MODE_RECORD   },
  { "Off",      AUTOMATION_MODE_OFF   },
  { "<invalid>",   NUM_AUTOMATION_MODES   },
};

typedef enum AutomationRecordMode
{
  AUTOMATION_RECORD_MODE_TOUCH,
  AUTOMATION_RECORD_MODE_LATCH,
  NUM_AUTOMATION_RECORD_MODES,
} AutomationRecordMode;

static const cyaml_strval_t
automation_record_mode_strings[] =
{
  { "Touch",     AUTOMATION_RECORD_MODE_TOUCH    },
  { "Latch",    AUTOMATION_RECORD_MODE_LATCH   },
  { "<invalid>",   NUM_AUTOMATION_RECORD_MODES   },
};

typedef struct AutomationTrack
{
  /** Index in parent AutomationTracklist. */
  int               index;

  /** Identifier of the Port this AutomationTrack
   * is for. */
  PortIdentifier    port_id;

  /** Whether it has been created by the user
   * yet or not. */
  bool              created;

  /** The automation Region's. */
  ZRegion **        regions;
  int               num_regions;
  size_t            regions_size;

  /**
   * Whether visible or not.
   *
   * Being created is a precondition for this.
   */
  bool              visible;

  /** Y local to track. */
  int               y;

  /** Position of multipane handle. */
  int               height;

  /** Last value recorded in this automation
   * track. */
  float             last_recorded_value;

  /** Automation mode. */
  AutomationMode    automation_mode;

  /** Automation record mode, when
   * \ref AutomationTrack.automation_mode is
   * set to record. */
  AutomationRecordMode    record_mode;

  /** To be set to true when recording starts
   * (when the first change is received) and
   * false when recording ends. */
  bool                 recording_started;

  /** Region currently recording to. */
  ZRegion *            recording_region;

  /** Buttons used by the track widget */
  CustomButtonWidget * top_right_buttons[8];
  int                  num_top_right_buttons;
  CustomButtonWidget * top_left_buttons[8];
  int                  num_top_left_buttons;
  CustomButtonWidget * bot_right_buttons[8];
  int                  num_bot_right_buttons;

  /** Automation mode button group. */
  AutomationModeWidget * am_widget;

  CustomButtonWidget * bot_left_buttons[8];
  int                  num_bot_left_buttons;

  /** The widget. */
  //AutomationTrackWidget * widget;
} AutomationTrack;

static const cyaml_schema_field_t
  automation_track_fields_schema[] =
{
  YAML_FIELD_INT (
    AutomationTrack, index),
  YAML_FIELD_MAPPING_EMBEDDED (
    AutomationTrack, port_id,
    port_identifier_fields_schema),
  YAML_FIELD_DYN_PTR_ARRAY_VAR_COUNT_OPT (
    AutomationTrack, regions, region_schema),
  YAML_FIELD_INT (
    AutomationTrack, created),
  YAML_FIELD_ENUM (
    AutomationTrack, automation_mode,
    automation_mode_strings),
  YAML_FIELD_INT (
    AutomationTrack, visible),
  YAML_FIELD_INT (
    AutomationTrack, height),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t
  automation_track_schema =
{
  CYAML_VALUE_MAPPING (
    CYAML_FLAG_POINTER,
    AutomationTrack,
    automation_track_fields_schema),
};

void
automation_track_init_loaded (
  AutomationTrack * self);

/**
 * Creates an automation track for the given
 * Port.
 */
AutomationTrack *
automation_track_new (
  Port * port);

/**
 * Gets the automation mode as a localized string.
 */
void
automation_mode_get_localized (
  AutomationMode mode,
  char *         buf);

/**
 * Gets the automation mode as a localized string.
 */
void
automation_record_mode_get_localized (
  AutomationRecordMode mode,
  char *         buf);

/**
 * @note This is expensive and should only be used
 *   if \ref PortIdentifier.at_idx is not set. Use
 *   port_get_automation_track() instead.
 *
 * @param basic_search If true, only basic port
 *   identifier members are checked.
 */
AutomationTrack *
automation_track_find_from_port_id (
  PortIdentifier * id,
  bool             basic_search);

/**
 * Finds the AutomationTrack associated with
 * `port`.
 *
 * @param track The track that owns the port, if
 *   known.
 */
AutomationTrack *
automation_track_find_from_port (
  Port *  port,
  Track * track,
  bool    basic_search);

static inline void
automation_track_swap_record_mode (
  AutomationTrack * self)
{
  self->record_mode =
    (self->record_mode + 1) %
      NUM_AUTOMATION_RECORD_MODES;
}

AutomationTracklist *
automation_track_get_automation_tracklist (
  AutomationTrack * self);

/**
 * Returns whether the automation in the automation
 * track should be read.
 *
 * @param cur_time Current time from
 *   g_get_monotonic_time() passed here to ensure
 *   the same timestamp is used in sequential calls.
 */
bool
automation_track_should_read_automation (
  AutomationTrack * at,
  gint64            cur_time);

/**
 * Returns if the automation track should currently
 * be recording data.
 *
 * Returns false if in touch mode after the release
 * time has passed.
 *
 * This function assumes that the transport will
 * be rolling.
 *
 * @param cur_time Current time from
 *   g_get_monotonic_time() passed here to ensure
 *   the same timestamp is used in sequential calls.
 * @param record_aps If set to true, this function
 *   will return whether we should be recording
 *   automation point data. If set to false, this
 *   function will return whether we should be
 *   recording a region (eg, if an automation point
 *   was already created before and we are still
 *   recording inside a region regardless of whether
 *   we should create/edit automation points or not.
 */
bool
automation_track_should_be_recording (
  AutomationTrack * at,
  gint64            cur_time,
  bool              record_aps);

/**
 * Adds an automation ZRegion to the AutomationTrack.
 *
 * @note This must not be used directly. Use
 *   track_add_region() instead.
 */
void
automation_track_add_region (
  AutomationTrack * self,
  ZRegion *          region);

/**
 * Returns the visible y offset from the top of
 * the track widget.
 */
//int
//automation_track_get_visible_y_offset (
  //AutomationTrack * self);

/**
 * Updates the frames of each position in each child
 * of the automation track recursively.
 */
void
automation_track_update_frames (
  AutomationTrack * self);

/**
 * Sets the index of the AutomationTrack in the
 * AutomationTracklist.
 */
void
automation_track_set_index (
  AutomationTrack * self,
  int               index);

/**
 * Clones the AutomationTrack.
 */
AutomationTrack *
automation_track_clone (
  AutomationTrack * src);

Track *
automation_track_get_track (
  AutomationTrack * self);

/**
 * Frees the automation track.
 */
void
automation_track_free (AutomationTrack * at);

/**
 * Returns the automation point before the pos.
 */
AutomationPoint *
automation_track_get_ap_before_pos (
  const AutomationTrack * self,
  const Position *        pos);

/**
 * Returns the ZRegion that surrounds the
 * given Position, if any.
 */
ZRegion *
automation_track_get_region_before_pos (
  const AutomationTrack * self,
  const Position *        pos);

/**
 * Unselects all arranger objects.
 */
void
automation_track_unselect_all (
  AutomationTrack * self);

/**
 * Removes all objects recursively.
 */
void
automation_track_clear (
  AutomationTrack * self);

/**
 * Returns the actual parameter value at the given
 * position.
 *
 * If there is no automation point/curve during
 * the position, it returns the current value
 * of the parameter it is automating.
 *
 * @param normalized Whether to return the value
 *   normalized.
 */
float
automation_track_get_val_at_pos (
  AutomationTrack * at,
  Position *        pos,
  bool              normalized);

/**
 * Returns the y pixels from the value based on the
 * allocation of the automation track.
 */
int
automation_track_get_y_px_from_normalized_val (
  AutomationTrack * self,
  float             normalized_val);

Port *
automation_track_get_port (
  AutomationTrack * self);

/**
 * Updates automation track & its GUI
 */
//void
//automation_track_update (AutomationTrack * at);

/**
 * Gets the last ZRegion in the AutomationTrack.
 */
ZRegion *
automation_track_get_last_region (
  AutomationTrack * self);

#endif // __AUDIO_AUTOMATION_TRACK_H__
