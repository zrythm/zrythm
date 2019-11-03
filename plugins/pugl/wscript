#!/usr/bin/env python

import os
import sys

from waflib import Logs, Options, TaskGen
from waflib.extras import autowaf

# Library and package version (UNIX style major, minor, micro)
# major increment <=> incompatible changes
# minor increment <=> compatible changes (additions)
# micro increment <=> no interface changes
PUGL_VERSION       = '0.2.0'
PUGL_MAJOR_VERSION = '0'

# Mandatory waf variables
APPNAME = 'pugl'        # Package name for waf dist
VERSION = PUGL_VERSION  # Package version for waf dist
top     = '.'           # Source directory
out     = 'build'       # Build directory


def options(ctx):
    ctx.load('compiler_c')

    opts = ctx.configuration_options()
    opts.add_option('--target', default=None, dest='target',
                    help='target platform (e.g. "win32" or "darwin")')

    ctx.add_flags(
        opts,
        {'all-headers': 'install complete header implementation',
         'no-gl':       'do not build OpenGL support',
         'no-cairo':    'do not build Cairo support',
         'no-static':   'do not build static library',
         'no-shared':   'do not build shared library',
         'log':         'print GL information to console',
         'grab-focus':  'work around keyboard issues by grabbing focus'})


def configure(conf):
    conf.env.ALL_HEADERS     = Options.options.all_headers
    conf.env.TARGET_PLATFORM = Options.options.target or sys.platform
    conf.load('compiler_c', cache=True)
    conf.load('autowaf', cache=True)

    if conf.env.TARGET_PLATFORM == 'win32':
        if conf.env.MSVC_COMPILER:
            conf.env.append_unique('CFLAGS', ['/wd4191'])
    elif conf.env.TARGET_PLATFORM == 'darwin':
        conf.env.append_unique('CFLAGS', ['-Wno-deprecated-declarations'])

    if not conf.env.MSVC_COMPILER:
        conf.env.append_value('LINKFLAGS', ['-fvisibility=hidden'])
        for f in ('CFLAGS', 'CXXFLAGS'):
            conf.env.append_value(f, ['-fvisibility=hidden'])
            if Options.options.strict:
                conf.env.append_value(f, ['-Wunused-parameter', '-Wno-pedantic'])

    autowaf.set_c_lang(conf, 'c99')

    if not Options.options.no_gl:
        # TODO: Portable check for OpenGL
        conf.define('HAVE_GL', 1)
        conf.define('PUGL_HAVE_GL', 1)

    conf.check(features='c cshlib', lib='m', uselib_store='M', mandatory=False)
    conf.check(features='c cshlib', lib='dl', uselib_store='DL', mandatory=False)

    if not Options.options.no_cairo:
        autowaf.check_pkg(conf, 'cairo',
                          uselib_store    = 'CAIRO',
                          atleast_version = '1.0.0',
                          mandatory       = False)
        if conf.is_defined('HAVE_CAIRO'):
            conf.define('PUGL_HAVE_CAIRO', 1)

    if Options.options.log:
        conf.define('PUGL_VERBOSE', 1)

    # Shared library building is broken on win32 for some reason
    conf.env.update({
        'BUILD_SHARED': not Options.options.no_shared,
        'BUILD_STATIC': conf.env.BUILD_TESTS or not Options.options.no_static})

    autowaf.set_lib_env(conf, 'pugl', PUGL_VERSION)
    conf.write_config_header('pugl_config.h', remove=False)

    autowaf.display_summary(
        conf,
        {"Build static library":   bool(conf.env.BUILD_STATIC),
         "Build shared library":   bool(conf.env.BUILD_SHARED),
         "OpenGL support":         conf.is_defined('HAVE_GL'),
         "Cairo support":          conf.is_defined('HAVE_CAIRO'),
         "Verbose console output": conf.is_defined('PUGL_VERBOSE')})


def _build_pc_file(bld, name, desc, target, libname, deps={}, requires=[]):
    "Builds a pkg-config file for a library"
    env = bld.env
    prefix = env.PREFIX
    xprefix = os.path.dirname(env.LIBDIR)

    uselib   = deps.get('uselib', [])
    pkg_deps = [l for l in uselib if 'PKG_' + l.lower() in env]
    lib_deps = [l for l in uselib if 'PKG_' + l.lower() not in env]

    link_flags = [env.LIB_ST % l for l in (deps.get('lib', []) + [libname])]
    for l in lib_deps:
        link_flags += [env.LIB_ST % l for l in env['LIB_' + l]]
    for f in deps.get('framework', []):
        link_flags += ['-framework', f]

    bld(features='subst',
        source='pugl.pc.in',
        target='%s-%s.pc' % (target, PUGL_MAJOR_VERSION),
        install_path=os.path.join(env.LIBDIR, 'pkgconfig'),

        PREFIX=prefix,
        EXEC_PREFIX='${prefix}' if xprefix == prefix else xprefix,
        LIBDIR='${exec_prefix}/' + os.path.basename(env.LIBDIR),
        INCLUDEDIR=env.INCLUDEDIR.replace(prefix, '${prefix}', 1),

        NAME=name,
        DESCRIPTION=desc,
        PUGL_MAJOR_VERSION=PUGL_MAJOR_VERSION,
        REQUIRES=' '.join(requires + [p.lower() for p in pkg_deps]),
        LIBS=' '.join(link_flags))


def build(bld):
    # C Headers
    includedir = '${INCLUDEDIR}/pugl-%s/pugl' % PUGL_MAJOR_VERSION
    bld.install_files(includedir, bld.path.ant_glob('pugl/*.h'))
    bld.install_files(includedir, bld.path.ant_glob('pugl/*.hpp'))
    if bld.env.ALL_HEADERS:
        detaildir = os.path.join(includedir, 'detail')
        bld.install_files(detaildir, bld.path.ant_glob('pugl/detail/*.h'))
        bld.install_files(detaildir, bld.path.ant_glob('pugl/detail/*.c'))

    # Library dependencies of pugl libraries (for buiding tests)
    deps = {}

    def build_pugl_lib(name, **kwargs):
        deps[name] = {}
        for k in ('lib', 'framework', 'uselib'):
            deps[name][k] = kwargs.get(k, [])

        args = kwargs.copy()
        args.update({'includes':        ['.'],
                     'export_includes': ['.'],
                     'install_path':    '${LIBDIR}',
                     'vnum':            PUGL_VERSION})

        if bld.env.BUILD_SHARED:
            bld(features = 'c cshlib',
                name     = name,
                target   = 'pugl_' + name,
                cflags   = ['-DPUGL_INTERNAL', '-DPUGL_SHARED'],
                **args)

        if bld.env.BUILD_STATIC:
            bld(features = 'c cstlib',
                name     = 'pugl_%s_static' % name,
                target   = 'pugl_' + name,
                cflags   = ['-DPUGL_INTERNAL'],
                **args)

    def build_platform(platform, **kwargs):
        build_pugl_lib(platform, **kwargs)
        _build_pc_file(bld, 'Pugl', 'Pugl GUI library core',
                       'pugl', 'pugl_%s' % platform,
                       deps=kwargs)

    def build_backend(platform, backend, **kwargs):
        name  = '%s_%s' % (platform, backend)
        label = 'OpenGL' if backend == 'gl' else backend.title()
        build_pugl_lib(name, **kwargs)
        _build_pc_file(bld, 'Pugl %s' % label,
                       'Pugl GUI library with %s backend' % label,
                       'pugl-%s' % backend, 'pugl_' + name,
                       deps=kwargs,
                       requires=['pugl-%s' % PUGL_MAJOR_VERSION])

    lib_source = ['pugl/detail/implementation.c']
    if bld.env.TARGET_PLATFORM == 'win32':
        platform = 'win'
        build_platform('win',
                       lib=['gdi32', 'user32'],
                       source=lib_source + ['pugl/detail/win.c'])

        if bld.is_defined('HAVE_GL'):
            build_backend('win', 'gl',
                          lib=['gdi32', 'user32', 'opengl32'],
                          source=['pugl/detail/win_gl.c'])

        if bld.is_defined('HAVE_CAIRO'):
            build_backend('win', 'cairo',
                          uselib=['CAIRO'],
                          lib=['gdi32', 'user32'],
                          source=['pugl/detail/win_cairo.c'])

    elif bld.env.TARGET_PLATFORM == 'darwin':
        platform = 'mac'
        build_platform('mac',
                       framework=['Cocoa'],
                       source=lib_source + ['pugl/detail/mac.m'])

        if bld.is_defined('HAVE_GL'):
            build_backend('mac', 'gl',
                          framework=['Cocoa', 'OpenGL'],
                          source=['pugl/detail/mac_gl.m'])

        if bld.is_defined('HAVE_CAIRO'):
            build_backend('mac', 'cairo',
                          framework=['Cocoa'],
                          uselib=['CAIRO'],
                          source=['pugl/detail/mac_cairo.m'])
    else:
        platform = 'x11'
        build_platform('x11',
                       lib=['X11'],
                       source=lib_source + ['pugl/detail/x11.c'])

        if bld.is_defined('HAVE_GL'):
            build_backend('x11', 'gl',
                          lib=['X11', 'GL'],
                          source=['pugl/detail/x11_gl.c'])

        if bld.is_defined('HAVE_CAIRO'):
            build_backend('x11', 'cairo',
                          lib=['X11'],
                          uselib=['CAIRO'],
                          source=['pugl/detail/x11_cairo.c'])

    def build_test(prog, source, platform, backend, **kwargs):
        use = ['pugl_%s_static' % platform,
               'pugl_%s_%s_static' % (platform, backend)]

        target = prog
        if bld.env.TARGET_PLATFORM == 'darwin':
            target = '{0}.app/Contents/MacOS/{0}'.format(prog)

            bld(features     = 'subst',
                source       = 'resources/Info.plist.in',
                target       = '{}.app/Contents/Info.plist'.format(prog),
                install_path = '',
                NAME         = prog)

        backend_lib = platform + '_' + backend
        for k in ('lib', 'framework', 'uselib'):
            kwargs.update({k: (kwargs.get(k, []) +
                               deps.get(platform, {}).get(k, []) +
                               deps.get(backend_lib, {}).get(k, []))})

        bld(features     = 'c cprogram',
            source       = source,
            target       = target,
            use          = use,
            install_path = '',
            **kwargs)

    if bld.env.BUILD_TESTS:
        if bld.is_defined('HAVE_GL'):
            build_test('pugl_test', ['test/pugl_test.c'],
                       platform, 'gl', uselib=['M'])
            build_test('pugl_gl3_test',
                       ['test/pugl_gl3_test.c', 'test/glad/glad.c'],
                       platform, 'gl', uselib=['M', 'DL'])

        if bld.is_defined('HAVE_CAIRO'):
            build_test('pugl_cairo_test', ['test/pugl_cairo_test.c'],
                       platform, 'cairo',
                       uselib=['M', 'CAIRO'])

    if bld.env.DOCS:
        bld(features     = 'subst',
            source       = 'doc/reference.doxygen.in',
            target       = 'doc/reference.doxygen',
            install_path = '',
            PUGL_VERSION = PUGL_VERSION,
            PUGL_SRCDIR  = os.path.abspath(bld.path.srcpath()))

        bld(features = 'doxygen',
            doxyfile = 'doc/reference.doxygen')


def test(tst):
    pass


def lint(ctx):
    "checks code for style issues"
    import subprocess

    subprocess.call("flake8 wscript --ignore E221,W504,E251,E241",
                    shell=True)

    cmd = ("clang-tidy -p=. -header-filter=.* -checks=\"*," +
           "-bugprone-suspicious-string-compare," +
           "-clang-analyzer-alpha.*," +
           "-cppcoreguidelines-avoid-magic-numbers," +
           "-google-readability-todo," +
           "-hicpp-multiway-paths-covered," +
           "-hicpp-signed-bitwise," +
           "-hicpp-uppercase-literal-suffix," +
           "-llvm-header-guard," +
           "-misc-misplaced-const," +
           "-misc-unused-parameters," +
           "-readability-else-after-return," +
           "-readability-magic-numbers," +
           "-readability-uppercase-literal-suffix\" " +
           "../pugl/detail/*.c")

    subprocess.call(cmd, cwd='build', shell=True)

    try:
        subprocess.call(['iwyu_tool.py', '-o', 'clang', '-p', 'build'])
    except Exception:
        Logs.warn('Failed to call iwyu_tool.py')

# Alias .m files to be compiled like .c files, gcc will do the right thing.
@TaskGen.extension('.m')
def m_hook(self, node):
    return self.create_compiled_task('c', node)
