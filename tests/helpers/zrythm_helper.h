// SPDX-FileCopyrightText: © 2019-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __TEST_HELPERS_ZRYTHM_H__
#define __TEST_HELPERS_ZRYTHM_H__

#include "zrythm-test-config.h"

/* include commonly used stuff */
#include "common/utils/gtest_wrapper.h"
#include "gui/backend/backend/zrythm.h"

#ifdef G_OS_UNIX
#  include <glib-unix.h>
#endif

/**
 * @brief A fixture class for Zrythm tests.
 *
 * This class provides a common setup and teardown for Zrythm tests. It can be
 * used as a base class for test cases that require a Zrythm environment.
 *
 * The constructor takes parameters to control the behavior of the Zrythm
 * environment, such as whether to use an optimized configuration or
 * PipeWire.
 */
class ZrythmFixture : virtual public ::testing::Test
{
public:
  // To be used in most tests as a fixture.
  ZrythmFixture () : ZrythmFixture (false, 0, 0, false, true) { }

  // To be used when more control is needed.
  ZrythmFixture (
    bool optimized,
    int  samplerate,
    int  buf_size,
    bool use_pipewire,
    bool logging_enabled);
  virtual ~ZrythmFixture ();

  void SetUp () override;
  void TestBody () override;
  void TearDown () override;

private:
  bool optimized_;
  int  samplerate_;
  int  buf_size_;
  bool use_pipewire_;
  bool logging_enabled_;
};

class ZrythmFixtureWithPipewire : public ZrythmFixture
{
public:
  ZrythmFixtureWithPipewire () : ZrythmFixture (false, 0, 0, true, true) { }
};

class ZrythmFixtureOptimized : public ZrythmFixture
{
public:
  ZrythmFixtureOptimized () : ZrythmFixture (true, 0, 0, false, true) { }
};

void
test_helper_zrythm_gui_init (int argc, char * argv[]);
void
test_helper_project_init_done_cb (
  bool        success,
  std::string error,
  void *      user_data);

/** Time to run fishbowl, in seconds */
#define DEFAULT_FISHBOWL_TIME 20

/**
 * @}
 */

#endif
