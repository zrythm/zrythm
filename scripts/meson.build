# SPDX-FileCopyrightText: © 2019-2024 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

scripts_conf = {
  'SCRIPTS_DIR': meson.current_build_dir (),
  'MESON_BUILD_ROOT': meson_build_root,
  'MESON_SOURCE_ROOT': meson_src_root,
  'HOMEPAGE_URL': 'https://www.zrythm.org/en/index.html',
  'DONATION_URL': 'https://www.zrythm.org/en/community.html#donate',
  'BUG_REPORT_URL': issue_tracker_url,
  'CHATROOM_URL': chatroom_url,
  'FAQ_URL': faq_url,
  'USER_MANUAL_URL': user_manual_url,
  'TRANSLATE_URL': 'https://hosted.weblate.org/engage/zrythm/',
  'CONTACT_URL': 'https://www.zrythm.org/en/community.html#contact',
  'MAIN_SCREENSHOT_URL': 'https://www.zrythm.org/static/images/screenshots/screenshot-20240208.png',
  'RELEASE_TAG_BASE_URL': 'https://gitlab.zrythm.org/zrythm/zrythm/-/tags/'
  }
if os_windows
  scripts_conf += { 'GUILE': 'guile' }
else
  scripts_conf += { 'GUILE': guile.full_path() }
endif

# configure each guile script to build/scripts
src_scripts = run_command ('ls').stdout().strip().split('\n')
foreach script : src_scripts
  if script.endswith ('.scm')
    res = configure_file (
      output: script,
      input: script,
      configuration: scripts_conf,
      )
  endif
endforeach

# configure gen-schema.scm
locales_for_gen_schema_scm = '"' + '" "'.join(locales_str.split(' ')) + '"'
configure_file (
  output: 'gen-schema.scm',
  input: 'gen-schema.scm.in',
  configuration: scripts_conf + {
    'LINGUAS': locales_for_gen_schema_scm },
  )

# ----
# Create shell-script wrappers that execute the Guile scripts.
#
# This is to avoid errors on platforms like MingW when running Guile scripts
# directly.
# ----

generic_guile_wrap_name = 'generic_guile_wrap.sh'

# create the gschema generator wrap
gen_gschema_wrap = configure_file (
  output: 'gen_gschema_wrap.sh',
  input: generic_guile_wrap_name,
  configuration: scripts_conf + {
    'SCRIPT_NAME': 'gen-schema.scm'},
  )

# create the translators C-string header generator
# wrap
translators_c_header_gen_wrap = configure_file (
  output: 'translators_c_header_gen_wrap.sh',
  input: generic_guile_wrap_name,
  configuration: scripts_conf + {
    'SCRIPT_NAME': 'gen-translators-list.scm'},
  )

# create the gtk resources generator wrap
src_script = 'gen-gtk-resources-xml.scm'
gen_gtk_resources_xml_wrap = configure_file (
  output: 'gen-gtk-resources-xml.sh',
  input: generic_guile_wrap_name,
  depfile: src_script,
  configuration: scripts_conf + {
    'SCRIPT_NAME': src_script },
  )

src_script = 'gen-appdata-xml.scm'
gen_appdata_xml_wrap = configure_file (
  output: 'gen-appdata-xml.sh',
  input: generic_guile_wrap_name,
  depfile: src_script,
  configuration: scripts_conf + {
    'SCRIPT_NAME': src_script },
  )

gen_completions_sh = configure_file (
  output: 'gen_completions.sh',
  input: 'gen_completions.sh',
  configuration: scripts_conf + {
    'BASH': bash.full_path (),
    'RUN_SH': manpage_completions_run_sh.full_path (), },
  )
