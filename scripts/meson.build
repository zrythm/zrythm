# Copyright (C) 2019-2022 Alexandros Theodotou <alex at zrythm dot org>
#
# This file is part of Zrythm
#
# Zrythm is free software: you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# Zrythm is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Affero General Public License for more details.
#
# You should have received a copy of the GNU Affero General Public License
# along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.

scripts_conf = {
  'SCRIPTS_DIR': meson.current_build_dir (),
  'MESON_BUILD_ROOT': meson_build_root,
  'MESON_SOURCE_ROOT': meson_src_root,
  'HOMEPAGE_URL': 'https://www.zrythm.org/en/index.html',
  'DONATION_URL': 'https://www.zrythm.org/en/community.html#donate',
  'BUG_REPORT_URL': 'https://todo.sr.ht/~alextee/zrythm-bug',
  'USER_MANUAL_URL': 'https://manual.zrythm.org/en/index.html',
  'TRANSLATE_URL': 'https://hosted.weblate.org/engage/zrythm/',
  'CONTACT_URL': 'https://www.zrythm.org/en/community.html#contact',
  'MAIN_SCREENSHOT_URL': 'https://www.zrythm.org/static/images/screenshots/mar-8-2022.png',
  'PIANO_ROLL_SCREENSHOT_URL': 'https://www.zrythm.org/static/images/screenshots/Screenshot_20220110_152402.png',
  'MIXER_SCREENSHOT_URL': 'https://www.zrythm.org/static/images/screenshots/Screenshot_20220110_152001.png',
  }
if os_windows
  scripts_conf += { 'GUILE': 'guile' }
else
  scripts_conf += { 'GUILE': guile.full_path() }
endif
if pandoc.found()
  scripts_conf += { 'PANDOC_BIN': pandoc.full_path() }
endif
if texi2html.found()
  scripts_conf += {
    'TEXI2HTML_BIN': texi2html.full_path() }
endif
if guile_snarf.found()
  scripts_conf += {
    'GUILE_SNARF': guile_snarf.full_path() }
endif
if get_option('guile_snarf_docs_path') != ''
  scripts_conf += {
    'GUILE_SNARF_DOCS_BIN': get_option (
      'guile_snarf_docs_path'),
    'GUILD_BIN': guild.full_path() }
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
  if script.endswith ('guile-gen-docs.scm')
    guile_gen_docs_scm = res
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

# ---- create wraps needed on windows mingw ----

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
gen_gtk_resources_xml_wrap = configure_file (
  output: 'gen-gtk-resources-xml.sh',
  input: generic_guile_wrap_name,
  configuration: scripts_conf + {
    'SCRIPT_NAME': 'gen-gtk-resources-xml.scm'},
  )

gen_appdata_xml_wrap = configure_file (
  output: 'gen-appdata-xml.sh',
  input: generic_guile_wrap_name,
  configuration: scripts_conf + {
    'SCRIPT_NAME': 'gen-appdata-xml.scm'},
  )

gen_completions_sh = configure_file (
  output: 'gen_completions.sh',
  input: 'gen_completions.sh',
  configuration: scripts_conf + {
    'BASH': bash.full_path (),
    'RUN_SH': manpage_completions_run_sh.full_path (), },
  )
