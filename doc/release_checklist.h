/**
 * \page release_checklist Release Checklist
 *
 * \section release_checklist_checklist Checklist
 *
 * These are the steps to take before each release to
 * ensure that the program is releasable.
 * 1. Run the test suite locally
 * 2. Check that it compiles with both gcc and clang
 * 3. Update the changelog
 * 4. Follow the steps in git-packaging-hooks
 * 5. Test the Debian and Arch packages
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
