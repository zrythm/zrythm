#!/usr/bin/env python
import filecmp
import glob
import os
import shutil
import subprocess

from waflib.extras import autowaf as autowaf
import waflib.Logs as Logs, waflib.Options as Options

# Version of this package (even if built as a child)
ZIX_VERSION       = '0.0.2'
ZIX_MAJOR_VERSION = '0'

# Library version (UNIX style major, minor, micro)
# major increment <=> incompatible changes
# minor increment <=> compatible changes (additions)
# micro increment <=> no interface changes
# Zix uses the same version number for both library and package
ZIX_LIB_VERSION = ZIX_VERSION

# Variables for 'waf dist'
APPNAME = 'zix'
VERSION = ZIX_VERSION

# Mandatory variables
top = '.'
out = 'build'

def options(opt):
    autowaf.set_options(opt)
    opt.load('compiler_c')
    opt.add_option('--test', action='store_true', dest='build_tests',
                   help='Build unit tests')
    opt.add_option('--bench', action='store_true', dest='build_bench',
                   help='Build benchmarks')
    opt.add_option('--static', action='store_true', dest='static',
                   help='Build static library')
    opt.add_option('--page-size', type='int', default=4096, dest='page_size',
                   help='Page size for B-tree')

def configure(conf):
    conf.load('compiler_c')
    autowaf.configure(conf)
    autowaf.set_c99_mode(conf)
    autowaf.display_header('Zix Configuration')

    conf.env.BUILD_BENCH  = Options.options.build_bench
    conf.env.BUILD_TESTS  = Options.options.build_tests
    conf.env.BUILD_STATIC = Options.options.static

    # Check for mlock
    conf.check(function_name='mlock',
               header_name='sys/mman.h',
               define_name='HAVE_MLOCK',
               mandatory=False)

    # Check for gcov library (for test coverage)
    if conf.env.BUILD_TESTS:
        conf.check_cc(lib='gcov',
                      define_name='HAVE_GCOV',
                      mandatory=False)

    if Options.options.build_bench:
        autowaf.check_pkg(conf, 'glib-2.0', uselib_store='GLIB',
                          atleast_version='2.0.0', mandatory=False)
        if not conf.is_defined('HAVE_GLIB'):
            conf.fatal('Glib is required to build benchmarks')

    autowaf.define(conf, 'ZIX_VERSION', ZIX_VERSION)
    autowaf.define(conf, 'ZIX_BTREE_PAGE_SIZE', Options.options.page_size)
    conf.write_config_header('zix-config.h', remove=False)

    autowaf.display_msg(conf, 'Unit tests', str(conf.env.BUILD_TESTS))
    autowaf.display_msg(conf, 'Benchmarks', str(conf.env.BUILD_BENCH))
    print('')

tests = [
    'hash_test',
    'inline_test',
    'patree_test',
    'trie_test',
    'ring_test',
    'sem_test',
    'sorted_array_test',
    'strindex_test',
    'tree_test',
    'btree_test',
    'bitset_test',
    'ampatree_test'
]

def build(bld):
    # C Headers
    bld.install_files('${INCLUDEDIR}/zix', bld.path.ant_glob('zix/*.h'))

    # Pkgconfig file
    autowaf.build_pc(bld, 'ZIX', ZIX_VERSION, ZIX_MAJOR_VERSION, [],
                     {'ZIX_MAJOR_VERSION' : ZIX_MAJOR_VERSION})

    framework = ''
    if Options.platform == 'darwin':
        framework = ['CoreServices']

    libflags = [ '-fvisibility=hidden' ]
    if bld.env.MSVC_COMPILER:
        libflags = []

    lib_source = '''
        zix/chunk.c
        zix/digest.c
        zix/hash.c
        zix/patree.c
        zix/trie.c
        zix/ring.c
        zix/sorted_array.c
        zix/strindex.c
        zix/tree.c
        zix/btree.c
        zix/bitset.c
        zix/ampatree.c
    '''

    # Library
    obj = bld(features        = 'c cshlib',
              export_includes = ['.'],
              source          = lib_source,
              includes        = ['.'],
              name            = 'libzix',
              target          = 'zix',
              vnum            = ZIX_LIB_VERSION,
              install_path    = '${LIBDIR}',
              framework       = framework,
              cflags          = libflags + ['-DZIX_SHARED',
                                            '-DZIX_INTERNAL'])

    if bld.env.BUILD_STATIC or bld.env.BUILD_BENCH:
        obj = bld(features        = 'c cstlib',
                  export_includes = ['.'],
                  source          = lib_source,
                  includes        = ['.'],
                  name            = 'libzix_static',
                  target          = 'zix',
                  vnum            = ZIX_LIB_VERSION,
                  install_path    = None,
                  framework       = framework,
                  cflags          = libflags + ['-DZIX_SHARED',
                                                '-DZIX_INTERNAL'])

    if bld.env.BUILD_TESTS:
        test_libs   = ['pthread', 'dl']
        test_cflags = []
        if bld.env.MSVC_COMPILER:
            test_libs = []
        if bld.is_defined('HAVE_GCOV'):
            test_libs   += ['gcov']
            test_cflags += ['-fprofile-arcs', '-ftest-coverage']

        # Profiled static library (for unit test code coverage)
        obj = bld(features     = 'c cstlib',
                  source       = lib_source,
                  includes     = ['.'],
                  lib          = test_libs,
                  name         = 'libzix_profiled',
                  target       = 'zix_profiled',
                  install_path = '',
                  framework    = framework,
                  cflags       = test_cflags + ['-DZIX_INTERNAL'])

        # Unit test programs
        for i in tests:
            obj = bld(features     = 'c cprogram',
                      source       = ['test/%s.c' % i, 'test/test_malloc.c'],
                      includes     = ['.'],
                      use          = 'libzix_profiled',
                      lib          = test_libs,
                      target       = 'test/%s' % i,
                      install_path = None,
                      framework    = framework,
                      cflags       = test_cflags)

    if bld.env.BUILD_BENCH:
        # Benchmark programs
        for i in ['tree_bench', 'dict_bench']:
            obj = bld(features     = 'c cprogram',
                      source       = 'test/%s.c' % i,
                      includes     = ['.'],
                      use          = 'libzix_static',
                      uselib       = 'GLIB',
                      lib          = ['rt'],
                      target       = 'test/%s' % i,
                      framework    = framework,
                      install_path = '')

    # Documentation
    autowaf.build_dox(bld, 'ZIX', ZIX_VERSION, top, out)

    bld.add_post_fun(autowaf.run_ldconfig)
    if bld.env.DOCS:
        bld.add_post_fun(fix_docs)

def lint(ctx):
    subprocess.call('cpplint.py --filter=-whitespace/tab,-whitespace/braces,-build/header_guard,-readability/casting,-readability/todo zix/*', shell=True)

def build_dir(ctx, subdir):
    if autowaf.is_child():
        return os.path.join('build', APPNAME, subdir)
    else:
        return os.path.join('build', subdir)

def fix_docs(ctx):
    if ctx.cmd == 'build':
        autowaf.make_simple_dox(APPNAME)

def upload_docs(ctx):
    os.system('rsync -avz --delete -e ssh build/doc/html/* drobilla@drobilla.net:~/drobilla.net/docs/zix')

def test(ctx):
    autowaf.pre_test(ctx, APPNAME)
    os.environ['PATH'] = 'test' + os.pathsep + os.getenv('PATH')
    autowaf.run_tests(ctx, APPNAME, tests, dirs=['.', './test'])
    autowaf.post_test(ctx, APPNAME, dirs=['.', './test'])

def bench(ctx):
    os.chdir('build')

    # Benchmark trees

    subprocess.call(['test/tree_bench', '400000', '6400000'])
    subprocess.call(['../plot.py', 'tree_bench.svg',
                     'tree_insert.txt',
                     'tree_search.txt',
                     'tree_iterate.txt',
                     'tree_delete.txt'])

    # Benchmark dictionaries

    filename = 'gibberish.txt'
    if not os.path.exists(filename):
        Logs.info('Generating random text %s' % filename)
        import random
        out = open(filename, 'w')
        for i in xrange(1 << 20):
            wordlen = random.randrange(1, 64)
            word    = ''
            for j in xrange(wordlen):
                word += chr(random.randrange(ord('A'), min(ord('Z'), ord('A') + j + 1)))
            out.write(word + ' ')
        out.close()

    subprocess.call(['test/dict_bench', 'gibberish.txt'])
    subprocess.call(['../plot.py', 'dict_bench.svg',
                     'dict_insert.txt', 'dict_search.txt'])
