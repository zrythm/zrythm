// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __CARLA_WRAPPER_H__
#define __CARLA_WRAPPER_H__

#include "zrythm-config.h"

#if HAVE_CARLA

#  include <CarlaBackend.h>
#  include <CarlaHost.h>
#  include <CarlaNative.h>
#  include <CarlaNativePlugin.h>
#  include <CarlaUtils.h>

/* Carla defines unlikely, which clashes with C++20 [[unlikely]] */
#  undef unlikely

#endif // HAVE_CARLA

#endif // __CARLA_WRAPPER_H__