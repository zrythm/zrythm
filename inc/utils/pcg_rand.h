/*
 * SPDX-FileCopyrightText: © 2021 Alexandros Theodotou <alex@zrythm.org>
 *
 * SPDX-License-Identifier: LicenseRef-ZrythmLicense
 */

/**
 * \file
 *
 * PCGRandom float generator.
 */

#ifndef __UTILS_PCG_RAND_H__
#define __UTILS_PCG_RAND_H__

#include <cstdint>

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
