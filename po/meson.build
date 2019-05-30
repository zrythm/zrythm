i18n = import('i18n')

# define GETTEXT_PACKAGE
add_project_arguments (
  '-DGETTEXT_PACKAGE="' + meson.project_name () + '"',
  language:'c',
  )
i18n.gettext (
  meson.project_name(),
  args: [
    '--directory=' + meson.source_root(),
    '--msgid-bugs-address=https://git.zrythm.org/zrythm/zrythm/issues',
    ],
  preset: 'glib',
  )
