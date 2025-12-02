#!/usr/bin/python
# encoding: UTF-8

#===============================================================================
# Define global imports
#===============================================================================
import os
import re
import sys
import locale
import codecs
import shutil
import filecmp
import subprocess as sp
from . import constants
from .GLError import GLError
from .GLConfig import GLConfig
from .GLModuleSystem import GLModule
from .GLModuleSystem import GLModuleTable
from .GLModuleSystem import GLModuleSystem
from .GLFileSystem import GLFileSystem
from .GLFileSystem import GLFileAssistant
from .GLMakefileTable import GLMakefileTable
from .GLEmiter import GLEmiter


#===============================================================================
# Define module information
#===============================================================================
__author__ = constants.__author__
__license__ = constants.__license__
__copyright__ = constants.__copyright__


#===============================================================================
# Define global constants
#===============================================================================
PYTHON3 = constants.PYTHON3
NoneType = type(None)
APP = constants.APP
DIRS = constants.DIRS
ENCS = constants.ENCS
UTILS = constants.UTILS
MODES = constants.MODES
TESTS = constants.TESTS
compiler = constants.compiler
joinpath = constants.joinpath
cleaner = constants.cleaner
relpath = constants.relativize
string = constants.string
isabs = os.path.isabs
isdir = os.path.isdir
isfile = os.path.isfile
normpath = os.path.normpath


#===============================================================================
# Define GLImport class
#===============================================================================
class GLImport(object):
    '''GLImport class is used to provide methods for --import, --add-import,
    --remove-import and --update actions. This is a high-level class, so
    developers may  have to use lower-level classes to create their own
    scripts. However, if user needs just to use power of gnulib-tool, this class
    is a very good choice.'''

    def __init__(self, config, mode):
        '''Create GLImport instance.
        The first variable, mode, must be one of the values of the MODES dict
        object, which is accessible from constants module. The second one, config,
        must be a GLConfig object.'''
        if type(config) is not GLConfig:
            raise(TypeError('config must have GLConfig type, not %s' %
                            repr(config)))
        if type(mode) is int and \
                MODES['import'] <= mode <= MODES['update']:
            self.mode = mode
        else:  # if mode is not int or is not 0-3
            raise(TypeError('mode must be 0 <= mode <= 3, not %s' %
                            repr(mode)))

        # Initialize some values.
        self.cache = GLConfig()
        self.config = config.copy()
        os.rmdir(self.cache['tempdir'])

        # Get cached auxdir and libtool from configure.ac/in.
        self.cache.setAuxDir('.')
        path = joinpath(self.config['destdir'], 'configure.ac')
        if not isfile(path):
            path = joinpath(self.config['destdir'], 'configure.in')
            if not isfile(path):
                raise(GLError(3, path))
        self.config.setAutoconfFile(path)
        with codecs.open(path, 'rb', 'UTF-8') as file:
            data = file.read()
        pattern = compiler(r'^AC_CONFIG_AUX_DIR\((.*?)\)$', re.S | re.M)
        match = pattern.findall(data)
        if match:
            result = cleaner(match)[0]
            self.cache.setAuxDir(joinpath(result, self.config['destdir']))
        pattern = compiler(r'A[CM]_PROG_LIBTOOL', re.S | re.M)
        guessed_libtool = bool(pattern.findall(data))
        if self.config['auxdir'] == None:
            self.config.setAuxDir(self.cache['auxdir'])

        # Guess autoconf version.
        pattern = compiler('.*AC_PREREQ\((.*?)\)', re.S | re.M)
        versions = cleaner(pattern.findall(data))
        if versions:
            version = sorted(set([float(version) for version in versions]))[-1]
            self.config.setAutoconfVersion(version)
            if version < 2.59:
                raise(GLError(4, version))

        # Get other cached variables.
        path = joinpath(self.config['m4base'], 'gnulib-cache.m4')
        if isfile(joinpath(self.config['m4base'], 'gnulib-cache.m4')):
            with codecs.open(path, 'rb', 'UTF-8') as file:
                data = file.read()

            # Create regex object and keys.
            pattern = compiler('^(gl_.*?)\\((.*?)\\)$', re.S | re.M)
            keys = \
                [
                    'gl_LOCAL_DIR', 'gl_MODULES', 'gl_AVOID', 'gl_SOURCE_BASE',
                    'gl_M4_BASE', 'gl_PO_BASE', 'gl_DOC_BASE', 'gl_TESTS_BASE',
                    'gl_MAKEFILE_NAME', 'gl_MACRO_PREFIX', 'gl_PO_DOMAIN',
                    'gl_WITNESS_C_MACRO', 'gl_VC_FILES', 'gl_LIB',
                ]

            # Find bool values.
            if 'gl_LGPL(' in data:
                keys.append('gl_LGPL')
                self.cache.setLGPL(True)
            if 'gl_LIBTOOL' in data:
                self.cache.enableLibtool()
                data = data.replace('gl_LIBTOOL', '')
            if 'gl_CONDITIONAL_DEPENDENCIES' in data:
                self.cache.enableCondDeps()
                data = data.replace('gl_CONDITIONAL_DEPENDENCIES', '')
            if 'gl_VC_FILES' in data:
                self.cache.enableVCFiles()
                data = data.replace('gl_VC_FILES', '')
            if 'gl_WITH_TESTS' in data:
                self.cache.enableTestFlag(TESTS['tests'])
                data = data.replace('gl_WITH_TESTS', '')
            if 'gl_WITH_OBSOLETE' in data:
                self.cache.enableTestFlag(TESTS['obsolete'])
                data = data.replace('gl_WITH_OBSOLETE', '')
            if 'gl_WITH_CXX_TESTS' in data:
                self.cache.enableTestFlag(TESTS['c++-test'])
                data = data.replace('gl_WITH_CXX_TESTS', '')
            if 'gl_WITH_LONGRUNNING_TESTS' in data:
                self.cache.enableTestFlag(TESTS['longrunning-test'])
                data = data.replace('gl_WITH_LONGRUNNING_TESTS', '')
            if 'gl_WITH_PRIVILEGED_TESTS' in data:
                self.cache.enableTestFlag(TESTS['privileged-test'])
                data = data.replace('gl_WITH_PRIVILEGED_TESTS', '')
            if 'gl_WITH_UNPORTABLE_TESTS' in data:
                self.cache.enableTestFlag(TESTS['unportable-test'])
                data = data.replace('gl_WITH_UNPORTABLE_TESTS', '')
            if 'gl_WITH_ALL_TESTS' in data:
                self.cache.enableTestFlag(TESTS['all-test'])
                data = data.replace('gl_WITH_ALL_TESTS', '')
            # Find string values
            result = dict(pattern.findall(data))
            values = cleaner([result.get(key, '') for key in keys])
            tempdict = dict(zip(keys, values))
            if 'gl_LGPL' in tempdict:
                lgpl = cleaner(tempdict['gl_LGPL'])
                if lgpl.isdecimal():
                    self.cache.setLGPL(int(self.cache['lgpl']))
            else:  # if 'gl_LGPL' not in tempdict
                self.cache.setLGPL(False)
            if tempdict['gl_LIB']:
                self.cache.setLibName(cleaner(tempdict['gl_LIB']))
            if tempdict['gl_LOCAL_DIR']:
                self.cache.setLocalDir(cleaner(tempdict['gl_LOCAL_DIR']))
            if tempdict['gl_MODULES']:
                self.cache.setModules(cleaner(tempdict['gl_MODULES'].split()))
            if tempdict['gl_AVOID']:
                self.cache.setAvoids(cleaner(tempdict['gl_AVOID'].split()))
            if tempdict['gl_SOURCE_BASE']:
                self.cache.setSourceBase(cleaner(tempdict['gl_SOURCE_BASE']))
            if tempdict['gl_M4_BASE']:
                self.cache.setM4Base(cleaner(tempdict['gl_M4_BASE']))
            if tempdict['gl_PO_BASE']:
                self.cache.setPoBase(cleaner(tempdict['gl_PO_BASE']))
            if tempdict['gl_DOC_BASE']:
                self.cache.setDocBase(cleaner(tempdict['gl_DOC_BASE']))
            if tempdict['gl_TESTS_BASE']:
                self.cache.setTestsBase(cleaner(tempdict['gl_TESTS_BASE']))
            if tempdict['gl_MAKEFILE_NAME']:
                self.cache.setMakefile(cleaner(tempdict['gl_MAKEFILE_NAME']))
            if tempdict['gl_MACRO_PREFIX']:
                self.cache.setMacroPrefix(cleaner(tempdict['gl_MACRO_PREFIX']))
            if tempdict['gl_PO_DOMAIN']:
                self.cache.setPoDomain(cleaner(tempdict['gl_PO_DOMAIN']))
            if tempdict['gl_WITNESS_C_MACRO']:
                self.cache.setWitnessCMacro(
                    cleaner(tempdict['gl_WITNESS_C_MACRO']))

            # Get cached filelist from gnulib-comp.m4.
            destdir, m4base = self.config.getDestDir(), self.config.getM4Base()
            path = joinpath(destdir, m4base, 'gnulib-comp.m4')
            if isfile(path):
                with codecs.open(path, 'rb', 'UTF-8') as file:
                    data = file.read()
                regex = 'AC_DEFUN\\(\\[%s_FILE_LIST\\], \\[(.*?)\\]\\)' % \
                    self.cache['macro_prefix']
                pattern = compiler(regex, re.S | re.M)
                self.cache.setFiles(pattern.findall(data)[-1].strip().split())

        # The self.config['localdir'] defaults to the cached one. Recall that the
        # cached one is relative to $destdir, whereas the one we use is relative
        # to . or absolute.
        if not self.config['localdir']:
            if self.cache['localdir']:
                if isabs(self.config['destdir']):
                    localdir = joinpath(
                        self.config['destdir'], self.cache['localdir'])
                else:  # if not isabs(self.config['destdir'])
                    if isabs(self.cache['localdir']):
                        localdir = joinpath(
                            self.config['destdir'], self.cache['localdir'])
                    else:  # if not isabs(self.cache['localdir'])
                        # NOTE: I NEED TO IMPLEMENT RELATIVE_CONCAT
                        localdir = os.path.relpath(joinpath(self.config['destdir'],
                                                            self.cache['localdir']))
                self.config.setLocalDir(localdir)

        if self.mode != MODES['import']:
            if self.cache['m4base'] and \
                    (self.config['m4base'] != self.cache['m4base']):
                raise(GLError(5, m4base))

            # Perform actions with modules. In --add-import, append each given module
            # to the list of cached modules; in --remove-import, remove each given
            # module from the list of cached modules; in --update, simply set
            # self.config['modules'] to its cached version.
            new, old = self.config.getModules(), self.cache.getModules()
            if self.mode == MODES['add-import']:
                modules = sorted(set(new + old))
            elif self.mode == MODES['remove-import']:
                modules = [module for module in old if module in new]
            elif self.mode == MODES['update']:
                modules = self.cache.getModules()

            # If user tries to apply conddeps and testflag['tests'] together.
            if self.config['tests'] and self.config['conddeps']:
                raise(GLError(10, None))

            # Update configuration dictionary.
            self.config.update(self.cache)
            for key in config.keys():
                value = config[key]
                if not config.isdefault(key, value):
                    self.config.update_key(config, key)
            self.config.setModules(modules)

        # Check if conddeps is enabled together with inctests.
        inctests = self.config.checkTestFlag(TESTS['tests'])
        if self.config['conddeps'] and inctests:
            raise(GLError(10, None))

        # Define GLImport attributes.
        self.emiter = GLEmiter(self.config)
        self.filesystem = GLFileSystem(self.config)
        self.modulesystem = GLModuleSystem(self.config)
        self.moduletable = GLModuleTable(self.config, list())
        self.makefiletable = GLMakefileTable(self.config)

    def __repr__(self):
        '''x.__repr__ <==> repr(x)'''
        result = '<pygnulib.GLImport %s>' % hex(id(self))
        return(result)

    def rewrite_old_files(self, files):
        '''GLImport.rewrite_old_files(files) -> list

        Replace auxdir, docbase, sourcebase, m4base and testsbase from default
        to their version from cached config.'''
        if type(files) is not list:
            raise(TypeError(
                'files argument must has list type, not %s' % type(files).__name__))
        files = \
            [  # Begin to convert bytes to string
                file.decode(ENCS['default']) \
                if type(file) is bytes else file \
                for file in files
            ]  # Finish to convert bytes to string
        for file in files:
            if type(file) is not string:
                raise(TypeError('each file must be a string instance'))
        files = sorted(set(files))
        files = ['%s%s' % (file, os.path.sep) for file in files]
        auxdir = self.cache['auxdir']
        docbase = self.cache['docbase']
        sourcebase = self.cache['sourcebase']
        m4base = self.cache['m4base']
        testsbase = self.cache['testsbase']
        result = list()
        for file in files:
            if file.startswith('build-aux/'):
                path = constants.substart('build-aux/', '%s/' % auxdir, file)
            elif file.startswith('doc/'):
                path = constants.substart('doc/', '%s/' % docbase, file)
            elif file.startswith('lib/'):
                path = constants.substart('lib/', '%s/' % sourcebase, file)
            elif file.startswith('m4/'):
                path = constants.substart('m4/', '%s/' % m4base, file)
            elif file.startswith('tests/'):
                path = constants.substart('tests/', '%s/' % testsbase, file)
            elif file.startswith('tests=lib/'):
                path = constants.substart(
                    'tests=lib/', '%s/' % testsbase, file)
            elif file.startswith('top/'):
                path = constants.substart('top/', '', file)
            else:  # file is not a special file
                path = file
            result += [os.path.normpath(path)]
        result = sorted(set(result))
        return(list(result))

    def rewrite_new_files(self, files):
        '''GLImport.rewrite_new_files(files)

        Replace auxdir, docbase, sourcebase, m4base and testsbase from default
        to their version from config.'''
        if type(files) is not list:
            raise(TypeError(
                'files argument must has list type, not %s' % type(files).__name__))
        files = \
            [  # Begin to convert bytes to string
                file.decode(ENCS['default']) \
                if type(file) is bytes else file \
                for file in files
            ]  # Finish to convert bytes to string
        for file in files:
            if type(file) is not string:
                raise(TypeError('each file must be a string instance'))
        files = sorted(set(files))
        auxdir = self.config['auxdir']
        docbase = self.config['docbase']
        sourcebase = self.config['sourcebase']
        m4base = self.config['m4base']
        testsbase = self.config['testsbase']
        result = list()
        for file in files:
            if file.startswith('build-aux/'):
                path = constants.substart('build-aux/', '%s/' % auxdir, file)
            elif file.startswith('doc/'):
                path = constants.substart('doc/', '%s/' % docbase, file)
            elif file.startswith('lib/'):
                path = constants.substart('lib/', '%s/' % sourcebase, file)
            elif file.startswith('m4/'):
                path = constants.substart('m4/', '%s/' % m4base, file)
            elif file.startswith('tests/'):
                path = constants.substart('tests/', '%s/' % testsbase, file)
            elif file.startswith('tests=lib/'):
                path = constants.substart(
                    'tests=lib/', '%s/' % testsbase, file)
            elif file.startswith('top/'):
                path = constants.substart('top/', '', file)
            else:  # file is not a special file
                path = file
            result += [os.path.normpath(path)]
        result = sorted(set(result))
        return(list(result))

    def actioncmd(self):
        '''Return command-line invocation comment.'''
        modules = self.config.getModules()
        avoids = self.config.getAvoids()
        destdir = self.config.getDestDir()
        localdir = self.config.getLocalDir()
        auxdir = self.config.getAuxDir()
        sourcebase = self.config.getSourceBase()
        m4base = self.config.getM4Base()
        docbase = self.config.getDocBase()
        pobase = self.config.getPoBase()
        testsbase = self.config.getTestsBase()
        testflags = self.config.getTestFlags()
        conddeps = self.config.checkCondDeps()
        libname = self.config.getLibName()
        lgpl = self.config.getLGPL()
        makefile = self.config.getMakefile()
        libtool = self.config.checkLibtool()
        macro_prefix = self.config.getMacroPrefix()
        witness_c_macro = self.config.getWitnessCMacro()
        podomain = self.config.getPoDomain()
        vc_files = self.config.checkVCFiles()
        verbose = self.config.getVerbosity()

        # Create command-line invocation comment.
        actioncmd = 'gnulib-tool --import'
        actioncmd += ' --dir=%s' % destdir
        if localdir:
            actioncmd += ' --local-dir=%s' % localdir
        actioncmd += ' --lib=%s' % libname
        actioncmd += ' --source-base=%s' % sourcebase
        actioncmd += ' --m4-base=%s' % m4base
        if pobase:
            actioncmd += ' --po-base=%s' % pobase
        actioncmd += ' --doc-base=%s' % docbase
        actioncmd += ' --tests-base=%s' % testsbase
        actioncmd += ' --aux-dir=%s' % auxdir
        if self.config.checkTestFlag(TESTS['tests']):
            actioncmd += ' --with-tests'
        if self.config.checkTestFlag(TESTS['obsolete']):
            actioncmd += ' --with-obsolete'
        if self.config.checkTestFlag(TESTS['c++-test']):
            actioncmd += ' --with-c++-tests'
        if self.config.checkTestFlag(TESTS['longrunning-test']):
            actioncmd += ' --with-longrunning-tests'
        if self.config.checkTestFlag(TESTS['privileged-test']):
            actioncmd += ' --with-privileged-test'
        if self.config.checkTestFlag(TESTS['unportable-test']):
            actioncmd += ' --with-unportable-tests'
        if self.config.checkTestFlag(TESTS['all-test']):
            actioncmd += ' --with-all-tests'
        for module in avoids:
            actioncmd += ' --avoid=%s' % module
        if lgpl:
            if lgpl == True:
                actioncmd += ' --lgpl'
            else:  # if lgpl != True
                actioncmd += ' --lgpl=%s' % lgpl
        if makefile:
            actioncmd += ' --makefile-name=%s' % makefile
        if conddeps:
            actioncmd += ' --conditional-dependencies'
        else:  # if not conddeps
            actioncmd += ' --no-conditional-dependencies'
        if libtool:
            actioncmd += ' --libtool'
        else:  # if not libtool
            actioncmd += ' --no-libtool'
        actioncmd += ' --macro-prefix=%s' % macro_prefix
        if podomain:
            actioncmd = ' --podomain=%s' % podomain
        if witness_c_macro:
            actioncmd += ' --witness_c_macro=%s' % witness_c_macro
        if vc_files == True:
            actioncmd += ' --vc-files'
        elif vc_files == False:
            actioncmd += ' --no-vc-files'
        actioncmd += ' '  # Add a space
        actioncmd += ' '.join(modules)
        return(actioncmd)

    def gnulib_cache(self):
        '''GLImport.gnulib_cache() -> string

        Emit the contents of generated $m4base/gnulib-cache.m4 file.
        GLConfig: destdir, localdir, tests, sourcebase, m4base, pobase, docbase,
        testsbase, conddeps, libtool, macro_prefix, podomain, vc_files.'''
        emit = string()
        moduletable = self.moduletable
        actioncmd = self.actioncmd()
        destdir = self.config['destdir']
        localdir = self.config['localdir']
        testflags = list(self.config['testflags'])
        sourcebase = self.config['sourcebase']
        m4base = self.config['m4base']
        pobase = self.config['pobase']
        docbase = self.config['docbase']
        testsbase = self.config['testsbase']
        lgpl = self.config['lgpl']
        libname = self.config['libname']
        makefile = self.config['makefile']
        conddeps = self.config['conddeps']
        libtool = self.config['libtool']
        macro_prefix = self.config['macro_prefix']
        podomain = self.config['podomain']
        witness_c_macro = self.config['witness_c_macro']
        vc_files = self.config['vc_files']
        modules = [str(module) for module in moduletable['base']]
        avoids = [str(avoid) for avoid in moduletable['avoids']]
        emit += self.emiter.copyright_notice()
        emit += '''#
# This file represents the specification of how gnulib-tool is used.
# It acts as a cache: It is written and read by gnulib-tool.
# In projects that use version control, this file is meant to be put under
# version control, like the configure.ac and various Makefile.am files.


# Specification in the form of a command-line invocation:
#   %s

# Specification in the form of a few \
gnulib-tool.m4 macro invocations:\n''' % actioncmd
        if not localdir or localdir.startswith('/'):
            relative_localdir = localdir
        else:  # if localdir or not localdir.startswith('/')
            relative_localdir = constants.relativize(destdir, localdir)
        emit += 'gl_LOCAL_DIR([%s])\n' % relative_localdir
        emit += 'gl_MODULES([\n'
        emit += '  %s\n' % '\n  '.join(modules)
        emit += '])\n'
        if self.config.checkTestFlag(TESTS['obsolete']):
            emit += 'gl_WITH_OBSOLETE\n'
        if self.config.checkTestFlag(TESTS['cxx-tests']):
            emit += 'gl_WITH_CXX_TESTS\n'
        if self.config.checkTestFlag(TESTS['privileged-tests']):
            emit += 'gl_WITH_PRIVILEGED_TESTS\n'
        if self.config.checkTestFlag(TESTS['unportable-tests']):
            emit += 'gl_WITH_UNPORTABLE_TESTS\n'
        if self.config.checkTestFlag(TESTS['all-tests']):
            emit += 'gl_WITH_ALL_TESTS\n'
        emit += 'gl_AVOID([%s])\n' % ' '.join(avoids)
        emit += 'gl_SOURCE_BASE([%s])\n' % sourcebase
        emit += 'gl_M4_BASE([%s])\n' % m4base
        emit += 'gl_PO_BASE([%s])\n' % pobase
        emit += 'gl_DOC_BASE([%s])\n' % docbase
        emit += 'gl_TESTS_BASE([%s])\n' % testsbase
        if self.config.checkTestFlag(TESTS['tests']):
            emit += 'gl_WITH_TESTS\n'
        emit += 'gl_LIB([%s])\n' % libname
        if lgpl != False:
            if lgpl == True:
                emit += 'gl_LGPL\n'
            else:  # if lgpl != True
                emit += 'gl_LGPL([%d])\n' % lgpl
        emit += 'gl_MAKEFILE_NAME([%s])\n' % makefile
        if conddeps:
            emit += 'gl_CONDITIONAL_DEPENDENCIES\n'
        if libtool:
            emit += 'gl_LIBTOOL\n'
        emit += 'gl_MACRO_PREFIX([%s])\n' % macro_prefix
        emit += 'gl_PO_DOMAIN([%s])\n' % podomain
        emit += 'gl_WITNESS_C_MACRO([%s])\n' % witness_c_macro
        if vc_files:
            emit += 'gl_VC_FILES([%s])\n' % vc_files
        if type(emit) is bytes:
            emit = emit.decode(ENCS['default'])
        return(constants.nlconvert(emit))

    def gnulib_comp(self, files):
        '''GLImport.gnulib_comp(files) -> string

        Emit the contents of generated $m4base/gnulib-comp.m4 file.
        GLConfig: destdir, localdir, tests, sourcebase, m4base, pobase, docbase,
        testsbase, conddeps, libtool, macro_prefix, podomain, vc_files.'''
        emit = string()
        assistant = self.assistant
        moduletable = self.moduletable
        destdir = self.config['destdir']
        localdir = self.config['localdir']
        auxdir = self.config['auxdir']
        testflags = list(self.config['testflags'])
        sourcebase = self.config['sourcebase']
        m4base = self.config['m4base']
        pobase = self.config['pobase']
        docbase = self.config['docbase']
        testsbase = self.config['testsbase']
        lgpl = self.config['lgpl']
        libname = self.config['libname']
        makefile = self.config['makefile']
        conddeps = self.config['conddeps']
        libtool = self.config['libtool']
        macro_prefix = self.config['macro_prefix']
        podomain = self.config['podomain']
        witness_c_macro = self.config['witness_c_macro']
        configure_ac = self.config['configure_ac']
        vc_files = self.config['vc_files']
        libtests = self.config['libtests']
        modules = [str(module) for module in moduletable['base']]
        avoids = [str(avoid) for avoid in moduletable['avoids']]
        emit += '# DO NOT EDIT! GENERATED AUTOMATICALLY!\n'
        emit += self.emiter.copyright_notice()
        emit += '''#
# This file represents the compiled summary of the specification in
# gnulib-cache.m4. It lists the computed macro invocations that need
# to be invoked from configure.ac.
# In projects that use version control, this file can be treated like
# other built files.


# This macro should be invoked from %s, in the section
# "Checks for programs", right after AC_PROG_CC, and certainly before
# any checks for libraries, header files, types and library functions.
AC_DEFUN([%s_EARLY],
[
  m4_pattern_forbid([^gl_[A-Z]])dnl the gnulib macro namespace
  m4_pattern_allow([^gl_ES$])dnl a valid locale name
  m4_pattern_allow([^gl_LIBOBJS$])dnl a variable
  m4_pattern_allow([^gl_LTLIBOBJS$])dnl a variable
  AC_REQUIRE([gl_PROG_AR_RANLIB])\n''' % (configure_ac, macro_prefix)
        uses_subdirs = False
        for module in moduletable['main']:
            # Test whether there are some source files in subdirectories.
            for file in module.getFiles():
                if file.startswith('lib/') and file.endswith('.c') and \
                        file.count('/') > 1:
                    uses_subdirs = True
                    break
        if uses_subdirs:
            emit += '  AC_REQUIRE([AM_PROG_CC_C_O])\n'
        for module in moduletable['final']:
            emit += '  # Code from module %s:\n' % str(module)
            snippet = module.getAutoconfSnippet_Early()
            lines = [line for line in snippet.split(
                constants.NL) if line != '']
            if lines:
                emit += '  %s\n' % '\n  '.join(lines)
        emit += '])\n'
        emit += '''
# This macro should be invoked from %s, in the section
# "Check for header files, types and library functions".
AC_DEFUN([%s_INIT],
[\n''' % (configure_ac, macro_prefix)
        if libtool:
            emit += '  AM_CONDITIONAL([GL_COND_LIBTOOL], [true])\n'
            emit += '  gl_cond_libtool=true\n'
        else:  # if not libtool
            emit += '  AM_CONDITIONAL([GL_COND_LIBTOOL], [false])\n'
            emit += '  gl_cond_libtool=false\n'
            emit += '  gl_libdeps=\n'
            emit += '  gl_ltlibdeps=\n'
        replace_auxdir = False
        if auxdir != 'build-aux':
            replace_auxdir = True
        emit += '  gl_m4_base=\'%s\'\n' % m4base
        emit += self.emiter.initmacro_start(macro_prefix)
        emit += '  gl_source_base=\'%s\'\n' % sourcebase
        if witness_c_macro:
            emit += '  m4_pushdef([gl_MODULE_INDICATOR_CONDITION], [%s])\n' % \
                witness_c_macro
        # Emit main autoconf snippets.
        emit += self.emiter.autoconfSnippets(moduletable['main'],
                                             moduletable, assistant, 0, True, False, True, replace_auxdir)
        if witness_c_macro:
            emit += '  m4_popdef([gl_MODULE_INDICATOR_CONDITION])\n'
        emit += '  # End of code from modules\n'
        emit += self.emiter.initmacro_end(macro_prefix)
        emit += '  gltests_libdeps=\n'
        emit += '  gltests_ltlibdeps=\n'
        emit += self.emiter.initmacro_start('%stests' % macro_prefix)
        emit += '  gl_source_base=\'%s\'\n' % testsbase
        # Define a tests witness macro that depends on the package.
        # PACKAGE is defined by AM_INIT_AUTOMAKE, PACKAGE_TARNAME is defined by
        # AC_INIT.
        # See <https://lists.gnu.org/r/automake/2009-05/msg00145.html>.
        emit += 'changequote(,)dnl\n'
        emit += '  %stests_WITNESS=' % macro_prefix
        emit += 'IN_`echo "${PACKAGE-$PACKAGE_TARNAME}" | LC_ALL=C tr \
abcdefghijklmnopqrstuvwxyz ABCDEFGHIJKLMNOPQRSTUVWXYZ | LC_ALL=C sed -e \
\'s/[^A-Z0-9_]/_/g\'`_GNULIB_TESTS\n'
        emit += 'changequote([, ])dnl\n'
        emit += '  AC_SUBST([%stests_WITNESS])\n' % macro_prefix
        emit += '  gl_module_indicator_condition=$%stests_WITNESS\n' % macro_prefix
        emit += '  m4_pushdef([gl_MODULE_INDICATOR_CONDITION], '
        emit += '[$gl_module_indicator_condition])\n'
        # Emit tests autoconf snippets.
        emit += self.emiter.autoconfSnippets(moduletable['tests'],
                                             moduletable, assistant, 0, True, True, True, replace_auxdir)
        emit += '  m4_popdef([gl_MODULE_INDICATOR_CONDITION])\n'
        emit += self.emiter.initmacro_end('%stests' % macro_prefix)
        # _LIBDEPS and _LTLIBDEPS variables are not needed if this library is
        # created using libtool, because libtool already handles the dependencies.
        if not libtool:
            libname_upper = libname.upper().replace('-', '_')
            emit += '  %s_LIBDEPS="$gl_libdeps"\n' % libname_upper
            emit += '  AC_SUBST([%s_LIBDEPS])\n' % libname_upper
            emit += '  %s_LTLIBDEPS="$gl_ltlibdeps"\n' % libname_upper
            emit += '  AC_SUBST([%s_LTLIBDEPS])\n' % libname_upper
        if libtests:
            emit += '  LIBTESTS_LIBDEPS="$gltests_libdeps"\n'
            emit += '  AC_SUBST([LIBTESTS_LIBDEPS])\n'
        emit += '])\n'
        emit += self.emiter.initmacro_done(macro_prefix, sourcebase)
        emit += self.emiter.initmacro_done('%stests' % macro_prefix, testsbase)
        emit += '''
# This macro records the list of files which have been installed by
# gnulib-tool and may be removed by future gnulib-tool invocations.
AC_DEFUN([%s_FILE_LIST], [\n''' % macro_prefix
        emit += '  %s\n' % '\n  '.join(files)
        emit += '])\n'
        if type(emit) is bytes:
            emit = emit.decode(ENCS['default'])
        return(emit)

    def _done_dir_(self, directory, dirs_added, dirs_removed):
        '''GLImport._done_dir_(directory, dirs_added, dirs_removed)

        This method is used to determine ignore argument for _update_ignorelist_
        method and then call it.'''
        destdir = self.config['destdir']
        if isdir(joinpath(destdir, 'CVS')) or \
                isdir(joinpath(destdir, directory, 'CVS')) or \
                isfile(joinpath(destdir, directory, '.cvsignore')):
            self._update_ignorelist_(directory, '.cvsignore',
                                     dirs_added, dirs_removed)
        if isdir(joinpath(destdir, '.git')) or \
                isfile(joinpath(destdir, directory, '.gitignore')):
            self._update_ignorelist_(directory, '.gitignore',
                                     dirs_added, dirs_removed)

    def _update_ignorelist_(self, directory, ignore, dirs_added, dirs_removed):
        '''GLImport._update_ignorelist_(directory, ignore, dirs_added, dirs_removed)

        Update .gitignore or .cvsignore files.'''
        result = string()
        destdir = self.config['destdir']
        if ignore == '.gitignore':
            anchor = '/'
        else:
            anchor = ''
        srcpath = joinpath(destdir, directory, ignore)
        backupname = '%s~' % srcpath
        if isfile(srcpath):
            if dirs_added or dirs_removed:
                with codecs.open(srcpath, 'rb', 'UTF-8') as file:
                    srcdata = file.read()
                dirs_ignore = sorted(set(srcdata.split('\n')))
                dirs_ignore = [line for line in dirs_ignore if line.strip()]
                srcdata = '\n'.join(sorted(set(dirs_ignore))).strip()
                dirs_ignore += [d for d in dirs_added if d not in dirs_ignore]
                dirs_ignore = [d for d in dirs_ignore if d in dirs_removed]
                dirs_ignore = ['%s%s' % (anchor, d) for d in dirs_ignore]
                dirs_ignore = sorted(set(dirs_ignore))
                destdata = '\n'.join(sorted(set(dirs_ignore))).strip()
                if srcdata != destdata:
                    if not self.config['dryrun']:
                        print('Updating %s (backup in %s)' %
                              (srcpath, backupname))
                        shutil.copy2(srcpath, backupname)
                        result = string()
                        with codecs.open(srcpath, 'ab', 'UTF-8') as file:
                            file.write(destdata)
                    else:  # if self.config['dryrun']
                        print('Updating %s (backup in %s)' %
                              (srcpath, backupname))
        else:  # if not isfile(srcpath)
            if dirs_added:
                if not self.config['dryrun']:
                    print('Creating %s' % srcpath)
                    dirs_added = sorted(set(dirs_added))
                    dirs_added = ['%s%s' % (anchor, d) for d in dirs_added]
                    if ignore == '.cvsignore':
                        dirs_added = ['.deps', '.dirstamp'] + dirs_added
                    with codecs.open(srcpath, 'wb', 'UTF-8') as file:
                        file.write('\n'.join(dirs_added))
                        file.write('\n')
                else:  # if self.config['dryrun']
                    print('Create %s' % srcpath)

    def prepare(self):
        '''Make all preparations before the execution of the code.
        Returns filetable and sed transformers, which change the license.'''
        destdir = self.config['destdir']
        localdir = self.config['localdir']
        auxdir = self.config['auxdir']
        modules = list(self.config['modules'])
        avoids = list(self.config['avoids'])
        testflags = list(self.config['testflags'])
        sourcebase = self.config['sourcebase']
        m4base = self.config['m4base']
        pobase = self.config['pobase']
        docbase = self.config['docbase']
        testsbase = self.config['testsbase']
        lgpl = self.config['lgpl']
        copyrights = self.config['copyrights']
        libname = self.config['libname']
        makefile = self.config['makefile']
        conddeps = self.config['conddeps']
        libtool = self.config['libtool']
        macro_prefix = self.config['macro_prefix']
        podomain = self.config['podomain']
        witness_c_macro = self.config['witness_c_macro']
        vc_files = self.config['vc_files']
        configure_ac = self.config['configure_ac']
        ac_version = self.config['ac_version']
        verbose = self.config['verbosity']
        base_modules = sorted(
            set([self.modulesystem.find(m) for m in modules]))
        avoids = sorted(set([self.modulesystem.find(a) for a in avoids]))

        # Perform transitive closure.
        self.moduletable.setAvoids(avoids)
        final_modules = self.moduletable.transitive_closure(base_modules)

        # Show final module list.
        if verbose >= 0:
            bold_on = ''
            bold_off = ''
            term = os.getenv('TERM')
            if term == 'xterm':
                bold_on = '\x1b[1m'
                bold_off = '\x1b[0m'
            print('Module list with included dependencies (indented):')
            for module in final_modules:
                if str(module) in self.config.getModules():
                    print('  %s%s%s' % (bold_on, module, bold_off))
                else:  # if str(module) not in self.config.getModules()
                    print('    %s' % module)

        # Separate modules into main_modules and tests_modules.
        modules = self.moduletable.transitive_closure_separately(
            base_modules, final_modules)
        main_modules, tests_modules = modules

        # Transmit base_modules, final_modules, main_modules and tests_modules.
        self.moduletable.setBaseModules(base_modules)
        self.moduletable.setFinalModules(final_modules)
        self.moduletable.setMainModules(main_modules)
        self.moduletable.setTestsModules(tests_modules)

        # Print main_modules and tests_modules.
        if verbose >= 1:
            print('Main module list:')
            for module in main_modules:
                print('  %s' % str(module))
            print('Tests-related module list:')
            for module in tests_modules:
                print('  %s' % str(module))

        # Determine whether a $testsbase/libtests.a is needed.
        libtests = False
        for module in tests_modules:
            files = module.getFiles()
            for file in files:
                if file.startswith('lib/'):
                    libtests = True
                    break
        if libtests:
            self.config.enableLibtests()

        # Add dummy package if it is needed.
        main_modules = self.moduletable.add_dummy(main_modules)
        if libtests:  # if we need to use libtests.a
            tests_modules = self.moduletable.add_dummy(tests_modules)

        # Check license incompatibilities.
        listing = list()
        compatibilities = dict()
        incompatibilities = string()
        compatibilities['all'] = ['GPLed build tool', 'public domain', 'unlimited',
                                  'unmodifiable license text']
        compatibilities[3] = ['LGPL', 'LGPLv2+', 'LGPLv3+']
        compatibilities[2] = ['LGPLv2+']
        if lgpl:
            for module in main_modules:
                license = module.getLicense()
                if license not in compatibilities['all']:
                    if lgpl == 3 or lgpl == True:
                        if license not in compatibilities[3]:
                            listing.append(tuple([str(module), license]))
                    elif lgpl == 2:
                        if license not in compatibilities[2]:
                            listing.append(tuple([str(module), license]))
            if listing:
                raise(GLError(11, listing))

        # Print notices from modules.
        for module in main_modules:
            notice = module.getNotice()
            notice = notice.strip()
            if notice:
                print('Notice from module %s:' % str(module))
                pattern = compiler('^(.*?)$', re.S | re.M)
                notice = pattern.sub('  \\1', notice)
                print(notice)

        # Determine script to apply to imported library files.
        lgpl2gpl = '''
      s/GNU Lesser General/GNU General/g
      s/Lesser General Public License/General Public License/g
      s/GNU Library General/GNU General/g
      s/Library General Public License/General Public License/g
      s/version 2\\(.1\\)\\{0,1\\}\\([ ,]\\)/version 3\\2/g'''
        sed_transform_lib_file = string()
        if 'config-h' in [str(module) for module in main_modules]:
            sed_transform_lib_file += '''
        s/^#ifdef[\t ]*HAVE_CONFIG_H[\t ]*$/#if 1/
      '''
        sed_transform_main_lib_file = sed_transform_lib_file
        if copyrights:
            if lgpl:  # if lgpl is enabled
                if lgpl == 3:
                    sed_transform_main_lib_file += '''
            s/GNU General/GNU Lesser General/g
            s/General Public License/Lesser General Public License/g
            s/Lesser Lesser General Public License/Lesser General Public''' \
                        + ' License/g'
                elif lgpl == 2:
                    sed_transform_main_lib_file += '''
            s/GNU General/GNU Lesser General/g
            s/General Public License/Lesser General Public License/g
            s/Lesser Lesser General Public License/Lesser General Public''' \
                        + '''License/g
            s/version [23]\\([ ,]\\)/version 2.1\\1/g'''
            else:  # if lgpl is disabled
                sed_transform_main_lib_file += lgpl2gpl

        # Determine script to apply to auxiliary files that go into $auxdir/.
        sed_transform_build_aux_file = string()
        if copyrights:
            sed_transform_build_aux_file += lgpl2gpl

        # Determine script to apply to library files that go into $testsbase/.
        sed_transform_testsrelated_lib_file = sed_transform_lib_file
        if copyrights:
            sed_transform_testsrelated_lib_file += lgpl2gpl

        # Determine the final file lists.
        main_filelist, tests_filelist = \
            self.moduletable.filelist_separately(main_modules, tests_modules)
        filelist = sorted(
            set(main_filelist + tests_filelist), key=string.lower)
        if not filelist:
            raise(GLError(12, None))

        # Print list of files.
        if verbose >= 0:
            print('File list:')
            for file in filelist:
                if file.startswith('tests=lib/'):
                    rest = file[10:]
                    print('  lib/%s -> tests/%s' % (rest, rest))
                else:
                    print('  %s' % file)

        # Prepare basic filelist and basic old_files/new_files variables.
        filelist = sorted(set(filelist))
        new_files = filelist + ['m4/gnulib-tool.m4']
        old_files = list(self.cache['files'])
        path = joinpath(destdir, m4base, 'gnulib-tool.m4')
        if isfile(path):
            old_files += [joinpath('m4', 'gnulib-tool.m4')]

        # Construct tables and transformers.
        transformers = dict()
        transformers['lib'] = string(sed_transform_lib_file)
        transformers['aux'] = string(sed_transform_build_aux_file)
        transformers['main'] = string(sed_transform_main_lib_file)
        transformers['tests'] = string(sed_transform_testsrelated_lib_file)
        old_table = list()
        new_table = list()
        for src in old_files:
            dest = self.rewrite_old_files([src])[-1]
            old_table += [tuple([dest, src])]
        for src in new_files:
            dest = self.rewrite_new_files([src])[-1]
            new_table += [tuple([dest, src])]
        old_table = sorted(set(old_table))
        new_table = sorted(set(new_table))

        # Prepare the filetable.
        filetable = dict()
        filetable['all'] = sorted(set(filelist))
        filetable['old'] = \
            sorted(set(old_table), key=lambda t: tuple(t[0].lower()))
        filetable['new'] = \
            sorted(set(new_table), key=lambda t: tuple(t[0].lower()))
        filetable['added'] = list()
        filetable['removed'] = list()

        # Return the result.
        result = tuple([filetable, transformers])
        return(result)

    def execute(self, filetable, transformers):
        '''Perform operations on the lists of files, which are given in a special
        format except filelist argument. Such lists of files can be created using
        GLImport.prepare() function.'''
        if type(filetable) is not dict:
            raise(TypeError('filetable must be a dict, not %s' %
                            type(filetable).__name__))
        for key in ['all', 'old', 'new', 'added', 'removed']:
            if key not in filetable:
                raise(KeyError('filetable must contain key %s' % repr(key)))
        destdir = self.config['destdir']
        localdir = self.config['localdir']
        auxdir = self.config['auxdir']
        modules = list(self.config['modules'])
        avoids = list(self.config['avoids'])
        testflags = list(self.config['testflags'])
        sourcebase = self.config['sourcebase']
        m4base = self.config['m4base']
        pobase = self.config['pobase']
        docbase = self.config['docbase']
        testsbase = self.config['testsbase']
        lgpl = self.config['lgpl']
        copyrights = self.config['copyrights']
        libname = self.config['libname']
        makefile = self.config['makefile']
        conddeps = self.config['conddeps']
        libtool = self.config['libtool']
        macro_prefix = self.config['macro_prefix']
        podomain = self.config['podomain']
        witness_c_macro = self.config['witness_c_macro']
        vc_files = self.config['vc_files']
        configure_ac = self.config['configure_ac']
        ac_version = self.config['ac_version']
        verbose = self.config['verbosity']
        actioncmd = self.actioncmd()

        # Create all necessary directories.
        dirs = list()
        if pobase:
            dirs += [pobase]
        if [file for file in filetable['all'] if file.startswith('doc/')]:
            dirs += [docbase]
        dirs += [sourcebase, m4base, auxdir]
        dirs += [os.path.dirname(pair[0]) for pair in filetable['new']]
        dirs = sorted(set([joinpath(destdir, d) for d in dirs]))
        for directory in dirs:
            if not isdir(directory):
                print('Creating directory %s' % directory)
                if not self.config['dryrun']:
                    try:  # Try to create directory
                        os.makedirs(directory)
                    except Exception as error:
                        raise(GLError(13, directory))
                else:  # if self.config['dryrun']
                    print('Create directory %s' % directory)

        # Create GLFileAssistant instance to process files.
        self.assistant = GLFileAssistant(self.config, transformers)

        # Files which are in filetable['old'] and not in filetable['new'].
        # They will be removed and added to filetable['removed'] list.
        pairs = [f for f in filetable['old'] if f not in filetable['old']]
        pairs = sorted(set(pairs), key=lambda t: tuple(t[0].lower()))
        files = sorted(set(pair[0] for pair in pairs))
        for file in files:
            path = joinpath(destdir, file)
            if isfile(path) or os.path.islink(path):
                if not self.config['dryrun']:
                    backup = string('%s~' % path)
                    print('Removing file %s (backup in )' % (path, backup))
                    try:  # Try to move file
                        if os.path.exists(backup):
                            os.remove(backup)
                        shutil.move(path, '%s~' % path)
                    except Exception as error:
                        raise(GLError(14, file))
                else:  # if self.config['dryrun']
                    print('Remove file %s (backup in %s~)' % (path, path))
                filetable['removed'] += [file]

        # Files which are in filetable['new'] and not in filetable['old'].
        # They will be added/updated and added to filetable['added'] list.
        already_present = False
        pairs = [f for f in filetable['new'] if f not in filetable['old']]
        pairs = sorted(set(pairs))
        for pair in pairs:
            original = pair[1]
            rewritten = pair[0]
            self.assistant.setOriginal(original)
            self.assistant.setRewritten(rewritten)
            self.assistant.add_or_update(already_present)

        # Files which are in filetable['new'] and in filetable['old'].
        # They will be added/updated and added to filetable['added'] list.
        already_present = True
        pairs = [f for f in filetable['new'] if f in filetable['old']]
        pairs = sorted(set(pairs))
        for pair in pairs:
            original = pair[1]
            rewritten = pair[0]
            self.assistant.setOriginal(original)
            self.assistant.setRewritten(rewritten)
            self.assistant.add_or_update(already_present)

        # Add files which were added to the list of filetable['added'].
        filetable['added'] += self.assistant.getFiles()
        filetable['added'] = sorted(set(filetable['added']))

        # Determine include_guard_prefix.
        include_guard_prefix = self.config['include_guard_prefix']

        # Determine makefile name.
        if not makefile:
            makefile_am = string('Makefile.am')
        else:  # if makefile
            makefile_am = makefile

        # Create normal Makefile.ams.
        for_test = False

        # Setup list of Makefile.am edits that are to be performed afterwards.
        # Some of these edits apply to files that we will generate; others are
        # under the responsibility of the developer.
        makefile_am_edits = dict()
        if makefile_am == 'Makefile.am':
            sourcebase_dir = os.path.dirname(sourcebase)
            sourcebase_base = os.path.basename(sourcebase)
            self.makefiletable.editor(
                sourcebase_dir, 'SUBDIRS', sourcebase_base)
        if pobase:
            pobase_dir = os.path.dirname(pobase)
            pobase_base = os.path.basename(pobase)
            self.makefiletable.editor(pobase_dir, 'SUBDIRS', pobase_base)
        if self.config.checkTestFlag(TESTS['tests']):
            if makefile_am == 'Makefile.am':
                testsbase_dir = os.path.dirname(testsbase)
                testsbase_base = os.path.basename(testsbase)
                self.makefiletable.editor(
                    testsbase_dir, 'SUBDIRS', testsbase_base)
        self.makefiletable.editor('', 'ACLOCAL_AMFLAGS', '-I %s' % m4base)
        self.makefiletable.parent()

        # Create library makefile.
        basename = joinpath(sourcebase, makefile_am)
        tmpfile = self.assistant.tmpfilename(basename)
        emit, uses_subdirs = self.emiter.lib_Makefile_am(basename,
                                                         self.moduletable['main'], self.moduletable, self.makefiletable,
                                                         actioncmd, for_test)
        with codecs.open(tmpfile, 'wb', 'UTF-8') as file:
            file.write(emit)
        filename, backup, flag = self.assistant.super_update(basename, tmpfile)
        if flag == 1:
            if not self.config['dryrun']:
                print('Updating %s (backup in %s)' % (filename, backup))
            else:  # if self.config['dryrun']
                print('Update %s (backup in %s)' % (filename, backup))
        elif flag == 2:
            if not self.config['dryrun']:
                print('Creating %s' % filename)
            else:  # if self.config['dryrun']:
                print('Create %s' % filename)
            filetable['added'] += [filename]
        if isfile(tmpfile):
            os.remove(tmpfile)

        # Create po/ directory.
        filesystem = GLFileSystem(self.config)
        if pobase:
            # Create po makefile and auxiliary files.
            for file in ['Makefile.in.in', 'remove-potcdate.sin']:
                tmpfile = self.assistant.tmpfilename(joinpath(pobase, file))
                path = joinpath('build-aux', 'po', file)
                lookedup, flag = filesystem.lookup(path)
                shutil.move(lookedup, tmpfile)
                basename = joinpath(pobase, file)
                filename, backup, flag = self.assistant.super_update(
                    basename, tmpfile)
                if flag == 1:
                    if not self.config['dryrun']:
                        print('Updating %s (backup in %s)' %
                              (filename, backup))
                    else:  # if self.config['dryrun']
                        print('Update %s (backup in %s)' % (filename, backup))
                elif flag == 2:
                    if not self.config['dryrun']:
                        print('Creating %s' % filename)
                    else:  # if self.config['dryrun']:
                        print('Create %s' % filename)
                    filetable['added'] += [filename]
            if isfile(tmpfile):
                os.remove(tmpfile)

            # Create po makefile parameterization, part 1.
            basename = joinpath(pobase, 'Makevars')
            tmpfile = self.assistant.tmpfilename(basename)
            emit = self.emiter.po_Makevars()
            with codecs.open(tmpfile, 'wb', 'UTF-8') as file:
                file.write(emit)
            filename, backup, flag = self.assistant.super_update(
                basename, tmpfile)
            if flag == 1:
                if not self.config['dryrun']:
                    print('Updating %s (backup in %s)' % (filename, backup))
                else:  # if self.config['dryrun']
                    print('Update %s (backup in %s)' % (filename, backup))
            elif flag == 2:
                if not self.config['dryrun']:
                    print('Creating %s' % filename)
                else:  # if self.config['dryrun']:
                    print('Create %s' % filename)
                filetable['added'] += [filename]
            if isfile(tmpfile):
                os.remove(tmpfile)

            # Create po makefile parameterization, part 2.
            basename = joinpath(pobase, 'POTFILES.in')
            tmpfile = self.assistant.tmpfilename(basename)
            with codecs.open(tmpfile, 'wb', 'UTF-8') as file:
                file.write(self.emiter.po_POTFILES_in(filetable['all']))
            basename = joinpath(pobase, 'POTFILES.in')
            filename, backup, flag = self.assistant.super_update(
                basename, tmpfile)
            if flag == 1:
                if not self.config['dryrun']:
                    print('Updating %s (backup in %s)' % (filename, backup))
                else:  # if self.config['dryrun']
                    print('Update %s (backup in %s)' % (filename, backup))
            elif flag == 2:
                if not self.config['dryrun']:
                    print('Creating %s' % filename)
                else:  # if self.config['dryrun']:
                    print('Create %s' % filename)
                filetable['added'] += [filename]
            if isfile(tmpfile):
                os.remove(tmpfile)

            # Fetch PO files.
            TP_URL = 'https://translationproject.org/latest/'
            if not self.config['dryrun']:
                print('Fetching gnulib PO files from %s' % TP_URL)
                os.chdir(joinpath(destdir, pobase))
                args = ['wget', '--no-verbose', '--mirror', '--level=1', '-nd', '-A.po', '-P', '.',
                        '%sgnulib/' % TP_URL]
                sp.call(args, shell=True)
            else:  # if self.config['dryrun']
                print('Fetch gnulib PO files from %s' % TP_URL)

            # Create po/LINGUAS.
            basename = joinpath(pobase, 'LINGUAS')
            if not self.config['dryrun']:
                tmpfile = self.assistant.tmpfilename(basename)
                data = string('# Set of available languages.\n')
                files = [constants.subend('.po', '', file)
                         for file in os.listdir(joinpath(destdir, pobase))]
                files = [file.decode(ENCS['default']) if type(file) is bytes
                         else file for file in files]
                data += '\n'.join(files)
                with codecs.open(tmpfile, 'wb', 'UTF-8') as file:
                    file.write(data)
                filename, backup, flag = self.assistant.super_update(
                    basename, tmpfile)
                if flag == 1:
                    print('Updating %s (backup in %s)' % (filename, backup))
                elif flag == 2:
                    print('Creating %s' % filename)
                    filetable['added'] += [filename]
                if isfile(tmpfile):
                    os.remove(tmpfile)
            else:  # if not self.config['dryrun']
                backupname = '%s~' % basename
                if isfile(destdir, basename):
                    print('Update %s (backup in %s)' % (basename, backupname))
                else:  # if not isfile(destdir, basename)
                    print('Create %s' % basename)

        # Create m4/gnulib-cache.m4.
        basename = joinpath(m4base, 'gnulib-cache.m4')
        tmpfile = self.assistant.tmpfilename(basename)
        emit = self.gnulib_cache()
        with codecs.open(tmpfile, 'wb', 'UTF-8') as file:
            file.write(emit)
        filename, backup, flag = self.assistant.super_update(basename, tmpfile)
        if flag == 1:
            if not self.config['dryrun']:
                print('Updating %s (backup in %s)' % (filename, backup))
            else:  # if self.config['dryrun']
                print('Update %s (backup in %s)' % (filename, backup))
        elif flag == 2:
            if not self.config['dryrun']:
                print('Creating %s' % filename)
            else:  # if self.config['dryrun']:
                print('Create %s' % filename)
                if emit[-2:] == '\r\n':
                    emit = emit[:-2]
                elif emit[-1:] == '\n':
                    emit = emit[:-1]
                print(emit)
        if isfile(tmpfile):
            os.remove(tmpfile)

        # Create m4/gnulib-comp.m4.
        basename = joinpath(m4base, 'gnulib-comp.m4')
        tmpfile = self.assistant.tmpfilename(basename)
        emit = self.gnulib_comp(filetable['all'])
        with codecs.open(tmpfile, 'wb', 'UTF-8') as file:
            file.write(emit)
        filename, backup, flag = self.assistant.super_update(basename, tmpfile)
        if flag == 1:
            if not self.config['dryrun']:
                print('Updating %s (backup in %s)' % (filename, backup))
            else:  # if self.config['dryrun']
                print('Update %s (backup in %s)' % (filename, backup))
        elif flag == 2:
            if not self.config['dryrun']:
                print('Creating %s' % filename)
            else:  # if self.config['dryrun']:
                print('Create %s' % filename)
                if emit[-2:] == '\r\n':
                    emit = emit[:-2]
                elif emit[-1:] == '\n':
                    emit = emit[:-1]
                print(emit)
        if isfile(tmpfile):
            os.remove(tmpfile)

        # Create tests Makefile.
        inctests = self.config.checkTestFlag(TESTS['tests'])
        if inctests:
            basename = joinpath(testsbase, makefile_am)
            tmpfile = self.assistant.tmpfilename(basename)
            emit, uses_subdirs = self.emiter.lib_Makefile_am(basename,
                                                             self.moduletable['tests'], self.moduletable, self.makefiletable,
                                                             actioncmd, for_test)
            with codecs.open(tmpfile, 'wb', 'UTF-8') as file:
                file.write(emit)
            filename, backup, flag = self.assistant.super_update(
                basename, tmpfile)
            if flag == 1:
                if not self.config['dryrun']:
                    print('Updating %s (backup in %s)' % (filename, backup))
                else:  # if self.config['dryrun']
                    print('Update %s (backup in %s)' % (filename, backup))
            elif flag == 2:
                if not self.config['dryrun']:
                    print('Creating %s' % filename)
                else:  # if self.config['dryrun']:
                    print('Create %s' % filename)
                filetable['added'] += [filename]
            if isfile(tmpfile):
                os.remove(tmpfile)

        # Update the .cvsignore and .gitignore files.
        ignorelist = list()
        filetable['added'] = sorted(set(filetable['added']))
        filetable['removed'] = sorted(set(filetable['added']))
        for file in filetable['added']:
            directory, basename = os.path.split(file)
            ignorelist += [tuple([directory, '|A|', basename])]
        for file in filetable['removed']:
            directory, basename = os.path.split(file)
            ignorelist += [tuple([directory, '|R|', basename])]
        last_dir = string()
        last_dirs_added = list()
        last_dirs_removed = list()
        for row in ignorelist:
            next_dir = row[0]
            operand = row[1]
            filename = row[2]
            if next_dir != last_dir:
                self._done_dir_(last_dir, last_dirs_added, last_dirs_removed)
                last_dir = next_dir
                last_dirs_added = list()
                last_dirs_removed = list()
            if operand == '|A|':
                last_dirs_added += [filename]
            elif operand == '|R|':
                last_dirs_removed += [filename]
        self._done_dir_(last_dir, last_dirs_added, last_dirs_removed)
        exit()

        # Finish the work.
        print('Finished.\n')
        print('You may need to add #include directives \
for the following .h files.')
        modules = sorted(set([module for module in self.moduletable['base']
                              if module in self.moduletable['main']]))
        # First the #include <...> directives without #ifs, sorted for convenience,
        # then the #include "..." directives without #ifs, sorted for convenience,
        # then the #include directives that are surrounded by #ifs. Not sorted.
        includes_angles = list()
        includes_quotes = list()
        includes_if = list()
        for module in modules:
            include = module.getInclude()
            for include in include.split('\n'):
                if '%s#if' % constants.NL in '%s%s' % (constants.NL, include):
                    includes_if += [include]
                # if '%s#if' % constants.NL in '%s%s' % (constants.NL, include)
                else:
                    if 'include "' in include:
                        includes_quotes += [include]
                    else:  # if 'include "' not in include
                        includes_angles += [include]
        includes_angles = sorted(set(includes_angles))
        includes_quotes = sorted(set(includes_quotes))
        includes = includes_angles + includes_quotes + includes_if
        includes = [include for include in includes if include.split()]
        for include in includes:
            print('  %s' % include)

        # Get link directives.
        links = [module.getLink() for module in self.moduletable['main']]
        ulinks = list()
        for link in links:
            for lib in link:
                ulinks += [lib]
        ulinks = sorted(set(ulinks))
        if ulinks:
            print('''
You may need to use the following Makefile variables when linking.
Use them in <program>_LDADD when linking a program, or
in <library>_a_LDFLAGS or <library>_la_LDFLAGS when linking a library.''')
            for link in ulinks:
                print('  %s' % link)

        # Print reminders.
        print('')
        print('Don\'t forget to')
        if makefile_am == 'Makefile.am':
            print('  - add "%s/Makefile" to AC_CONFIG_FILES in %s,' %
                  (sourcebase, configure_ac))
        else:  # if makefile_am != 'Makefile.am'
            print('  - "include %s" from within "%s/Makefile.am",' %
                  (makefile, sourcebase))
        if pobase:
            print('  - add "%s/Makefile.in to AC_CONFIG_FILES in %s,' %
                  (pobase, configure_ac))
        if inctests:
            if makefile_am == 'Makefile.am':
                print('  - add "%s/Makefile" to AC_CONFIG_FILES in %s,' %
                      (testsbase, configure_ac))
            else:  # if makefile_am != 'Makefile.am'
                print('  - "include %s" from within "%s/Makefile.am",' %
                      (makefile, testsbase))
        # Print makefile edits.
        current_edit = int()
        makefile_am_edits = self.makefiletable.count()
        while current_edit != makefile_am_edits:
            dictionary = self.makefiletable[current_edit]
            if dictionary['var']:
                print('  - mention "%s" in %s in %s,' %
                      (dictionary['val'], dictionary['var'],
                       joinpath(dictionary['dir'], 'Makefile.am')))
            current_edit += 1

        # Detect position_early_after.
        with codecs.open(configure_ac, 'rb', 'UTF-8') as file:
            data = file.read()
        match_result1 = \
            bool(compiler('^ *AC_PROG_CC_STDC', re.S | re.M).findall(data))
        match_result2 = \
            bool(compiler('^ *AC_PROG_CC_C99', re.S | re.M).findall(data))
        if match_result1:
            position_early_after = 'AC_PROG_CC_STDC'
        elif match_result2:
            position_early_after = 'AC_PROG_CC_C99'
        else:  # if not any([match_result1, match_result2])
            position_early_after = 'AC_PROG_CC'
        print('  - invoke %s_EARLY in %s, right after %s,' %
              (macro_prefix, configure_ac, position_early_after))
        print('  - invoke %s_INIT in %s.' %
              (macro_prefix, configure_ac))
        sp.call(['rm', '-rf', self.config['tempdir']], shell=False)
