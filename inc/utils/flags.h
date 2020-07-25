/*
 * Copyright (C) 2019-2020 Alexandros Theodotou <alex at zrythm dot org>
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

/** \file */

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
#define F_IS_NOT_AFTER 0

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

#endif
