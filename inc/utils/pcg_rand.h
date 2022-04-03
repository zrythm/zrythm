/*
 * Copyright (C) 2021 Alexandros Theodotou <alex at zrythm dot org>
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
 * PCGRandom float generator.
 */

#ifndef __UTILS_PCG_RAND_H__
#define __UTILS_PCG_RAND_H__

#include <stdint.h>

typedef struct PCGRand PCGRand;

PCGRand *
pcg_rand_new (void);

/* unsigned float [0..1] */
float
pcg_rand_uf (PCGRand * self);

/* signed float [-1..+1] */
float
pcg_rand_sf (PCGRand * self);

uint32_t
pcg_rand_u32 (PCGRand * self);

#endif
