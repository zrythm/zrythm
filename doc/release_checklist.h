/**
 * \page release_checklist Release Checklist
 *
 * \section release_checklist_checklist Checklist
 *
 * These are the steps to take before each release to
 * ensure that the program is releasable.
 * 1. Run the test suite locally *
 * 2. Check that it compiles with both gcc and clang *
 * 3. Follow the steps in git-packaging-hooks for the
 * staging branch
 * 4. Update and commit the changelog in the staging branch
 * 5. Follow the steps in git-packaging-hooks for the
 * packaging branch
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
 * 3. Rebuild the website to apply the new version and news
 * 4. Rebuild the manual to apply the new version
 * 5. If major version, announce to linux-audio-announce@lists.linuxaudio.org
 *
 */
