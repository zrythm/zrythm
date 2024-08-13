// SPDX-FileCopyrightText: © 2019-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * @file
 *
 * Zrythm test helper.
 */

#ifndef __TEST_HELPERS_ZRYTHM_H__
#define __TEST_HELPERS_ZRYTHM_H__

#include "zrythm-test-config.h"

/* include commonly used stuff */
#include "project.h"
#include "project/project_init_flow_manager.h"
#include "utils/backtrace.h"
#include "utils/cairo.h"
#include "utils/datetime.h"
#include "utils/flags.h"
#include "utils/io.h"
#include "utils/logger.h"
#include "utils/objects.h"
#include "utils/ui.h"
#include "zrythm.h"
#include "zrythm_app.h"

#include <glib.h>
#include <glib/gi18n.h>

#ifdef G_OS_UNIX
#  include <glib-unix.h>
#endif

/**
 * @addtogroup tests
 *
 * @{
 */

void
_test_helper_zrythm_init (
  bool optimized,
  int  samplerate,
  int  buf_size,
  bool use_pipewire);
void
test_helper_zrythm_init ();
void
test_helper_zrythm_init_with_pipewire ();
void
test_helper_zrythm_init_optimized ();
void
test_helper_zrythm_cleanup ();
void
test_helper_zrythm_gui_init (int argc, char * argv[]);
void
test_helper_project_init_done_cb (
  bool        success,
  std::string error,
  void *      user_data);

/** Time to run fishbowl, in seconds */
#define DEFAULT_FISHBOWL_TIME 20

/** Compares 2 Position pointers. */
#define g_assert_cmppos(a, b) \
  REQUIRE ((a)->ticks_ == doctest::Approx ((b)->ticks_).epsilon (0.0001)); \
  REQUIRE ((a)->frames_ == (b)->frames_)

#ifndef G_APPROX_VALUE
#  define G_APPROX_VALUE(a, b, epsilon) \
    (((a) > (b) ? (a) - (b) : (b) - (a)) < (epsilon))
#endif

#ifndef REQUIRE_FLOAT_NEAR
#  define REQUIRE_FLOAT_NEAR(n1, n2, epsilon) \
    G_STMT_START \
    { \
      double __n1 = (n1), __n2 = (n2), __epsilon = (epsilon); \
      if (G_APPROX_VALUE (__n1, __n2, __epsilon)) \
        ; \
      else \
        g_assertion_message_cmpnum ( \
          G_LOG_DOMAIN, __FILE__, __LINE__, G_STRFUNC, \
          #n1 " == " #n2 " (+/- " #epsilon ")", __n1, "==", __n2, 'f'); \
    } \
    G_STMT_END
#endif

#ifndef g_assert_cmpfloat
#  define g_assert_cmpfloat(n1, n2) \
    G_STMT_START \
    { \
      double __n1 = (n1), __n2 = (n2), __epsilon = (FLT_EPSILON); \
      if (G_APPROX_VALUE (__n1, __n2, __epsilon)) \
        ; \
      else \
        g_assertion_message_cmpnum ( \
          G_LOG_DOMAIN, __FILE__, __LINE__, G_STRFUNC, \
          #n1 " == " #n2 " (+/- " #epsilon ")", __n1, "==", __n2, 'f'); \
    } \
    G_STMT_END
#endif

#ifndef g_assert_cmpenum
#  define g_assert_cmpenum(a, b, c) g_assert_cmpint ((int) (a), b, (int) (c))
#endif

/**
 * @}
 */

#endif
