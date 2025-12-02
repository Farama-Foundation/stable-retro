#!/usr/bin/python
# encoding: UTF-8

#===============================================================================
# Define global imports
#===============================================================================
import os
import re
import sys
import copy
import codecs
import tempfile
import subprocess as sp
from . import constants
from .GLError import GLError


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
# Define GLConfig class
#===============================================================================
class GLConfig(object):
    '''This class is used to store intermediate settings for all pygnulib
    classes. It contains all necessary attributes to setup any other class.
    By default all attributes are set to empty string, empty list or zero.
    The most common value, however, is a None value.'''

    def __init__(self, destdir=None, localdir=None, auxdir=None,
                 sourcebase=None, m4base=None, pobase=None, docbase=None, testsbase=None,
                 modules=None, avoids=None, files=None, testflags=None, libname=None,
                 lgpl=None, makefile=None, libtool=None, conddeps=None, macro_prefix=None,
                 podomain=None, witness_c_macro=None, vc_files=None, symbolic=None,
                 lsymbolic=None, modcache=None, configure_ac=None, ac_version=None,
                 libtests=None, single_configure=None, verbose=None, dryrun=None,
                 errors=None):
        '''GLConfig.__init__(arguments) -> GLConfig

        Create new GLConfig instance.'''
        self.table = dict()
        self.table['tempdir'] = tempfile.mkdtemp()
        # destdir
        self.resetDestDir()
        if destdir != None:
            self.setDestDir(destdir)
        # localdir
        self.resetLocalDir()
        if localdir != None:
            self.setLocalDir(localdir)
        # auxdir
        self.resetAuxDir()
        if auxdir != None:
            self.setAuxDir(auxdir)
        # sourcebase
        self.resetSourceBase()
        if sourcebase != None:
            self.setSourceBase(sourcebase)
        # m4base
        self.resetM4Base()
        if m4base != None:
            self.setM4Base(m4base)
        # pobase
        self.resetPoBase()
        if pobase != None:
            self.setPoBase(pobase)
        # docbase
        self.resetDocBase()
        if docbase != None:
            self.setDocBase(docbase)
        # testsbase
        self.resetTestsBase()
        if testsbase != None:
            self.setTestsBase(testsbase)
        # modules
        self.resetModules()
        if modules != None:
            self.setModules(modules)
        # avoids
        self.resetAvoids()
        if avoids != None:
            self.setAvoids(avoids)
        # files
        self.resetFiles()
        if files != None:
            self.setFiles(files)
        # testflags
        self.resetTestFlags()
        if testflags != None:
            self.setTestFlags(testflags)
        # libname
        self.resetLibName()
        if libname != None:
            self.setLibName(libname)
        # lgpl
        self.resetLGPL()
        if lgpl != None:
            self.setLGPL(lgpl)
        # makefile
        self.resetMakefile()
        if makefile != None:
            self.setMakefile(makefile)
        # libtool
        self.resetLibtool()
        if libtool != None:
            if type(libtool) is bool:
                if not libtool:
                    self.disableLibtool()
                else:  # if libtool
                    self.enableLibtool()
            else:  # if type(libtool) is not bool
                raise(TypeError('libtool must be a bool, not %s' %
                                type(libtool).__name__))
        # conddeps
        self.resetCondDeps()
        if conddeps != None:
            if type(conddeps) is bool:
                if not conddeps:
                    self.disableCondDeps()
                else:  # if conddeps
                    self.enableCondDeps()
            else:  # if type(conddeps) is not bool
                raise(TypeError('conddeps must be a bool, not %s' %
                                type(conddeps).__name__))
        # macro_prefix
        self.resetMacroPrefix()
        if macro_prefix != None:
            self.setMacroPrefix(macro_prefix)
        # podomain
        self.resetPoDomain()
        if podomain != None:
            self.setPoDomain(podomain)
        # witness_c_macro
        self.resetWitnessCMacro()
        if witness_c_macro != None:
            if type(witness_c_macro) is bool:
                if not witness_c_macro:
                    self.setWitnessCMacro()
                else:  # if witness_c_macro
                    self.resetWitnessCMacro()
            else:  # if type(witness_c_macro) is not bool
                raise(TypeError('witness_c_macro must be a bool, not %s' %
                                type(witness_c_macro).__name__))
        # vc_files
        self.resetVCFiles()
        if vc_files != None:
            if type(vc_files) is bool:
                if not vc_files:
                    self.disableVCFiles()
                else:  # if vc_files
                    self.enableVCFiles()
            else:  # if type(vc_files) is not bool
                raise(TypeError('vc_files must be a bool, not %s' %
                                type(vc_files).__name__))
        # symbolic
        self.resetSymbolic()
        if symbolic != None:
            if type(symbolic) is bool:
                if not symbolic:
                    self.disableSymbolic()
                else:  # if symbolic
                    self.enableSymbolic()
            else:  # if type(symbolic) is not bool
                raise(TypeError('symbolic must be a bool, not %s' %
                                type(symbolic).__name__))
        # lsymbolic
        self.resetLSymbolic()
        if lsymbolic != None:
            if type(lsymbolic) is bool:
                if not lsymbolic:
                    self.disableLSymbolic()
                else:  # if lsymbolic
                    self.enableLSymbolic()
            else:  # if type(lsymbolic) is not bool
                raise(TypeError('lsymbolic must be a bool, not %s' %
                                type(lsymbolic).__name__))
        # modcache
        self.resetModuleCaching()
        if modcache != None:
            if type(modcache) is bool:
                if not modcache:
                    self.disableModuleCaching()
                else:  # if modcache
                    self.enableModuleCaching()
            else:  # if type(modcache) is not bool
                raise(TypeError('modcache must be a bool, not %s' %
                                type(modcache).__name__))
        # configure_ac
        self.resetAutoconfFile()
        if configure_ac != None:
            self.setAutoconfFile(configure_ac)
        # ac_version
        self.resetAutoconfVersion()
        if ac_version != None:
            self.setAutoconfVersion(ac_version)
        # libtests
        self.resetLibtests()
        if libtests != None:
            if type(libtests) is bool:
                if not libtests:
                    self.disableLibtests()
                else:  # if libtests
                    self.enableLibtests()
            else:  # if type(libtests) is not bool
                raise(TypeError('libtests must be a bool, not %s' %
                                type(libtests).__name__))
        # single_configure
        self.resetSingleConfigure()
        if single_configure != None:
            if type(single_configure) is bool:
                if not single_configure:
                    self.disableSingleConfigure()
                else:  # if single_configure
                    self.enableSingleConfigure()
            else:  # if type(single_configure) is not bool
                raise(TypeError('single_configure must be a bool, not %s' %
                                type(single_configure).__name__))
        # verbose
        self.resetVerbosity()
        if verbose != None:
            self.setVerbosity(verbose)
        # dryrun
        self.resetDryRun()
        if dryrun != None:
            if type(dryrun) is bool:
                if not dryrun:
                    self.disableDryRun()
                else:  # if dryrun
                    self.enableDryRun()
            else:  # if type(dryrun) is not bool
                raise(TypeError('dryrun must be a bool, not %s' %
                                type(dryrun).__name__))
        # errors
        self.resetErrors()
        if errors != None:
            if type(errors) is bool:
                if not errors:
                    self.disableErrors()
                else:  # if errors
                    self.enableErrors()
            else:  # if type(errors) is not bool
                raise(TypeError('errors must be a bool, not %s' %
                                type(errors).__name__))

    # Define special methods.
    def __repr__(self):
        '''x.__repr__() <==> repr(x)'''
        return('<pygnulib.GLConfig>')

    def __getitem__(self, y):
        '''x.__getitem__(y) <==> x[y]'''
        if y in self.table:
            result = self.table[y]
            if type(y) is list:
                result = list(self.table[y])
            if y == "auxdir":
                if self.table['auxdir']:
                    return self.table['auxdir']
                return "build-aux"
            return(self.table[y])
        else:  # if y not in self.table
            raise(KeyError('GLConfig does not contain key: %s' % repr(y)))

    def dictionary(self):
        '''Return the configuration as a dict object.'''
        return(dict(self.table))

    def copy(self):
        '''Return the copy of the configuration.'''
        table = copy.deepcopy(self)
        return(table)

    def update(self, dictionary):
        '''Specify the dictionary whose keys will be used to update config.'''
        if type(dictionary) is not GLConfig:
            raise(TypeError('dictionary must be a GLConfig, not %s' %
                            type(dictionary).__name__))
        dictionary = dict(dictionary.table)
        result = dict()
        for key in dictionary:
            src = self.table[key]
            dest = dictionary[key]
            result[key] = src
            if src != dest:
                if self.isdefault(key, src):
                    result[key] = dest
                else:  # if not self.isdefault(key, src)
                    result[key] == src
                    if not self.isdefault(key, dest):
                        if key in ['modules', 'avoids', 'tests']:
                            dest = sorted(set(src + dest))
                        result[key] = dest
        self.table = dict(result)

    def update_key(self, dictionary, key):
        '''Update the given key using value from the given dictionary.'''
        if key in self.table:
            if type(dictionary) is not GLConfig:
                raise(TypeError('dictionary must be a GLConfig, not %s' %
                                type(dictionary).__name__))
            dictionary = dict(dictionary.table)
            self.table[key] = dictionary[key]
        else:  # if key not in self.table
            raise(KeyError('GLConfig does not contain key: %s' % repr(key)))

    def default(self, key):
        '''Return default value for the given key.'''
        if key in self.table:
            if key == 'libname':
                return(string('libgnu'))
            elif key == 'macro_prefix':
                return(string('gl'))
            elif key == 'include_guard_prefix':
                return(string('GL'))
            elif key == 'ac_version':
                return(2.59)
            elif key == 'verbosity':
                return(0)
            elif key == 'copyrights':
                return(True)
            elif key in ['modules', 'avoids', 'tests', 'testflags']:
                return(list())
            elif key in ['libtool', 'lgpl', 'conddeps', 'modcache', 'symbolic',
                         'lsymbolic', 'libtests', 'dryrun']:
                return(False)
            if key == 'vc_files':
                return(None)
            elif key == 'errors':
                return(True)
            else:  # otherwise
                return(string())
        else:  # if key not in self.table
            raise(KeyError('GLConfig does not contain key: %s' % repr(key)))

    def isdefault(self, key, value):
        '''Check whether the value for the given key is a default value.'''
        if key in self.table:
            default = self.default(key)
            return(value == default)
        else:  # if key not in self.table
            raise(KeyError('GLConfig does not contain key: %s' % repr(key)))

    def keys(self):
        '''Return list of keys.'''
        return(list(self.table.keys()))

    def values(self):
        '''Return list of values.'''
        return(list(self.table.values()))

    # Define destdir methods.
    def getDestDir(self):
        '''Return the target directory. For --import, this specifies where your
        configure.ac can be found. Defaults to current directory.'''
        return(self.table['destdir'])

    def setDestDir(self, destdir):
        '''Specify the target directory. For --import, this specifies where your
        configure.ac can be found. Defaults to current directory.'''
        if type(destdir) is bytes or type(destdir) is string:
            if type(destdir) is bytes:
                destdir = string(destdir, ENCS['system'])
            if destdir:
                self.table['destdir'] = os.path.normpath(destdir)
        else:  # if destdir has not bytes/string type
            raise(TypeError('destdir must be a string, not %s' %
                            type(destdir).__name__))

    def resetDestDir(self):
        '''Reset the target directory. For --import, this specifies where your
        configure.ac can be found. Defaults to current directory.'''
        self.table['destdir'] = string()

    # Define localdir methods.
    def getLocalDir(self):
        '''Return a local override directory where to look up files before looking
        in gnulib's directory.'''
        return(self.table['localdir'])

    def setLocalDir(self, localdir):
        '''Specify a local override directory where to look up files before looking
        in gnulib's directory.'''
        if type(localdir) is bytes or type(localdir) is string:
            if type(localdir) is bytes:
                localdir = string(localdir, ENCS['system'])
            if localdir:
                self.table['localdir'] = localdir
        else:  # if localdir has not bytes/string type
            raise(TypeError('localdir must be a string, not %s' %
                            type(localdir).__name__))

    def resetLocalDir(self):
        '''Reset a local override directory where to look up files before looking
        in gnulib's directory.'''
        self.table['localdir'] = string()

    # Define auxdir methods.
    def getAuxDir(self):
        '''Return directory relative to --dir where auxiliary build tools are
        placed. Default comes from configure.ac or configure.in.'''
        if self.table['auxdir']:
            return self.table['auxdir']
        return "build-aux"

    def setAuxDir(self, auxdir):
        '''Specify directory relative to --dir where auxiliary build tools are
        placed. Default comes from configure.ac or configure.in.'''
        if type(auxdir) is bytes or type(auxdir) is string:
            if type(auxdir) is bytes:
                auxdir = string(auxdir, ENCS['system'])
            if auxdir:
                self.table['auxdir'] = auxdir
        else:  # if type of auxdir is not bytes or string
            raise(TypeError('auxdir must be a string, not %s' %
                            type(auxdir).__name__))

    def resetAuxDir(self):
        '''Reset directory relative to --dir where auxiliary build tools are
        placed. Default comes from configure.ac or configure.in.'''
        self.table['auxdir'] = string()

    # Define sourcebase methods.
    def getSourceBase(self):
        '''Return directory relative to destdir where source code is placed.'''
        return(self.table['sourcebase'])

    def setSourceBase(self, sourcebase):
        '''Specify directory relative to destdir where source code is placed.'''
        if type(sourcebase) is bytes or type(sourcebase) is string:
            if type(sourcebase) is bytes:
                sourcebase = string(sourcebase, ENCS['system'])
            if sourcebase:
                self.table['sourcebase'] = sourcebase
        else:  # if type of sourcebase is not bytes or string
            raise(TypeError('sourcebase must be a string, not %s' %
                            type(sourcebase).__name__))

    def resetSourceBase(self):
        '''Return directory relative to destdir where source code is placed.'''
        self.table['sourcebase'] = string()

    # Define m4base methods.
    def getM4Base(self):
        '''Return directory relative to destdir where *.m4 macros are placed.'''
        return(self.table['m4base'])

    def setM4Base(self, m4base):
        '''Specify directory relative to destdir where *.m4 macros are placed.'''
        if type(m4base) is bytes or type(m4base) is string:
            if type(m4base) is bytes:
                m4base = string(m4base, ENCS['system'])
            if m4base:
                self.table['m4base'] = m4base
        else:  # if type of m4base is not bytes or string
            raise(TypeError('m4base must be a string, not %s' %
                            type(m4base).__name__))

    def resetM4Base(self):
        '''Reset directory relative to destdir where *.m4 macros are placed.'''
        self.table['m4base'] = string()

    # Define pobase methods.
    def getPoBase(self):
        '''Return directory relative to destdir where *.po files are placed.'''
        return(self.table['pobase'])

    def setPoBase(self, pobase):
        '''Specify directory relative to destdir where *.po files are placed.'''
        if type(pobase) is bytes or type(pobase) is string:
            if type(pobase) is bytes:
                pobase = string(pobase, ENCS['system'])
            if pobase:
                self.table['pobase'] = pobase
        else:  # if type of pobase is not bytes or string
            raise(TypeError('pobase must be a string, not %s' %
                            type(pobase).__name__))

    def resetPoBase(self):
        '''Reset directory relative to destdir where *.po files are placed.'''
        self.table['pobase'] = string()

    # Define docbase methods.
    def getDocBase(self):
        '''Return directory relative to destdir where doc files are placed.
        Default value for this variable is 'doc').'''
        return(self.table['docbase'])

    def setDocBase(self, docbase):
        '''Specify directory relative to destdir where doc files are placed.
        Default value for this variable is 'doc').'''
        if type(docbase) is bytes or type(docbase) is string:
            if type(docbase) is bytes:
                docbase = string(docbase, ENCS['system'])
            if docbase:
                self.table['docbase'] = docbase
        else:  # if type of docbase is not bytes or string
            raise(TypeError('docbase must be a string, not %s' %
                            type(docbase).__name__))

    def resetDocBase(self):
        '''Reset directory relative to destdir where doc files are placed.
        Default value for this variable is 'doc').'''
        self.table['docbase'] = string()

    # Define testsbase methods.
    def getTestsBase(self):
        '''Return directory relative to destdir where unit tests are placed.
        Default value for this variable is 'tests').'''
        return(self.table['testsbase'])

    def setTestsBase(self, testsbase):
        '''Specify directory relative to destdir where unit tests are placed.
        Default value for this variable is 'tests').'''
        if type(testsbase) is bytes or type(testsbase) is string:
            if type(testsbase) is bytes:
                testsbase = string(testsbase, ENCS['system'])
            if testsbase:
                self.table['testsbase'] = testsbase
        else:  # if type of testsbase is not bytes or string
            raise(TypeError('testsbase must be a string, not %s' %
                            type(testsbase).__name__))

    def resetTestsBase(self):
        '''Reset directory relative to destdir where unit tests are placed.
        Default value for this variable is 'tests').'''
        self.table['testsbase'] = string()

    # Define modules methods.
    def addModule(self, module):
        '''Add the module to the modules list.'''
        if type(module) is bytes or type(module) is string:
            if type(module) is bytes:
                module = module.decode(ENCS['default'])
            if module not in self.table['modules']:
                self.table['modules'] += [module]
        else:  # if module has not bytes or string type
            raise(TypeError('module must be a string, not %s' %
                            type(module).__name__))

    def removeModule(self, module):
        '''Remove the module from the modules list.'''
        if type(module) is bytes or type(module) is string:
            if type(module) is bytes:
                module = module.decode(ENCS['default'])
            if module in self.table['modules']:
                self.table['modules'].remove(module)
        else:  # if module has not bytes or string type
            raise(TypeError('module must be a string, not %s' %
                            type(module).__name__))

    def getModules(self):
        '''Return the modules list.'''
        return(list(self.table['modules']))

    def setModules(self, modules):
        '''Set the modules list.'''
        if type(modules) is list or type(modules) is tuple:
            old_modules = self.table['modules']
            self.table['modules'] = list()
            for module in modules:
                try:  # Try to add each module
                    self.addModule(module)
                except TypeError as error:
                    self.table['modules'] = old_modules
                    raise(TypeError('each module must be a string'))
                except GLError as error:
                    self.table['modules'] = old_modules
                    raise(GLError(error.errno, error.errinfo))
        else:  # if type of modules is not list or tuple
            raise(TypeError('modules must be a list or a tuple, not %s' %
                            type(modules).__name__))

    def resetModules(self):
        '''Reset the list of the modules.'''
        self.table['modules'] = list()

    # Define avoids methods.
    def addAvoid(self, module):
        '''Avoid including the given module. Useful if you have code that provides
        equivalent functionality.'''
        if type(module) is bytes or type(module) is string:
            if type(module) is bytes:
                module = module.decode(ENCS['default'])
            if module not in self.table['avoids']:
                self.table['avoids'].append(module)
        else:  # if module has not bytes or string type
            raise(TypeError('avoid must be a string, not %s' %
                            type(module).__name__))

    def removeAvoid(self, module):
        '''Remove the given module from the list of avoided modules.'''
        if type(module) is bytes or type(module) is string:
            if type(module) is bytes:
                module = module.decode(ENCS['default'])
            if module in self.table['avoids']:
                self.table['avoids'].remove(module)
        else:  # if module has not bytes or string type
            raise(TypeError('avoid must be a string, not %s' %
                            type(module).__name__))

    def getAvoids(self):
        '''Return the list of the avoided modules.'''
        return(list(self.table['avoids']))

    def setAvoids(self, modules):
        '''Specify the modules which will be avoided.'''
        if type(modules) is list or type(modules) is tuple:
            old_avoids = self.table['avoids']
            self.table['avoids'] = list()
            for module in modules:
                try:  # Try to add each module
                    self.addAvoid(module)
                except TypeError as error:
                    self.table['avoids'] = old_avoids
                    raise(TypeError('each module must be a string'))
                except GLError as error:
                    self.table['avoids'] = old_avoids
                    raise(GLError(error.errno, error.errinfo))
        else:  # if type of modules is not list or tuple
            raise(TypeError('modules must be a list or a tuple, not %s' %
                            type(modules).__name__))

    def resetAvoids(self):
        '''Reset the list of the avoided modules.'''
        self.table['avoids'] = list()

    # Define files methods.
    def addFile(self, file):
        '''Add file to the list of files.'''
        if type(file) is bytes or type(file) is string:
            if type(file) is bytes:
                file = file.decode(ENCS['default'])
            if file not in self.table['files']:
                self.table['files'].append(file)
        else:  # if file has not bytes or string type
            raise(TypeError('file must be a string, not %s' %
                            type(file).__name__))

    def removeFile(self, file):
        '''Remove the given file from the list of files.'''
        if type(file) is bytes or type(file) is string:
            if type(file) is bytes:
                file = file.decode(ENCS['default'])
            if file in self.table['files']:
                self.table['files'].remove(file)
        else:  # if file has not bytes or string type
            raise(TypeError('file must be a string, not %s' %
                            type(file).__name__))

    def getFiles(self):
        '''Return the list of the fileed files.'''
        return(list(self.table['files']))

    def setFiles(self, files):
        '''Specify the list of files.'''
        if type(files) is list or type(files) is tuple:
            old_files = self.table['files']
            self.table['files'] = list()
            for file in files:
                try:  # Try to add each file
                    self.addFile(file)
                except TypeError as error:
                    self.table['files'] = old_files
                    raise(TypeError('each file must be a string'))
                except GLError as error:
                    self.table['files'] = old_files
                    raise(GLError(error.errno, error.errinfo))
        else:  # if type of files is not list or tuple
            raise(TypeError('files must be a list or a tuple, not %s' %
                            type(files).__name__))

    def resetFiles(self):
        '''Reset the list of files.'''
        self.table['files'] = list()

    # Define tests/testflags methods
    def checkTestFlag(self, flag):
        '''Return the status of the test flag.'''
        if flag in TESTS.values():
            return(flag in self.table['testflags'])
        else:  # if flag is not in TESTS
            raise(TypeError('unknown flag: %s' % repr(flag)))

    def enableTestFlag(self, flag):
        '''Enable test flag. You can get flags from TESTS variable.'''
        if flag in TESTS.values():
            if flag not in self.table['testflags']:
                self.table['testflags'].append(flag)
        else:  # if flag is not in TESTS
            raise(TypeError('unknown flag: %s' % repr(flag)))

    def disableTestFlag(self, flag):
        '''Disable test flag. You can get flags from TESTS variable.'''
        if flag in TESTS.values():
            if flag in self.table['testflags']:
                self.table['testflags'].remove(flag)
        else:  # if flag is not in TESTS
            raise(TypeError('unknown flag: %s' % repr(flag)))

    def getTestFlags(self):
        '''Return test flags. You can get flags from TESTS variable.'''
        return(list(self.table['testflags']))

    def setTestFlags(self, testflags):
        '''Specify test flags. You can get flags from TESTS variable.'''
        if type(testflags) is list or type(testflags) is tuple:
            old_testflags = self.table['testflags']
            self.table['testflags'] = list()
            for flag in testflags:
                try:  # Try to enable each flag
                    self.enableTestFlag(flag)
                except TypeError as error:
                    raise(TypeError('each flag must be one of TESTS integers'))
            self.table['testflags'] = testflags
        else:  # if type of testflags is not list or tuple
            raise(TypeError('testflags must be a list or a tuple, not %s' %
                            type(testflags).__name__))

    def resetTestFlags(self):
        '''Reset test flags (only default flag will be enabled).'''
        self.table['testflags'] = list()
        self.table['tests'] = self.table['testflags']

    # Define libname methods.
    def getLibName(self):
        '''Return the library name.'''
        return(self.table['libname'])

    def setLibName(self, libname):
        '''Specify the library name.'''
        if type(libname) is bytes or type(libname) is string:
            if type(libname) is bytes:
                libname = string(libname, ENCS['system'])
            if libname:
                self.table['libname'] = libname
        else:  # if type of libname is not bytes or string
            raise(TypeError('libname must be a string, not %s' %
                            type(module).__name__))

    def resetLibName(self):
        '''Reset the library name to 'libgnu'.'''
        self.table['libname'] = string('libgnu')

    # Define libtool methods.
    def checkLibtool(self):
        '''Check if user enabled libtool rules.'''
        return(self.table['libtool'])

    def enableLibtool(self):
        '''Enable libtool rules.'''
        self.table['libtool'] = True

    def disableLibtool(self):
        '''Disable libtool rules.'''
        self.table['libtool'] = False

    def resetLibtool(self):
        '''Reset libtool rules.'''
        self.table['libtool'] = False

    # Define conddeps methods.
    def checkCondDeps(self):
        '''Check if user enabled cond. dependencies.'''
        return(self.table['conddeps'])

    def enableCondDeps(self):
        '''Enable cond. dependencies (may save configure time and object code).'''
        self.table['conddeps'] = True

    def disableCondDeps(self):
        '''Disable cond. dependencies (may save configure time and object code).'''
        self.table['conddeps'] = False

    def resetCondDeps(self):
        '''Reset cond. dependencies (may save configure time and object code).'''
        self.table['conddeps'] = False

    # Define lgpl methods.
    def getLGPL(self):
        '''Check for abort if modules aren't available under the LGPL.
        Default value is False, which means that lgpl is disabled.'''
        return(self.table['lgpl'])

    def setLGPL(self, lgpl):
        '''Abort if modules aren't available under the LGPL.
        Default value is False, which means that lgpl is disabled.'''
        if (type(lgpl) is int and 2 <= lgpl <= 3) or type(lgpl) is bool:
            self.table['lgpl'] = lgpl
        else:  # if lgpl is not False, 2 or 3
            raise(TypeError('invalid LGPL version: %s' % repr(lgpl)))

    def resetLGPL(self):
        '''Disable abort if modules aren't available under the LGPL.
        Default value is False, which means that lgpl is disabled.'''
        self.table['lgpl'] = False

    # Define macro_prefix methods.
    def getIncludeGuardPrefix(self):
        '''Return include_guard_prefix to use inside GLEmiter class.'''
        return(self.table['include_guard_prefix'])

    def getMacroPrefix(self):
        '''Return the prefix of the macros 'gl_EARLY' and 'gl_INIT'.
        Default macro_prefix is 'gl'.'''
        return(self.table['macro_prefix'])

    def setMacroPrefix(self, macro_prefix):
        '''Specify the prefix of the macros 'gl_EARLY' and 'gl_INIT'.
        Default macro_prefix is 'gl'.'''
        if type(macro_prefix) is bytes or type(macro_prefix) is string:
            if type(macro_prefix) is bytes:
                macro_prefix = string(macro_prefix, ENCS['system'])
            if macro_prefix:
                self.table['macro_prefix'] = macro_prefix
        else:  # if type of macro_prefix is not bytes or string
            raise(TypeError('macro_prefix must be a string, not %s' %
                            type(macro_prefix).__name__))
        if macro_prefix == 'gl':
            include_guard_prefix = 'GL'
        else:  # macro_prefix != 'gl'
            include_guard_prefix = 'GL_%s' % macro_prefix.upper()
        if type(include_guard_prefix) is bytes:
            include_guard_prefix = include_guard_prefix.decode(ENCS['default'])
        self.table['include_guard_prefix'] = include_guard_prefix

    def resetMacroPrefix(self):
        '''Reset the prefix of the macros 'gl_EARLY' and 'gl_INIT'.
        Default macro_prefix is 'gl'.'''
        self.table['macro_prefix'] = string('gl')
        include_guard_prefix = string('GL')
        if type(include_guard_prefix) is bytes:
            include_guard_prefix = include_guard_prefix.decode(ENCS['default'])
        self.table['include_guard_prefix'] = include_guard_prefix

    # Define makefile methods.
    def getMakefile(self):
        '''Return the name of makefile in automake syntax in the source-base and
        tests-base directories. Default is 'Makefile.am'.'''
        return(self.table['makefile'])

    def setMakefile(self, makefile):
        '''Specify the name of makefile in automake syntax in the source-base and
        tests-base directories. Default is 'Makefile.am'.'''
        if type(makefile) is bytes or type(makefile) is string:
            if type(makefile) is bytes:
                makefile = string(makefile, ENCS['system'])
            if makefile:
                self.table['makefile'] = makefile
        else:  # if type of makefile is not bytes or string
            raise(TypeError('makefile must be a string, not %s' %
                            type(makefile).__name__))

    def resetMakefile(self):
        '''Reset the name of makefile in automake syntax in the source-base and
        tests-base directories. Default is 'Makefile.am'.'''
        self.table['makefile'] = string()

    # Define podomain methods.
    def getPoDomain(self):
        '''Return the prefix of the i18n domain. Usually use the package name.
        A suffix '-gnulib' is appended.'''
        return(self.table['podomain'])

    def setPoDomain(self, podomain):
        '''Specify the prefix of the i18n domain. Usually use the package name.
        A suffix '-gnulib' is appended.'''
        if type(podomain) is bytes or type(podomain) is string:
            if type(podomain) is bytes:
                podomain = string(podomain, ENCS['system'])
            if podomain:
                self.table['podomain'] = podomain
        else:  # if type of podomain is not bytes or string
            raise(TypeError('podomain must be a string, not %s' %
                            type(podomain).__name__))

    def resetPoDomain(self):
        '''Reset the prefix of the i18n domain. Usually use the package name.
        A suffix '-gnulib' is appended.'''
        self.table['podomain'] = string()

    # Define witness_c_macro methods.
    def getWitnessCMacro(self):
        '''Return the C macro that is defined when the sources in this directory
        are compiled or used.'''
        return(self.table['witness_c_macro'])

    def setWitnessCMacro(self, witness_c_macro):
        '''Specify the C macro that is defined when the sources in this directory
        are compiled or used.'''
        if type(witness_c_macro) is bytes or type(witness_c_macro) is string:
            if type(witness_c_macro) is bytes:
                witness_c_macro = string(witness_c_macro, ENCS['system'])
            if witness_c_macro:
                self.table['witness_c_macro'] = witness_c_macro
        else:  # if type of witness_c_macro is not bytes or string
            raise(TypeError('witness_c_macro must be a string, not %s' %
                            type(witness_c_macro).__name__))

    def resetWitnessCMacro(self):
        '''Return the C macro that is defined when the sources in this directory
        are compiled or used.'''
        self.table['witness_c_macro'] = string()

    # Define vc_files methods.
    def checkVCFiles(self):
        '''Check if update of the version control files is enabled or disabled.'''
        return(self.table['vc_files'])

    def enableVCFiles(self):
        '''Enable update of the version control files.'''
        self.table['vc_files'] = True

    def disableVCFiles(self):
        '''Disable update of the version control files.'''
        self.table['vc_files'] = False

    def resetVCFiles(self):
        '''Reset update of the version control files and set it to None.'''
        self.table['vc_files'] = None

    # Define modcache methods.
    def checkModuleCaching(self):
        '''Get status of module caching optimization.'''
        return(self.table['modcache'])

    def enableModuleCaching(self):
        '''Enable module caching optimization.'''
        self.table['modcache'] = True

    def disableModuleCaching(self):
        '''Disable module caching optimization.'''
        self.table['modcache'] = False

    def resetModuleCaching(self):
        '''Reset module caching optimization.'''
        self.table['modcache'] = False

    # Define configure_ac methods.
    def getAutoconfFile(self):
        '''Return path of autoconf file relative to destdir.'''
        return(self.table['configure_ac'])

    def setAutoconfFile(self, configure_ac):
        '''Specify path of autoconf file relative to destdir.'''
        if type(configure_ac) is bytes or type(configure_ac) is string:
            if type(configure_ac) is bytes:
                configure_ac = string(configure_ac, ENCS['system'])
            if configure_ac:
                self.table['configure_ac'] = \
                    relpath(self.table['destdir'], configure_ac)
        else:  # if type of configure_ac is not bytes or string
            raise(TypeError('configure_ac must be a string, not %s' %
                            type(configure_ac).__name__))

    def resetAutoconfFile(self):
        '''Reset path of autoconf file relative to destdir.'''
        configure_ac = string()
        if isfile(joinpath(self.table['destdir'], 'configure.ac')):
            configure_ac = joinpath(self.table['destdir'], 'configure.ac')
        elif isfile(joinpath(self.table['destdir'], 'configure.in')):
            configure_ac = joinpath(self.table['destdir'], 'configure.in')
        self.table['configure_ac'] = configure_ac

    # Define ac_version methods.
    def getAutoconfVersion(self):
        '''Return preferred autoconf version. Default value is 2.59.'''
        return(self.table['ac_version'])

    def setAutoconfVersion(self, ac_version):
        '''Specify preferred autoconf version. Default value is 2.59.'''
        if type(ac_version) is float or type(ac_version) is int:
            self.table['ac_version'] = float(ac_version)
        else:  # if ac_version has not int or float type
            raise(TypeError('ac_version must be an int or a float, not %s' %
                            type(ac_version).__name__))

    def resetAutoconfVersion(self):
        '''Specify preferred autoconf version. Default value is 2.59.'''
        self.table['ac_version'] = 2.59

    # Define symbolic methods.
    def checkCopyrights(self):
        '''Check if copyright notices in files should be replaced.'''
        return(self.table['copyrights'])

    def checkSymbolic(self):
        '''Check if pygnulib will make symbolic links instead of copying files.'''
        return(self.table['symbolic'])

    def enableSymbolic(self):
        '''Enable creation of the symbolic links instead of copying files.'''
        self.table['symbolic'] = True
        self.table['copyrights'] = False

    def disableSymbolic(self):
        '''Enable creation of the symbolic links instead of copying files.'''
        self.table['symbolic'] = False
        self.table['copyrights'] = True

    def resetSymbolic(self):
        '''Reset creation of the symbolic links instead of copying files.'''
        self.table['symbolic'] = False
        self.table['copyrights'] = True

    # Define lsymbolic methods.
    def checkLSymbolic(self):
        '''Check if pygnulib will make symbolic links instead of copying files, only
        for files from the local override directory.'''
        return(self.table['lsymbolic'])

    def enableLSymbolic(self):
        '''Enable creation of symbolic links instead of copying files, only for
        files from the local override directory.'''
        self.table['lsymbolic'] = True

    def disableLSymbolic(self):
        '''Disable creation of symbolic links instead of copying files, only for
        files from the local override directory.'''
        self.table['lsymbolic'] = False

    def resetLSymbolic(self):
        '''Reset creation of symbolic links instead of copying files, only for
        files from the local override directory.'''
        self.table['lsymbolic'] = False

    # Define verbosity methods.
    def getVerbosity(self):
        '''Get verbosity level.'''
        return(self.table['verbosity'])

    def decreaseVerbosity(self):
        '''Decrease verbosity level.'''
        if self.table['verbosity'] > MODES['verbose-min']:
            self.table['verbosity'] -= 1

    def increaseVerbosity(self):
        '''Increase verbosity level.'''
        if self.table['verbosity'] < MODES['verbose-max']:
            self.table['verbosity'] += 1

    def setVerbosity(self, verbose):
        '''Set verbosity level to verbose, where -2 <= verbose <= 2.
        If verbosity level is less than -2, verbosity level will be set to -2.
        If verbosity level is greater than 2, verbosity level will be set to 2.'''
        if type(verbose) is int:
            if MODES['verbose-min'] <= verbose <= MODES['verbose-max']:
                self.table['verbosity'] = verbose
            elif verbose < MODES['verbose-min']:
                self.table['verbosity'] = MODES['verbose-min']
            elif verbose > MODES['verbose-max']:
                self.table['verbosity'] = MODES['verbose-max']
        else:  # if type(verbose) is not int
            raise(TypeError('verbosity must be an int, not %s' %
                            type(verbose).__name__))

    def resetVerbosity(self):
        '''Reset verbosity level.'''
        self.table['verbosity'] = 0

    # Define libtests methods.
    def checkLibtests(self):
        '''Return True if a testsbase/libtests.a is needed.'''
        return(self.table['libtests'])

    def enableLibtests(self):
        '''If libtests is enabled, then testsbase/libtests.a is needed.'''
        self.table['libtests'] = True

    def disableLibtests(self):
        '''If libtests is disabled, then testsbase/libtests.a is not needed.'''
        self.table['libtests'] = False

    def resetLibtests(self):
        '''Reset status of testsbase/libtests.a.'''
        self.table['libtests'] = False

    # Define single_configure methods.
    def checkSingleConfigure(self):
        '''Check whether single configure file should be generated.'''
        return(self.table['single_configure'])

    def enableSingleConfigure(self):
        '''Enable generation of the single configure file.'''
        self.table['single_configure'] = True

    def disableSingleConfigure(self):
        '''Disable generation of the single configure file.'''
        self.table['single_configure'] = False

    def resetSingleConfigure(self):
        '''Reset status of the single configure file generation.'''
        self.table['single_configure'] = False

    # Define dryrun methods.
    def checkDryRun(self):
        '''Check whether dryrun is enabled.'''
        return(self.table['dryrun'])

    def enableDryRun(self):
        '''Enable dryrun mode.'''
        self.table['dryrun'] = True

    def disableDryRun(self):
        '''Disable dryrun mode.'''
        self.table['dryrun'] = False

    def resetDryRun(self):
        '''Reset status of dryrun mode.'''
        self.table['dryrun'] = False

    # Define errors methods.
    def checkErrors(self):
        '''Check if GLError will be raised in non-critical situations.'''
        return(self.table['errors'])

    def enableErrors(self):
        '''Raise GLError in non-critical situations.'''
        self.table['errors'] = True

    def disableErrors(self):
        '''Do not raise GLError in non-critical situations.'''
        self.table['errors'] = False

    def resetErrors(self):
        '''Reset status of raising GLError in non-critical situations.'''
        self.table['errors'] = False
