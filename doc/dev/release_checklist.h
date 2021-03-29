/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
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
 * \page release_checklist Release Checklist
 *
 * \section testing_before_release Testing Before Release
 *
 * 1. Check that both ALSA and JACK audio backends work
 *
 * \section release_checklist_checklist Checklist
 *
 * These are the steps to take before each release to
 * ensure that the program is releasable.
 * 1. Run the test suite locally *
 * 2. Check that it compiles with both gcc and clang *
 * 3. Check if the build compiles with --buildtype "debugoptimized" and "release"
 * 4. Switch to staging and update the changelog
 * using "version" as the version
 * 5. Follow the rest of the steps in git-packaging-hooks
 * 6. Test the Debian and Arch packages
 * 7. Merge staging back to master
 *
 * @note Steps marked with (*) are performed
 * automatically by git-packaging-hooks
 *
 * \subsection updating_changelog Updating the Changelog
 *
 * Run @code git log @endcode to see what changed
 * and make summarized human-readable entries in the
 * CHANGELOG.md file.
 *
 * \subsection running_test_suite Running the Test Suite Locally
 *
 * Run the test suite locally.
 *
 * \section after_release_checklist After-Release Checklist
 *
 * 1. Create a Savannah news post
 * 2. Scp the tarball and sig to Savannah downloads
 * 3. Rebuild the website to apply the new version and news, create a tag with the same version as zrythm
 * 4. Rebuild the manual to apply the new version, create a tag with the same version as zrythm
 * 5. git archive the manual and scp to Savannah downloads
 * 6. Announce to social media (Mastodon)
 * 7. If major version, announce to linux-audio-announce@lists.linuxaudio.org
 *
 */
