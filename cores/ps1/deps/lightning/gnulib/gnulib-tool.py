#!/usr/bin/python
#
# Copyright (C) 2002-2021 Free Software Foundation, Inc.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.
#

# This program is meant for authors or maintainers which want to import
# modules from gnulib into their packages.


#===============================================================================
# Define global imports
#===============================================================================
import os
import re
import sys
import codecs
import random
import shutil
import argparse
import subprocess as sp
from tempfile import mktemp
from pprint import pprint
from pygnulib import constants
from pygnulib import classes


#===============================================================================
# Define global constants
#===============================================================================
PYTHON3 = constants.PYTHON3
APP = constants.APP
DIRS = constants.DIRS
ENCS = constants.ENCS
UTILS = constants.UTILS
MODES = constants.MODES
TESTS = constants.TESTS
compiler = constants.compiler
joinpath = constants.joinpath
cleaner = constants.cleaner
string = constants.string
isabs = os.path.isabs
isdir = os.path.isdir
isfile = os.path.isfile
normpath = os.path.normpath
relpath = os.path.relpath


#===============================================================================
# Define main part
#===============================================================================
def main():
    # Reset arguments
    mode = None
    destdir = None
    localdir = None
    modcache = None
    verbose = None
    auxdir = None
    modules = None
    avoids = None
    sourcebase = None
    m4base = None
    pobase = None
    docbase = None
    testsbase = None
    tests = None
    libname = None
    lgpl = None
    makefile = None
    libtool = None
    conddeps = None
    macro_prefix = None
    podomain = None
    witness_c_macro = None
    vc_files = None
    dryrun = None
    errors = None

    info = classes.GLInfo()
    parser = argparse.ArgumentParser(
        prog=constants.APP['name'],
        usage='gnulib-tool.py --help',
        add_help=False)
    # help
    parser.add_argument('-h', '--help', '--hel', '--he', '--h',
                        dest='help',
                        default=None,
                        action='store_true')
    # version
    parser.add_argument('--version', '--versio', '--versi', '--vers',
                        dest='version',
                        default=None,
                        action='store_true')
    # list
    parser.add_argument('-l', '--list', '--lis',
                        dest='mode_list',
                        default=None,
                        action='store_true')
    # find
    parser.add_argument('-f', '--find', '--fin', '--fi', '--f',
                        dest='mode_find',
                        default=None,
                        nargs='*')
    # import
    parser.add_argument('-i', '--import',
                        dest='mode_import',
                        default=None,
                        nargs='*')
    # add-import
    parser.add_argument('-a', '--add-import',
                        dest='mode_add_import',
                        default=None,
                        nargs='+')
    # remove-import
    parser.add_argument('-r', '--remove-import',
                        dest='mode_remove_import',
                        default=None,
                        nargs='+')
    # update
    parser.add_argument('-u', '--update',
                        dest='mode_update',
                        default=None,
                        action='store_true')
    # create-testdir
    parser.add_argument('-td', '--create-testdir',
                        dest='mode_create_testdir',
                        default=None,
                        nargs='*')
    # create-megatestdir
    parser.add_argument('-mtd', '--create-megatestdir',
                        dest='mode_create_megatestdir',
                        default=None,
                        nargs='*')
    # test
    parser.add_argument('-t', '--test',
                        dest='mode_test',
                        default=None,
                        nargs='*')
    # megatest
    parser.add_argument('-mt', '--megatest', '--megates', '--megate', '--megat',
                        '--mega', '--meg', '--me', '--m',
                        dest='mode_megatest',
                        default=None,
                        nargs='*')
    # copy-file
    parser.add_argument('-c', '--copy-file',
                        dest='mode_copy_file',
                        default=None,
                        nargs='+')
    # extract-*
    parser.add_argument('-xD', '--extract-description',
                        dest='mode_xdescription',
                        default=None,
                        nargs='*')
    parser.add_argument('-xc', '--extract-comment',
                        dest='mode_xcomment',
                        default=None,
                        nargs='*')
    parser.add_argument('-xs', '--extract-status',
                        dest='mode_xstatus',
                        default=None,
                        nargs='*')
    parser.add_argument('-xn', '--extract-notice',
                        dest='mode_xnotice',
                        default=None,
                        nargs='*')
    parser.add_argument('-xa', '--extract-applicability',
                        dest='mode_xapplicability',
                        default=None,
                        nargs='*')
    parser.add_argument('-xf', '--extract-filelist',
                        dest='mode_xfilelist',
                        default=None,
                        nargs='*')
    parser.add_argument('-xd', '--extract-dependencies',
                        dest='mode_xdependencies',
                        default=None,
                        nargs='*')
    parser.add_argument('-xac', '--extract-autoconf-snippet',
                        dest='mode_xautoconf',
                        default=None,
                        nargs='*')
    parser.add_argument('-xam', '--extract-automake-snippet',
                        dest='mode_xautomake',
                        default=None,
                        nargs='*')
    parser.add_argument('-xi', '--extract-include-directive',
                        dest='mode_xinclude',
                        default=None,
                        nargs='*')
    parser.add_argument('-xl', '--extract-link-directive',
                        dest='mode_xlink',
                        default=None,
                        nargs='*')
    parser.add_argument('-xL', '--extract-license',
                        dest='mode_xlicense',
                        default=None,
                        nargs='*')
    parser.add_argument('-xm', '--extract-maintainer',
                        dest='mode_xmaintainer',
                        default=None,
                        nargs='*')
    # no-changelog: a no-op for backward compatibility
    parser.add_argument('--no-changelog',
                        dest='changelog',
                        default=None,
                        action='store_false')
    # destdir
    parser.add_argument('-d', '--dir',
                        dest='destdir',
                        default=None,
                        nargs=1)
    # localdir
    parser.add_argument('-ld', '--local-dir',
                        dest='localdir',
                        default=None,
                        nargs=1)
    # verbose
    parser.add_argument('-v', '--verbose',
                        default=0,
                        action='count')
    # quiet
    parser.add_argument('-q', '--quiet',
                        default=0,
                        action='count')
    # dryrun
    parser.add_argument('--dry-run',
                        dest='dryrun',
                        default=None,
                        action='store_true')
    # inctests
    parser.add_argument('--with-tests',
                        dest='inctests',
                        default=None,
                        action='store_true')
    # makefile
    parser.add_argument("--makefile-name",
                        dest="makefile",
                        default=None,
                        type=str)
    # obsolete
    parser.add_argument('--with-obsolete',
                        dest='obsolete',
                        default=None,
                        action='store_true')
    # c++-tests
    parser.add_argument('--with-c++-tests',
                        dest='cxx',
                        default=None,
                        action='store_true')
    # longrunning-tests
    parser.add_argument('--with-longrunning-tests',
                        dest='longrunning',
                        default=None,
                        action='store_true')
    # privileged-tests
    parser.add_argument('--with-privileged-tests',
                        dest='privileged',
                        default=None,
                        action='store_true')
    # unportable-tests
    parser.add_argument('--with-unportable-tests',
                        dest='unportable',
                        default=None,
                        action='store_true')
    # all-tests
    parser.add_argument('--with-all-tests',
                        dest='alltests',
                        default=None,
                        action='store_true')
    # auxdir
    parser.add_argument('--aux-dir',
                        dest='auxdir',
                        default=None,
                        nargs=1)
    # libname
    parser.add_argument('--lib',
                        dest='libname',
                        default=None,
                        nargs=1)
    # libtool
    parser.add_argument("--libtool",
                        dest=libtool,
                        default=False,
                        action="store_true")
    # sourcebase
    parser.add_argument('-sb', '--source-base',
                        dest='sourcebase',
                        default=None,
                        nargs=1)
    # m4base
    parser.add_argument('-mb', '--m4-base',
                        dest='m4base',
                        default=None,
                        nargs=1)
    # pobase
    parser.add_argument('-pb', '--po-base',
                        dest='pobase',
                        default=None,
                        nargs=1)
    # docbase
    parser.add_argument('-db', '--doc-base',
                        dest='docbase',
                        default=None,
                        nargs=1)
    # testsbase
    parser.add_argument('-tb', '--tests-base',
                        dest='testsbase',
                        default=None,
                        nargs=1)
    # lgpl
    parser.add_argument('--lgpl',
                        dest='lgpl',
                        default=False,
                        type=int,
                        nargs='?')
    # avoids
    parser.add_argument('--avoid',
                        dest='avoids',
                        default=None,
                        nargs='+')

    # Parse the given arguments.
    cmdargs = parser.parse_args()

    # Determine when user tries to combine modes.
    args = [
        cmdargs.mode_list,
        cmdargs.mode_import,
        cmdargs.mode_add_import,
        cmdargs.mode_remove_import,
        cmdargs.mode_update,
        cmdargs.mode_create_testdir,
        cmdargs.mode_create_megatestdir,
        cmdargs.mode_test,
        cmdargs.mode_megatest,
        cmdargs.mode_copy_file,
        cmdargs.mode_xdescription,
        cmdargs.mode_xcomment,
        cmdargs.mode_xstatus,
        cmdargs.mode_xnotice,
        cmdargs.mode_xapplicability,
        cmdargs.mode_xfilelist,
        cmdargs.mode_xdependencies,
        cmdargs.mode_xautoconf,
        cmdargs.mode_xautomake,
        cmdargs.mode_xinclude,
        cmdargs.mode_xlink,
        cmdargs.mode_xlicense,
        cmdargs.mode_xmaintainer,
    ]
    overflow = [arg for arg in args if arg]
    if len(overflow) > 1:
        message = 'gnulib-tool: Unable to combine different modes of work.\n'
        message += 'Try \'gnulib-tool --help\' for more information.\n'
        sys.stderr.write(message)
        sys.exit(1)

    # Determine selected mode.
    if cmdargs.help != None:
        print(info.usage())
        sys.exit(0)
    if cmdargs.version != None:
        message = '''gnulib-tool (%s %s)%s\n%s\n%s\n\nWritten by %s.''' % \
            (info.package(), info.date(), info.version(), info.copyright(),
             info.license(), info.authors())
        print(message)
        sys.exit(0)
    if cmdargs.mode_list != None:
        mode = 'list'
    if cmdargs.mode_import != None:
        mode = 'import'
        modules = list(cmdargs.mode_import)
    if cmdargs.mode_add_import != None:
        mode = 'add-import'
        modules = list(cmdargs.mode_add_import)
    if cmdargs.mode_remove_import != None:
        mode = 'remove-import'
        modules = list(cmdargs.mode_remove_import)
    if cmdargs.mode_update != None:
        mode = 'update'
    if cmdargs.mode_create_testdir != None:
        mode = 'create-testdir'
        modules = list(cmdargs.mode_create_testdir)
    if cmdargs.mode_create_megatestdir != None:
        mode = 'create-megatestdir'
        modules = list(cmdargs.mode_create_megatestdir)
    if cmdargs.mode_test != None:
        mode = 'test'
        modules = list(cmdargs.mode_test)
    if cmdargs.mode_megatest != None:
        mode = 'megatest'
        modules = list(cmdargs.mode_megatest)
    if cmdargs.mode_xdescription != None:
        mode = 'extract-description'
        modules = list(cmdargs.mode_xdescription)
    if cmdargs.mode_xcomment != None:
        mode = 'extract-comment'
        modules = list(cmdargs.mode_xcomment)
    if cmdargs.mode_xstatus != None:
        mode = 'extract-status'
        modules = list(cmdargs.mode_xstatus)
    if cmdargs.mode_xnotice != None:
        mode = 'extract-notice'
        modules = list(cmdargs.mode_xnotice)
    if cmdargs.mode_xapplicability != None:
        mode = 'extract-applicability'
        modules = list(cmdargs.mode_xapplicability)
    if cmdargs.mode_xfilelist != None:
        mode = 'extract-filelist'
        modules = list(cmdargs.mode_xfilelist)
    if cmdargs.mode_xautoconf != None:
        mode = 'extract-autoconf-snippet'
        modules = list(cmdargs.mode_xautoconf)
    if cmdargs.mode_xautomake != None:
        mode = 'extract-automake-snippet'
        modules = list(cmdargs.mode_xautomake)
    if cmdargs.mode_xdependencies != None:
        mode = 'extract-dependencies'
        modules = list(cmdargs.mode_xdependencies)
    if cmdargs.mode_xinclude != None:
        mode = 'extract-include-directive'
        modules = list(cmdargs.mode_xinclude)
    if cmdargs.mode_xlink != None:
        mode = 'extract-link-directive'
        modules = list(cmdargs.mode_xlink)
    if cmdargs.mode_xlicense != None:
        mode = 'extract-license'
        modules = list(cmdargs.mode_xlicense)
    if cmdargs.mode_xmaintainer != None:
        mode = 'extract-maintainer'
        modules = list(cmdargs.mode_xmaintainer)
    if cmdargs.mode_copy_file != None:
        mode = 'copy-file'
        if len(cmdargs.mode_copy_file) > 2:
            message = '%s: *** ' % constants.APP['name']
            message += 'invalid number of arguments for --%s' % mode
            message += '\n%s: *** Exit.\n' % constants.APP['name']
            sys.stderr.write(message)
            sys.exit(1)
        files = list(cmdargs.mode_copy_file)

    # Determine specific settings.
    destdir = cmdargs.destdir
    if destdir != None:
        destdir = cmdargs.destdir[0]
    localdir = cmdargs.localdir
    if localdir != None:
        localdir = cmdargs.localdir[0]
    libname = cmdargs.libname
    if libname != None:
        libname = cmdargs.libname[0]
    auxdir = cmdargs.auxdir
    if auxdir != None:
        auxdir = cmdargs.auxdir[0]
    sourcebase = cmdargs.sourcebase
    if sourcebase != None:
        sourcebase = cmdargs.sourcebase[0]
    m4base = cmdargs.m4base
    if m4base != None:
        m4base = cmdargs.m4base[0]
    pobase = cmdargs.pobase
    if pobase != None:
        pobase = cmdargs.pobase[0]
    testsbase = cmdargs.testsbase
    if testsbase != None:
        testsbase = cmdargs.testsbase[0]
    dryrun = cmdargs.dryrun
    verbose = -cmdargs.quiet + cmdargs.verbose
    inctests = cmdargs.inctests
    flags = [cmdargs.inctests, cmdargs.obsolete, cmdargs.cxx,
             cmdargs.longrunning, cmdargs.privileged, cmdargs.unportable,
             cmdargs.alltests]
    testflags = list()
    for flag in flags:
        index = flags.index(flag)
        if flag != None:
            if flag:
                if index == 0:
                    testflags += [constants.TESTS['tests']]
                elif index == 1:
                    testflags += [constants.TESTS['obsolete']]
                elif index == 2:
                    testflags += [constants.TESTS['cxx-tests']]
                elif index == 3:
                    testflags += [constants.TESTS['longrunning-tests']]
                elif index == 4:
                    testflags += [constants.TESTS['privileged-tests']]
                elif index == 5:
                    testflags += [constants.TESTS['unportable-tests']]
                elif index == 6:
                    testflags += [constants.TESTS['all-tests']]
    lgpl = cmdargs.lgpl
    libtool = cmdargs.libtool
    makefile = cmdargs.makefile
    if lgpl == None:
        lgpl = True
    avoids = cmdargs.avoids

    # Create pygnulib configuration.
    config = classes.GLConfig(
        destdir=destdir,
        localdir=localdir,
        m4base=m4base,
        auxdir=auxdir,
        modules=modules,
        avoids=avoids,
        sourcebase=sourcebase,
        pobase=pobase,
        docbase=docbase,
        testsbase=testsbase,
        testflags=testflags,
        libname=libname,
        lgpl=lgpl,
        makefile=makefile,
        libtool=libtool,
        conddeps=conddeps,
        macro_prefix=macro_prefix,
        podomain=podomain,
        witness_c_macro=witness_c_macro,
        vc_files=vc_files,
        modcache=modcache,
        verbose=verbose,
        dryrun=dryrun,
    )

    # Canonicalize the inctests variable.
    if inctests == None:
        if mode in ['import', 'add-import', 'remove-import', 'update']:
            config.disableTestFlag(TESTS['tests'])
        elif mode in ['create-testdir', 'create-megatestdir', 'test', 'megatest']:
            config.enableTestFlag(TESTS['tests'])

    # Work in the given mode.
    if mode in ['list']:
        modulesystem = classes.GLModuleSystem(config)
        listing = modulesystem.list()
        result = '\n'.join(listing)
        os.rmdir(config['tempdir'])
        print(result)

    elif mode in ['import', 'add-import', 'remove-import', 'update']:
        mode = MODES[mode]
        if not destdir:
            destdir = '.'
        config.setDestDir(destdir)

        if mode == MODES['import']:
            # Set variables to default values.
            if not sourcebase:
                sourcebase = 'lib'
            if not m4base:
                m4base = 'm4'
            if not docbase:
                docbase = 'doc'
            if not testsbase:
                testsbase = 'tests'
            if not macro_prefix:
                macro_prefix = 'gl'
            config.setSourceBase(sourcebase)
            config.setM4Base(m4base)
            config.setDocBase(docbase)
            config.setTestsBase(testsbase)
            config.setMacroPrefix(macro_prefix)

            # Perform GLImport actions.
            importer = classes.GLImport(config, mode)
            filetable, transformers = importer.prepare()
            importer.execute(filetable, transformers)

        else:  # if mode != MODE['--import']
            if m4base:
                if not isfile(joinpath(destdir, m4base, 'gnulib-cache.m4')):
                    if not sourcebase:
                        sourcebase = 'lib'
                    if not docbase:
                        docbase = 'doc'
                    if not testsbase:
                        testsbase = 'tests'
                    if not macro_prefix:
                        macro_prefix = 'gl'
                    config.setSourceBase(sourcebase)
                    config.setM4Base(m4base)
                    config.setDocBase(docbase)
                    config.setTestsBase(testsbase)
                    config.setMacroPrefix(macro_prefix)
                # Perform GLImport actions.
                importer = classes.GLImport(config, mode)
                filetable, transformers = importer.prepare()
                importer.execute(filetable, transformers)
            else:  # if not m4base
                m4dirs = list()
                dirisnext = bool()
                filepath = joinpath(destdir, 'Makefile.am')
                if isfile(filepath):
                    with codecs.open(filepath, 'rb', 'UTF-8') as file:
                        data = file.read()
                        data = data.split('ACLOCAL_AMFLAGS')[1]
                        data = data[data.find('=') + 1:data.find('\n')]
                    aclocal_amflags = data.split()
                    for aclocal_amflag in aclocal_amflags:
                        if dirisnext:
                            if not isabs(aclocal_amflag):
                                m4dirs += [aclocal_amflag]
                        else:  # if not dirisnext
                            if aclocal_amflag == '-I':
                                dirisnext = True
                            else:  # if aclocal_amflag != '-I'
                                dirisnext = False
                else:  # if not isfile(filepath)
                    filepath = joinpath(destdir, 'aclocal.m4')
                    if isfile(filepath):
                        pattern = constants.compiler(
                            r'm4_include\(\[(.*?)\]\)')
                        with codecs.open(filepath, 'rb', 'UTF-8') as file:
                            m4dirs = pattern.findall(file.read())
                        m4dirs = [os.path.dirname(m4dir) for m4dir in m4dirs]
                        m4dirs = \
                            [  # Begin filtering
                                m4dir for m4dir in m4dirs \
                                if isfile(joinpath(destdir, m4dir, 'gnulib-cache.m4'))
                            ]  # Finish filtering
                        m4dirs = sorted(set(m4dirs))
                if len(m4dirs) == 0:
                    # First use of gnulib in a package.
                    # Any number of additional modules can be given.
                    if not sourcebase:
                        sourcebase = 'lib'
                    m4base = 'm4'
                    if not docbase:
                        docbase = 'doc'
                    if not testsbase:
                        testsbase = 'tests'
                    if not macro_prefix:
                        macro_prefix = 'gl'
                    config.setSourceBase(sourcebase)
                    config.setM4Base(m4base)
                    config.setDocBase(docbase)
                    config.setTestsBase(testsbase)
                    config.setMacroPrefix(macro_prefix)
                    # Perform GLImport actions.
                    importer = classes.GLImport(config, mode)
                    filetable, transformers = importer.prepare()
                    importer.execute(filetable, transformers)
                elif len(m4dirs) == 1:
                    m4base = m4dirs[-1]
                    config.setM4Base(m4base)
                    # Perform GLImport actions.
                    importer = classes.GLImport(config, mode)
                    filetable, transformers = importer.prepare()
                    importer.execute(filetable, transformers)
                else:  # if len(m4dirs) > 1
                    for m4base in m4dirs:
                        config.setM4Base(m4base)
                        # Perform GLImport actions.
                        importer = classes.GLImport(config, mode)
                        filetable, transformers = importer.prepare()
                        importer.execute(filetable, transformers)

    elif mode == 'create-testdir':
        if not destdir:
            message = '%s: *** ' % constants.APP['name']
            message += 'please specify --dir option'
            message += '\n%s: *** Exit.\n' % constants.APP['name']
            sys.stderr.write(message)
            sys.exit(1)
        if not auxdir:
            auxdir = 'build-aux'
        config.setAuxDir(auxdir)
        testdir = classes.GLTestDir(config, destdir)
        testdir.execute()

    elif mode == 'create-megatestdir':
        if not destdir:
            message = '%s: *** ' % constants.APP['name']
            message += 'please specify --dir option'
            message += '\n%s: *** Exit.\n' % constants.APP['name']
            sys.stderr.write(message)
            sys.exit(1)
        if not auxdir:
            auxdir = 'build-aux'
        config.setAuxDir(auxdir)
        testdir = classes.GLMegaTestDir(config, destdir)
        testdir.execute()

    elif mode == 'test':
        if not destdir:
            destdir = 'testdir %04d' % random.randrange(0, 9999)
        if not auxdir:
            auxdir = 'build-aux'
        config.setAuxDir(auxdir)
        testdir = classes.GLTestDir(config, destdir)
        testdir.execute()
        os.chdir(destdir)
        os.mkdir('build')
        os.chdir('build')
        try:  # Try to execute commands
            sp.call(['../configure'])
            sp.call([UTILS['make']])
            sp.call([UTILS['make'], 'check'])
            sp.call([UTILS['make'], 'distclean'])
        except Exception as error:
            sys.exit(1)
        args = ['find', '.', '-type', 'f', '-print']
        remaining = sp.check_output(args).decode(ENCS['shell'])
        lines = [line.strip()
                 for line in remaining.split('\n') if line.strip()]
        remaining = ' '.join(lines)
        if remaining:
            message = 'Remaining files: %s\n' % remaining
            message += 'gnulib-tool: *** Stop.\n'
            sys.stderr.write(message)
            sys.exit(1)
        os.chdir('../..')
        sp.call(['rm', '-rf', destdir], shell=False)

    elif mode == 'megatest':
        if not destdir:
            destdir = 'testdir %04d' % random.randrange(0, 9999)
        if not auxdir:
            auxdir = 'build-aux'
        config.setAuxDir(auxdir)
        testdir = classes.GLMegaTestDir(config, destdir)
        testdir.execute()
        os.chdir(destdir)
        os.mkdir('build')
        os.chdir('build')
        sp.call(['../configure'])
        sp.call([UTILS['make']])
        sp.call([UTILS['make'], 'check'])
        sp.call([UTILS['make'], 'distclean'])
        args = ['find', '.', '-type', 'f', '-print']
        remaining = sp.check_output(args).decode(ENCS['shell'])
        lines = [line.strip()
                 for line in remaining.split('\n') if line.strip()]
        remaining = ' '.join(lines)
        if remaining:
            message = 'Remaining files: %s\n' % remaining
            message += 'gnulib-tool: *** Stop.\n'
            sys.stderr.write(message)
            sys.exit(1)
        os.chdir('../..')
        sp.call(['rm', '-rf', destdir], shell=False)

    elif mode == 'extract-description':
        modulesystem = classes.GLModuleSystem(config)
        modules = [modulesystem.find(module) for module in modules]
        for module in modules:
            print(module.getDescription())

    elif mode == 'extract-comment':
        modulesystem = classes.GLModuleSystem(config)
        modules = [modulesystem.find(module) for module in modules]
        for module in modules:
            print(module.getComment())

    elif mode == 'extract-status':
        modulesystem = classes.GLModuleSystem(config)
        modules = [modulesystem.find(module) for module in modules]
        for module in modules:
            status = module.getStatus()
            print('\n'.join(status))

    elif mode == 'extract-notice':
        modulesystem = classes.GLModuleSystem(config)
        modules = [modulesystem.find(module) for module in modules]
        for module in modules:
            print(module.getNotice())

    elif mode == 'extract-applicability':
        modulesystem = classes.GLModuleSystem(config)
        modules = [modulesystem.find(module) for module in modules]
        for module in modules:
            print(module.getApplicability())

    elif mode == 'extract-filelist':
        modulesystem = classes.GLModuleSystem(config)
        modules = [modulesystem.find(module) for module in modules]
        for module in modules:
            files = module.getFiles()
            print('\n'.join(files))

    elif mode == 'extract-dependencies':
        result = string()
        if avoids:
            message = '%s: *** ' % constants.APP['name']
            message += 'cannot combine --avoid and --extract-dependencies\n'
            message += '%s: *** Exit.\n' % constants.APP['name']
            sys.stderr.write(message)
            sys.exit(1)
        modulesystem = classes.GLModuleSystem(config)
        modules = [modulesystem.find(module) for module in modules]
        for module in modules:
            dependencies = module.getDependencies()
            if dependencies:
                for depmodule, condition in dependencies:
                    if condition == None:
                        result += '%s\n' % str(depmodule)
                    else:  # if condition != None
                        result += '%s\t%s' % (str(depmodule), condition)
        print(result)

    elif mode == 'extract-autoconf-snippet':
        modulesystem = classes.GLModuleSystem(config)
        modules = [modulesystem.find(module) for module in modules]
        for module in modules:
            print(module.getAutoconfSnippet())

    elif mode == 'extract-automake-snippet':
        modulesystem = classes.GLModuleSystem(config)
        modules = [modulesystem.find(module) for module in modules]
        for module in modules:
            print(module.getAutomakeSnippet())

    elif mode == 'extract-include-directive':
        modulesystem = classes.GLModuleSystem(config)
        modules = [modulesystem.find(module) for module in modules]
        for module in modules:
            print(module.getInclude())

    elif mode == 'extract-link-directive':
        modulesystem = classes.GLModuleSystem(config)
        modules = [modulesystem.find(module) for module in modules]
        for module in modules:
            print(module.getLink())

    elif mode == 'extract-license':
        modulesystem = classes.GLModuleSystem(config)
        modules = [modulesystem.find(module) for module in modules]
        for module in modules:
            print(module.getLicense())

    elif mode == 'extract-maintainer':
        modulesystem = classes.GLModuleSystem(config)
        modules = [modulesystem.find(module) for module in modules]
        for module in modules:
            print(module.getMaintainer())

    elif mode == 'extract-tests-module':
        modulesystem = classes.GLModuleSystem(config)
        modules = [modulesystem.find(module) for module in modules]
        for module in modules:
            if module.getTestsModule():
                print(module.getTestsName())

    elif mode == 'copy-file':
        srcpath = files[0]
        if len(files) == 2:
            dest = files[1]
        else:  # if len(files) != 2
            dest = '.'
        if type(srcpath) is bytes:
            srcpath = srcpath.decode(ENCS['default'])
        if type(dest) is bytes:
            dest = dest.decode(ENCS['default'])
        if not auxdir:
            auxdir = 'build-aux'
        if not sourcebase:
            sourcebase = 'lib'
        if not m4base:
            m4base = 'm4'
        if not docbase:
            docbase = 'doc'
        if not testsbase:
            testsbase = 'tests'
        config.setAuxDir(auxdir)
        config.setSourceBase(sourcebase)
        config.setM4Base(m4base)
        config.setDocBase(docbase)
        config.setTestsBase(testsbase)
        filesystem = classes.GLFileSystem(config)
        lookedup, flag = filesystem.lookup(srcpath)
        if isdir(dest):
            destdir = string(dest)
            if srcpath.startswith('build-aux/'):
                destpath = constants.substart(
                    'build-aux/', '%s/' % auxdir, srcpath)
            elif srcpath.startswith('doc/'):
                destpath = constants.substart('doc/', '%s/' % docbase, srcpath)
            elif srcpath.startswith('lib/'):
                destpath = constants.substart(
                    'lib/', '%s/' % sourcebase, srcpath)
            elif srcpath.startswith('m4/'):
                destpath = constants.substart('m4/', '%s/' % m4base, srcpath)
            elif srcpath.startswith('tests/'):
                destpath = constants.substart(
                    'tests/', '%s/' % testsbase, srcpath)
            elif srcpath.startswith('tests=lib/'):
                destpath = constants.substart(
                    'tests=lib/', '%s/' % testsbase, srcpath)
            elif srcpath.startswith('top/'):
                destpath = constants.substart('top/', '', srcpath)
            else:  # either case
                destpath = srcpath
        else:  # if not isdir(destpath)
            destdir = os.path.dirname(destpath)
            destpath = os.path.basename(destpath)
        # Create the directory for destfile.
        dirname = os.path.dirname(joinpath(destdir, destpath))
        if not config['dryrun']:
            if dirname and not isdir(dirname):
                try:  # Try to create directories
                    os.makedirs(dirname)
                except Exception as error:
                    pass
        # Copy the file.
        assistant = classes.GLFileAssistant(config)
        tmpfile = assistant.tmpfilename(destpath)
        shutil.copy(lookedup, tmpfile)
        already_present = True
        assistant.setOriginal(srcpath)
        assistant.setRewritten(destpath)
        if isfile(joinpath(destdir, destpath)):
            # The file already exists.
            assistant.update(lookedup, flag, tmpfile, already_present)
        else:  # if not isfile(joinpath(destdir, destpath))
            # Install the file.
            # Don't protest if the file should be there but isn't: it happens
            # frequently that developers don't put autogenerated files under version
            # control.
            assistant.add(lookedup, flag, tmpfile)
        if isfile(tmpfile):
            os.remove(tmpfile)

    else:
        message = '%s: *** ' % constants.APP['name']
        message += 'no mode specified'
        message += '\n%s: *** Exit.\n' % constants.APP['name']
        sys.stderr.write(message)
        sys.exit(1)


if __name__ == '__main__':
    try:  # Try to execute
        main()
    except classes.GLError as error:
        errmode = 0  # gnulib-style errors
        errno = error.errno
        errinfo = error.errinfo
        if errmode == 0:
            message = '%s: *** ' % constants.APP['name']
            if errinfo == None:
                errinfo = string()
            if errno == 1:
                message += 'file %s not found' % errinfo
            elif errno == 2:
                message += 'patch file %s didn\'t apply cleanly' % errinfo
            elif errno == 3:
                message += 'cannot find %s - make sure ' % errinfo
                message += 'you run gnulib-tool from within your package\'s directory'
            elif errno == 4:
                message += 'minimum supported autoconf version is 2.59. Try adding'
                message += 'AC_PREREQ([%s])' % constants.DEFAULT_AUTOCONF_MINVERSION
                message += ' to your configure.ac.'
            elif errno == 5:
                "%s is expected to contain gl_M4_BASE([%s])" % \
                    (repr(os.path.join(errinfo, 'gnulib-comp.m4')), repr(errinfo))
            elif errno == 6:
                message += 'missing --source-base option'
            elif errno == 7:
                message += 'missing --doc-base option. --doc-base has been introduced '
                message += 'on 2006-07-11; if your last invocation of \'gnulib-tool '
                message += '--import\' is before that date, you need to run'
                message += '\'gnulib-tool --import\' once, with a --doc-base option.'
            elif errno == 8:
                message += 'missing --tests-base option'
            elif errno == 9:
                message += 'missing --lib option'
            elif errno == 10:
                message = 'gnulib-tool: option --conditional-dependencies is not '
                message += 'supported with --with-tests\n'
            elif errno == 11:
                incompatibilities = string()
                message += 'incompatible license on modules:%s' % constants.NL
                for pair in errinfo:
                    incompatibilities += pair[0]
                    incompatibilities += ' %s' % pair[1]
                    incompatibilities += constants.NL
                tempname = mktemp()
                with codecs.open(tempname, 'wb', 'UTF-8') as file:
                    file.write(incompatibilities)
                sed_table = 's,^\\([^ ]*\\) ,\\1' + ' ' * 51 + ',\n'
                sed_table += 's,^\\(' + '.' * 49 + \
                    '[^ ]*\) *,' + ' ' * 17 + '\\1 ,'
                args = ['sed', '-e', sed_table, tempname]
                incompatibilities = sp.check_output(
                    args).decode(ENCS['default'])
                message += incompatibilities
                os.remove(tempname)
            elif errno == 12:
                message += 'refusing to do nothing'
            elif errno in [13, 14, 15, 16, 17]:
                message += 'failed'
            elif errno == 19:
                message += 'could not create destination directory: %s' % errinfo
            if errno != 10:
                message += '\n%s: *** Exit.\n' % constants.APP['name']
            sys.stderr.write(message)
            sys.exit(1)
