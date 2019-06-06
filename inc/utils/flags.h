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

#define F_TRANS_ONLY 1
#define F_NO_TRANS_ONLY 0

#define F_USE_CACHED 1
#define F_NO_USE_CACHED 0

#endif
