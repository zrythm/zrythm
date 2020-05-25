# Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
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
  'MESON_BUILD_ROOT': meson.build_root (),
  'MESON_SOURCE_ROOT': meson.source_root (),
  }
if os_windows
  scripts_conf += { 'GUILE': 'guile' }
else
  scripts_conf += { 'GUILE': guile.path() }
endif
if pandoc.found()
  scripts_conf += { 'PANDOC_BIN': pandoc.path() }
endif
if texi2html.found()
  scripts_conf += {
    'TEXI2HTML_BIN': texi2html.path() }
endif
if guile_snarf.found()
  scripts_conf += {
    'GUILE_SNARF': guile_snarf.path() }
endif
if get_option('guile_snarf_docs_path') != ''
  scripts_conf += {
    'GUILE_SNARF_DOCS_BIN': get_option (
      'guile_snarf_docs_path'),
    'GUILD_BIN': guild.path() }
endif

# configure each guile script to build/scripts
src_scripts = run_command ('ls').stdout().strip().split('\n')
foreach script : src_scripts
  if script.endswith ('.scm')
    meson_post_install = configure_file (
      output: script,
      input: script,
      configuration: scripts_conf,
      )
  endif
endforeach

# ---- create wraps needed on windows mingw ----

generic_guile_wrap_name = 'generic_guile_wrap.sh'

# create the post install wrap
meson_post_install_wrap = configure_file (
  output: 'meson_post_install_wrap.sh',
  input: generic_guile_wrap_name,
  configuration: scripts_conf + {
    'SCRIPT_NAME': 'meson-post-install.scm'},
  )

# create the gschema generator wrap
gschema_gen_wrap = configure_file (
  output: 'gschema_gen_wrap.sh',
  input: generic_guile_wrap_name,
  configuration: scripts_conf + {
    'SCRIPT_NAME': 'gschema-gen.scm'},
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
