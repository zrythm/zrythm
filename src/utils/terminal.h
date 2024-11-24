/*
 * SPDX-FileCopyrightText: © 2020 Alexandros Theodotou <alex@zrythm.org>
 *
 * SPDX-License-Identifier: LicenseRef-ZrythmLicense
 */

/**
 * @file
 *
 * Terminal utilities.
 */

#ifndef __UTILS_TERMINAL_H__
#define __UTILS_TERMINAL_H__

/* ANSI color codes */
#define TERMINAL_COLOR_RED "\x1b[31m"
#define TERMINAL_COLOR_GREEN "\x1b[32m"
#define TERMINAL_COLOR_YELLOW "\x1b[33m"
#define TERMINAL_COLOR_LIGHT_YELLOW "\x1b[93m"
#define TERMINAL_COLOR_BLUE "\x1b[34m"
#define TERMINAL_COLOR_LIGHTBLUE "\x1b[94m"
#define TERMINAL_COLOR_MAGENTA "\x1b[35m"
#define TERMINAL_COLOR_LIGHT_PURPLE "\x1b[95m"
#define TERMINAL_COLOR_CYAN "\x1b[36m"

#define TERMINAL_BOLD "\x1b[1m"
#define TERMINAL_ITALIC "\x1b[3m"
#define TERMINAL_UNDERLINE "\x1b[4m"

#define TERMINAL_RESET "\x1b[0m"

#endif
