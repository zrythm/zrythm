/*
 * Copyright (C) 2019-2022 Alexandros Theodotou <alex at zrythm dot org>
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

/**
 * \file
 *
 * Flags.
 *
 * Z_F = Zrythm Flag.
 */

#ifndef __UTILS_FLAGS_H__
#define __UTILS_FLAGS_H__

/**
 * Used to select something instead of using 1s and
 * 0s. */
#define F_SELECT 1
#define F_NO_SELECT 0

#define F_TRANSIENTS 1
#define F_NO_TRANSIENTS 0

#define F_VISIBLE 1
#define F_NOT_VISIBLE 0

#define F_APPEND 1
#define F_NO_APPEND 0

#define F_FREE 1
#define F_NO_FREE 0

#define F_DELETE 1
#define F_NO_DELETE 0

#define F_PUBLISH_EVENTS 1
#define F_NO_PUBLISH_EVENTS 0

#define F_RECALC_GRAPH 1
#define F_NO_RECALC_GRAPH 0

#define F_CONFIRM 1
#define F_NO_CONFIRM 0

#define F_GEN_AUTOMATABLES 1
#define F_NO_GEN_AUTOMATABLES 0

#define F_REMOVE_PL 1
#define F_NO_REMOVE_PL 0

#define F_GEN_NAME 1
#define F_NO_GEN_NAME 0

#define F_CACHED 1
#define F_NO_CACHED 0

#define F_COPY_MOVING 1
#define F_NOT_COPY_MOVING 0

#define F_VALIDATE 1
#define F_NO_VALIDATE 0

#define F_GEN_WIDGET 1
#define F_NO_GEN_WIDGET 0

#define F_GEN_CURVE_POINTS 1
#define F_NO_GEN_CURVE_POINTS 0

#define F_MAIN 1
#define F_NOT_MAIN 0

#define F_PADDING 1
#define F_NO_PADDING 0

#define F_LOOP 1
#define F_NO_LOOP 0

#define F_WITH_LANE 1
#define F_WITHOUT_LANE 0

#define F_CONNECT 1
#define F_NO_CONNECT 0

#define F_DISCONNECT 1
#define F_NO_DISCONNECT 0

#define F_GLOBAL 1
#define F_NOT_GLOBAL 0

#define F_RESIZE 1
#define F_NO_RESIZE 0

#define F_SHRINK 1
#define F_NO_SHRINK 0

#define F_ALREADY_MOVED 1
#define F_NOT_ALREADY_MOVED 0

#define F_ALREADY_EDITED 1
#define F_NOT_ALREADY_EDITED 0

#define F_INCLUDE_PLUGINS 1
#define F_NO_INCLUDE_PLUGINS 0

#define F_ASYNC 1
#define F_NO_ASYNC 0

#define F_PANIC 1
#define F_NO_PANIC 0

#define F_SET_CUE_POINT 1
#define F_NO_SET_CUE_POINT 0

#define F_CLONE 1
#define F_NO_CLONE 0

#define F_IS_AFTER 1
#define F_NOT_IS_AFTER 0

#define F_RECURSIVE 1
#define F_NO_RECURSIVE 0

#define F_HAS_PADDING 1
#define F_NO_HAS_PADDING 0

#define F_NORMALIZED 1
#define F_NOT_NORMALIZED 0

#define F_USE_COLOR 1
#define F_NO_USE_COLOR 0

#define F_EXCLUSIVE 1
#define F_NOT_EXCLUSIVE 0

#define F_BACKUP 1
#define F_NOT_BACKUP 0

#define F_ACTIVATE 1
#define F_NO_ACTIVATE 0

#define F_TRIGGER_UNDO 1
#define F_NO_TRIGGER_UNDO 0

#define F_SOFT 1
#define F_NOT_SOFT 0

#define F_EXPAND 1
#define F_NO_EXPAND 0

#define F_FILL 1
#define F_NO_FILL 0

#define F_QUEUED 1
#define F_NOT_QUEUED 0

#define F_IN_RANGE 1
#define F_NOT_IN_RANGE 0

#define F_EXPOSE 1
#define F_NOT_EXPOSE 0

#define F_DELETING_PLUGIN 1
#define F_NOT_DELETING_PLUGIN 0

#define F_DELETING_CHANNEL 1
#define F_NOT_DELETING_CHANNEL 0

#define F_DELETING_TRACK 1
#define F_NOT_DELETING_TRACK 0

#define F_REPLACING 1
#define F_NOT_REPLACING 0

#define F_PROJECT 1
#define F_NOT_PROJECT 0

#define F_UNDOABLE 1
#define F_NOT_UNDOABLE 0

#define F_DRY_RUN 1
#define F_NOT_DRY_RUN 0

#define F_SERIALIZE 1
#define F_NO_SERIALIZE 0

#define F_CHECK_VALID 1
#define F_NO_CHECK_VALID 0

#define F_CHECK_BLACKLISTED 1
#define F_NO_CHECK_BLACKLISTED 0

#define F_PUSH_DOWN 1
#define F_NO_PUSH_DOWN 0

#define F_MULTI_SELECT 1
#define F_NO_MULTI_SELECT 0

#define F_DND 1
#define F_NO_DND 0

#define F_MUTE 1
#define F_NO_MUTE 0

#define F_SOLO 1
#define F_NO_SOLO 0

#define F_LISTEN 1
#define F_NO_LISTEN 0

#define F_SHOW_PROGRESS 1
#define F_NO_SHOW_PROGRESS 0

#define F_PARTS 1
#define F_NO_PARTS 0

#define F_IGNORE_FROZEN 1
#define F_NO_IGNORE_FROZEN 0

#define F_DUPLICATE_CLIP 1
#define F_NO_DUPLICATE_CLIP 0

#define F_WRITE_FILE 1
#define F_NO_WRITE_FILE 0

#define F_OPTIMIZED 1
#define F_NOT_OPTIMIZED 0

#define F_ASCENDING 1
#define F_NOT_ASCENDING 0

#define F_DESCENDING 1
#define F_NOT_DESCENDING 0

#define F_MOVING_PLUGIN 1
#define F_NOT_MOVING_PLUGIN 0

#define F_CANCELABLE 1
#define F_NOT_CANCELABLE 0

#define F_INCLUSIVE 1
#define F_NOT_INCLUSIVE 0

#define F_NORMALIZE 1
#define F_NO_NORMALIZE 0

#define F_DYNAMIC 1
#define F_NOT_DYNAMIC 0

#define F_EXPAND 1
#define F_NO_EXPAND 0

#define F_FILL 1
#define F_NO_FILL 0

#define F_UPDATE_AUTOMATION_TRACK 1
#define F_NO_UPDATE_AUTOMATION_TRACK 0

#define F_FORCE 1
#define F_NO_FORCE 0

#define F_FOLLOW_SYMLINKS 1
#define F_NO_FOLLOW_SYMLINKS 0

#define F_BOUNCE 1
#define F_NO_BOUNCE 0

#define F_MARK_REGIONS 1
#define F_NO_MARK_REGIONS 0

#define F_MARK_CHILDREN 1
#define F_NO_MARK_CHILDREN 0

#define F_MARK_PARENTS 1
#define F_NO_MARK_PARENTS 0

#define F_MARK_MASTER 1
#define F_NO_MARK_MASTER 0

#define F_LOCKED 1
#define F_NOT_LOCKED 0

#define F_AUTO_SELECT 1
#define F_NO_AUTO_SELECT 0

#define F_TOGGLE 1
#define F_NO_TOGGLE 0

#define F_INPUT 1
#define F_NOT_INPUT 0

#define F_MIDI 1
#define F_NOT_MIDI 0

#define F_ZERO_TERMINATED 1
#define F_NOT_ZERO_TERMINATED 0

#define F_CLEAR 1
#define F_NO_CLEAR 0

#define F_ENABLED 1
#define F_NOT_ENABLED 0

#define F_ENABLE 1
#define F_NO_ENABLE 0

#define F_AUTO_CLOSE 1
#define F_NO_AUTO_CLOSE 0

#define F_CANCELABLE 1
#define F_NOT_CANCELABLE 0

#define F_AUDITIONER 1
#define F_NOT_AUDITIONER 0

#define ZRYTHM_F_NOTIFY 1
#define ZRYTHM_F_NO_NOTIFY 0

#define Z_F_COPY 1
#define Z_F_NO_COPY 0

#define Z_F_PIN 1
#define Z_F_NO_PIN 0

#define Z_F_INSTANTIATE 1
#define Z_F_NO_INSTANTIATE 0

#define Z_F_AUTOMATING 1
#define Z_F_NOT_AUTOMATING 0

#define Z_F_TEMPORARY 1
#define Z_F_NOT_TEMPORARY 0

#define Z_F_DROP_UNNECESSARY 1
#define Z_F_NO_DROP_UNNECESSARY 0

#define Z_F_RECHAIN 1
#define Z_F_NO_RECHAIN 0

#define Z_F_TEMPLATE 1
#define Z_F_NOT_TEMPLATE 0

#define Z_F_EDITABLE 1
#define Z_F_NOT_EDITABLE 0

#define Z_F_RESIZABLE 1
#define Z_F_NOT_RESIZABLE 0

#define Z_F_LEFT 1
#define Z_F_NOT_LEFT 0

#define Z_F_DURING_UI_ACTION 1
#define Z_F_NOT_DURING_UI_ACTION 0

#endif
