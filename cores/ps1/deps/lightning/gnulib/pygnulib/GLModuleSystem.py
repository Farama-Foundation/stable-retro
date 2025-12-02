#!/usr/bin/python
# encoding: UTF-8

#===============================================================================
# Define global imports
#===============================================================================
import os
import re
import sys
import codecs
import hashlib
import subprocess as sp
from . import constants
from .GLError import GLError
from .GLConfig import GLConfig
from .GLFileSystem import GLFileSystem


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
string = constants.string
isabs = os.path.isabs
isdir = os.path.isdir
isfile = os.path.isfile
normpath = os.path.normpath
relpath = os.path.relpath
filter_filelist = constants.filter_filelist


#===============================================================================
# Define GLModuleSystem class
#===============================================================================
class GLModuleSystem(object):
    '''GLModuleSystem is used to operate with module system using dynamic
    searching and patching.'''

    def __init__(self, config):
        '''GLModuleSystem.__init__(config) -> GLModuleSystem

        Create new GLModuleSystem instance. Some functions use GLFileSystem class
        to look up a file in localdir or gnulib directories, or combine it through
        'patch' utility.'''
        self.args = dict()
        if type(config) is not GLConfig:
            raise(TypeError('config must be a GLConfig, not %s' %
                            type(config).__name__))
        self.config = config
        self.filesystem = GLFileSystem(self.config)

    def __repr__(self):
        '''x.__repr__ <==> repr(x)'''
        result = '<pygnulib.GLModuleSystem %s>' % hex(id(self))
        return(result)

    def exists(self, module):
        '''GLModuleSystem.exists(module) -> bool

        Check whether the given module exists.
        GLConfig: localdir.'''
        if type(module) is bytes or string:
            if type(module) is bytes:
                module = module.decode(ENCS['default'])
        else:  # if module has not bytes or string type
            raise(TypeError(
                'module must be a string, not %s' % type(module).__name__))
        result = bool()
        badnames = ['ChangeLog', 'COPYING', 'README', 'TEMPLATE',
                    'TEMPLATE-EXTENDED', 'TEMPLATE-TESTS']
        if isfile(joinpath(DIRS['modules'], module)) or \
                all([  # Begin all(iterable) function
                    self.config['localdir'],
                    isdir(joinpath(self.config['localdir'], 'modules')),
                    isfile(
                        joinpath(self.config['localdir'], 'modules', module))
                ]):  # Close all(iterable) function
            if module not in badnames:
                result = True
        return(result)

    def find(self, module):
        '''GLModuleSystem.find(module) -> GLModule

        Find the given module.'''
        if type(module) is bytes or string:
            if type(module) is bytes:
                module = module.decode(ENCS['default'])
        else:  # if module has not bytes or string type
            raise(TypeError(
                'module must be a string, not %s' % type(module).__name__))
        if self.exists(module):
            path, istemp = self.filesystem.lookup(joinpath('modules', module))
            result = GLModule(self.config, path, istemp)
            return(result)
        else:  # if not self.exists(module)
            if self.config['errors']:
                raise(GLError(3, module))
            else:  # if not self.config['errors']
                sys.stderr.write('gnulib-tool: warning: ')
                sys.stderr.write('file %s does not exist\n' % str(module))

    def list(self):
        '''GLModuleSystem.list() -> list

        Return the available module names as tuple. We could use a combination
        of os.walk() function and re module. However, it takes too much time to
        complete, so this version uses subprocess to run shell commands.'''
        result = string()
        listing = list()
        localdir = self.config['localdir']
        find_args = ['find', 'modules', '-type', 'f', '-print']
        sed_args = \
            [
                'sed',
                '-e', r's,^modules/,,',
                '-e', r'/^ChangeLog$/d',
                '-e', r'/\/ChangeLog$/d',
                '-e', r'/^COPYING$/d',
                '-e', r'/\/COPYING$/d',
                '-e', r'/^README$/d',
                '-e', r'/\/README$/d',
                '-e', r'/^TEMPLATE$/d',
                '-e', r'/^TEMPLATE-EXTENDED$/d',
                '-e', r'/^TEMPLATE-TESTS$/d',
                '-e', r'/^\..*/d',
                '-e', r'/~$/d',
                '-e', r'/-tests$/d',
            ]

        # Read modules from gnulib root directory.
        os.chdir(constants.DIRS['root'])
        find = sp.Popen(find_args, stdout=sp.PIPE)
        result += find.stdout.read().decode("UTF-8")

        # Read modules from local directory.
        if localdir and isdir(joinpath(localdir, 'modules')):
            os.chdir(localdir)
            find = sp.Popen(find_args, stdout=sp.PIPE)
            result += find.stdout.read().decode("UTF-8")
            sed_args += ['-e', r's,\.diff$,,']

        # Save the list of the modules to file.
        os.chdir(DIRS['cwd'])
        path = joinpath(self.config['tempdir'], 'list')
        with codecs.open(path, 'wb', 'UTF-8') as file:
            file.write(result)

        # Filter the list of the modules.
        stdin = codecs.open(path, 'rb', 'UTF-8')
        sed = sp.Popen(sed_args, stdin=stdin, stdout=sp.PIPE)
        result = sed.stdout.read().decode("UTF-8")
        stdin.close()
        os.remove(path)
        listing = [line for line in result.split('\n') if line.strip()]
        listing = sorted(set(listing))
        return(listing)


#===============================================================================
# Define GLModule class
#===============================================================================
class GLModule(object):
    '''GLModule is used to create a module object from the file with the given
    path. GLModule can get all information about module, get its dependencies,
    files, etc.'''

    def __init__(self, config, module, patched=False):
        '''GLModule.__init__(config, module[, patched]) -> GLModule

        Create new GLModule instance. Arguments are module and patched, where
        module is a string representing the path to the module and patched is a
        bool indicating that module was created after applying patch.'''
        self.args = dict()
        self.cache = dict()
        self.content = string()
        if type(config) is not GLConfig:
            raise(TypeError('config must be a GLConfig, not %s' %
                            type(config).__name__))
        if type(module) is bytes or type(module) is string:
            if type(module) is bytes:
                module = module.decode(ENCS['default'])
        else:  # if module has not bytes or string type
            raise(TypeError('module must be a string, not %s' %
                            type(module).__name__))
        if type(patched) is not bool:
            raise(TypeError('patched must be a bool, not %s' %
                            type(module).__name__))
        self.module = module
        self.patched = patched
        self.config = config
        self.filesystem = GLFileSystem(self.config)
        self.modulesystem = GLModuleSystem(self.config)
        with codecs.open(module, 'rb', 'UTF-8') as file:
            self.content = file.read()
        self.regex = '(?:Description:|Comment:|Status:|Notice:|Applicability:|\
Files:|Depends-on:|configure\\.ac-early:|configure\\.ac:|Makefile\\.am:|\
Include:|Link:|License:|Maintainer:)'

    def __eq__(self, module):
        '''x.__eq__(y) <==> x==y'''
        result = bool()
        if type(module) is GLModule:
            if self.module == module.module:
                result = True
        return(result)

    def __ne__(self, module):
        '''x.__ne__(y) <==> x!=y'''
        result = bool()
        if type(module) is GLModule:
            if self.module != module.module:
                result = True
        return(result)

    def __ge__(self, module):
        '''x.__ge__(y) <==> x>=y'''
        result = bool()
        if type(module) is GLModule:
            if self.module >= module.module:
                result = True
        return(result)

    def __gt__(self, module):
        '''x.__gt__(y) <==> x>y'''
        result = bool()
        if type(module) is GLModule:
            if self.module > module.module:
                result = True
        return(result)

    def __hash__(self):
        '''x.__hash__() <==> hash(x)'''
        module = hash(self.module)
        patched = hash(self.patched)
        result = module ^ patched
        return(result)

    def __le__(self, module):
        '''x.__le__(y) <==> x<=y'''
        result = bool()
        if type(module) is GLModule:
            if self.module <= module.module:
                result = True
        return(result)

    def __lt__(self, module):
        '''x.__lt__(y) <==> x<y'''
        result = bool()
        if type(module) is GLModule:
            if self.module < module.module:
                result = True
        return(result)

    def __str__(self):
        '''x.__str__() <==> str(x)'''
        result = self.getName()
        return(result)

    def __repr__(self):
        '''x.__repr__ <==> repr(x)'''
        result = '<pygnulib.GLModule %s %s>' % \
            (repr(self.getName()), hex(id(self)))
        return(result)

    def getName(self):
        '''GLModule.getName() -> string

        Return the name of the module.'''
        pattern = compiler(joinpath('modules', '(.*?)$'))
        result = pattern.findall(self.module)[0]
        return(result)

    def isPatched(self):
        '''GLModule.isPatched() -> bool

        Check whether module was created after applying patch.'''
        return(self.patched)

    def isTests(self):
        '''GLModule.isTests() -> bool

        Check whether module is a -tests version of module.'''
        result = self.getName().endswith('-tests')
        return(result)

    def isNonTests(self):
        '''GLModule.isTests() -> bool

        Check whether module is not a -tests version of module.'''
        result = not(self.isTests())
        return(result)

    def getTestsName(self):
        '''Return -tests version of the module name.'''
        result = self.getName()
        if not result.endswith('-tests'):
            result += '-tests'
        return(result)

    def getTestsModule(self):
        '''Return -tests version of the module as GLModule.'''
        result = self.modulesystem.find(self.getTestsName())
        return(result)

    def getShellFunc(self):
        '''GLModule.getShellFunc() -> string

        Computes the shell function name that will contain the m4 macros for the
        module.'''
        isalnum = True
        macro_prefix = self.config['macro_prefix']
        for char in str(module):
            if char not in constants.ALPHANUMERIC:
                isalnum = False
                break
        if isalnum:
            module = str(self)
        else:  # if not isalnum
            module = '%s\n' % str(self)
            if type(module) is string:
                module = module.encode(ENCS['default'])
            module = hashlib.md5(module).hexdigest()
        result = 'func_%s_gnulib_m4code_%s' % (macro_prefix, module)
        if type(result) is bytes:
            result = result.decode(ENCS['default'])
        return(result)

    def getShellVar(self):
        '''GLModule.getShellVar() -> string

        Compute the shell variable name the will be set to true once the m4 macros
        for the module have been executed.'''
        isalnum = True
        macro_prefix = self.config['macro_prefix']
        for char in str(module):
            if char not in constants.ALPHANUMERIC:
                isalnum = False
                break
        if isalnum:
            module = str(self)
        else:  # if not isalnum
            module = '%s\n' % str(self)
            if type(module) is string:
                module = module.encode(ENCS['default'])
            module = hashlib.md5(module).hexdigest()
        result = '%s_gnulib_enabled_%s' % (macro_prefix, module)
        if type(result) is bytes:
            result = result.decode(ENCS['default'])
        return(result)

    def getConditionalName(self):
        '''GLModule.getConditionalName() -> string

        Return the automake conditional name.
        GLConfig: macro_prefix.'''
        macro_prefix = self.config['macro_prefix']
        nonascii = \
            [  # Begin to filter non-ascii chars
                char for char in self.getName() if char not in \
                'abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_'
            ]  # Finish to filter non-ascii chars
        if nonascii:
            name = self.getName().encode(ENCS['default'])
            name = hashlib.md5(name).hexdigest()
            conditional = '%s_GNULIB_ENABLED_%s' % (macro_prefix, name)
        else:  # if not nonascii
            result = '%s_GNULIB_ENABLED_%s' (macro_prefix, name)
        if type(result) is bytes:
            result = result.decode(ENCS['default'])
        return(result)

    def getDescription(self):
        '''GLModule.getDescription() -> string

        Return description of the module.'''
        section = 'Description:'
        if 'description' not in self.cache:
            if section not in self.content:
                result = string()
            else:  # if section in self.content
                pattern = '^%s[\t ]*(.*?)%s' % (section, self.regex)
                pattern = compiler(pattern, re.S | re.M)
                result = pattern.findall(self.content)
                if type(result) is list:
                    if not result:
                        result = string()
                    else:  # if result
                        result = result[-1]
            result = result.strip()
            self.cache['description'] = result
        return(self.cache['description'])

    def getComment(self):
        '''GLModule.getComment() -> string

        Return comment to module.'''
        section = 'Comment:'
        if 'comment' not in self.cache:
            if section not in self.content:
                result = string()
            else:  # if section in self.content
                pattern = '^%s[\t ]*(.*?)%s' % (section, self.regex)
                pattern = compiler(pattern, re.S | re.M)
                result = pattern.findall(self.content)
                if type(result) is list:
                    if not result:
                        result = string()
                    else:  # if result
                        result = result[-1]
            result = result.strip()
            self.cache['comment'] = result
        return(self.cache['comment'])

    def getStatus(self):
        '''GLModule.getStatus() -> string

        Return module status.'''
        section = 'Status:'
        if 'status' not in self.cache:
            if section not in self.content:
                result = string()
            else:  # if section in self.content
                snippet = self.content.split(section)[-1]
                snippet = snippet.replace('\r\n', '\n')
                lines = ['%s\n' % line for line in snippet.split('\n')]
                parts = list()
                for line in lines:
                    regex = '^(Description|Comment|Status|Notice|Applicability|'
                    regex += 'Files|Depends-on|configure\\.ac-early|configure\\.ac|'
                    regex += 'Makefile\\.am|Include|Link|License|Maintainer):$'
                    pattern = compiler(regex)
                    findflag = pattern.findall(line)
                    if findflag:
                        break
                    parts += [line]
                result = [part.strip() for part in parts if part.strip()]
            self.cache['status'] = list(result)
        return(list(self.cache['status']))

    def getNotice(self):
        '''GLModule.getNotice() -> string

        Return notice to module.'''
        section = 'Notice:'
        if 'notice' not in self.cache:
            if section not in self.content:
                result = string()
            else:  # if section in self.content
                snippet = self.content.split(section)[-1]
                snippet = snippet.replace('\r\n', '\n')
                lines = ['%s\n' % line for line in snippet.split('\n')]
                parts = list()
                for line in lines:
                    regex = '^(Description|Comment|Status|Notice|Applicability|'
                    regex += 'Files|Depends-on|configure\\.ac-early|configure\\.ac|'
                    regex += 'Makefile\\.am|Include|Link|License|Maintainer):$'
                    pattern = compiler(regex)
                    findflag = pattern.findall(line)
                    if findflag:
                        break
                    parts += [line]
                result = ''.join(parts)
            self.cache['notice'] = result
        return(self.cache['notice'])

    def getApplicability(self):
        '''GLModule.getApplicability() -> string

        Return applicability of module.'''
        section = 'Applicability:'
        if 'applicability' not in self.cache:
            if section not in self.content:
                result = string()
            else:  # if section in self.content
                snippet = self.content.split(section)[-1]
                snippet = snippet.replace('\r\n', '\n')
                lines = ['%s\n' % line for line in snippet.split('\n')]
                parts = list()
                for line in lines:
                    regex = '^(Description|Comment|Status|Notice|Applicability|'
                    regex += 'Files|Depends-on|configure\\.ac-early|configure\\.ac|'
                    regex += 'Makefile\\.am|Include|Link|License|Maintainer):$'
                    pattern = compiler(regex)
                    findflag = pattern.findall(line)
                    if findflag:
                        break
                    parts += [line]
                parts = [part.strip() for part in parts]
                result = ''.join(parts)
            if not result.strip():
                if self.getName().endswith('-tests'):
                    result = 'tests'
                else:  # if not self.getName().endswith('-tests')
                    result = 'main'
            if type(result) is bytes:
                result = result.decode(ENCS['default'])
            result = result.strip()
            self.cache['applicability'] = result
        return(self.cache['applicability'])

    def getFiles(self):
        '''GLModule.getFiles() -> list

        Return list of files.
        GLConfig: ac_version.'''
        ac_version = self.config['ac_version']
        section = 'Files:'
        result = list()
        if 'files' not in self.cache:
            if section not in self.content:
                result = list()
            else:  # if section in self.content
                snippet = self.content.split(section)[-1]
                snippet = snippet.replace('\r\n', '\n')
                lines = ['%s\n' % line for line in snippet.split('\n')]
                parts = list()
                for line in lines:
                    regex = '^(Description|Comment|Status|Notice|Applicability|'
                    regex += 'Files|Depends-on|configure\\.ac-early|configure\\.ac|'
                    regex += 'Makefile\\.am|Include|Link|License|Maintainer):$'
                    pattern = compiler(regex)
                    findflag = pattern.findall(line)
                    if findflag:
                        break
                    parts += [line]
                result = [part.strip() for part in parts if part.strip()]
            result += [joinpath('m4', '00gnulib.m4')]
            result += [joinpath('m4', 'zzgnulib.m4')]
            result += [joinpath('m4', 'gnulib-common.m4')]
            self.cache['files'] = list(result)
        return(list(self.cache['files']))

    def getDependencies(self):
        '''GLModule.getDependencies() -> list

        Return list of dependencies.
        GLConfig: localdir.'''
        localdir = self.config['localdir']
        result = list()
        section = 'Depends-on:'
        if 'dependencies' not in self.cache:
            if section not in self.content:
                depmodules = list()
            else:  # if section in self.content
                snippet = self.content.split(section)[-1]
                snippet = snippet.replace('\r\n', '\n')
                lines = ['%s\n' % line for line in snippet.split('\n')]
                parts = list()
                for line in lines:
                    regex = '^(Description|Comment|Status|Notice|Applicability|'
                    regex += 'Files|Depends-on|configure\\.ac-early|configure\\.ac|'
                    regex += 'Makefile\\.am|Include|Link|License|Maintainer):$'
                    pattern = compiler(regex)
                    findflag = pattern.findall(line)
                    if findflag:
                        break
                    parts += [line]
                modules = ''.join(parts)
                modules = [line for line in modules.split(
                    '\n') if line.strip()]
                modules = [
                    module for module in modules if not module.startswith('#')]
                for line in modules:
                    split = [part for part in line.split(' ') if part.strip()]
                    if len(split) == 1:
                        module = line.strip()
                        condition = None
                    else:  # if len(split) != 1
                        module = split[0]
                        condition = split[1]
                        if type(condition) is bytes:
                            condition = condition.decode(ENCS['default'])
                    result += [tuple([self.modulesystem.find(module), condition])]
            self.cache['dependencies'] = result
        return(list(self.cache['dependencies']))

    def getAutoconfSnippet_Early(self):
        '''GLModule.getAutoconfSnippet_Early() -> string

        Return autoconf-early snippet.'''
        section = 'configure.ac-early:'
        if 'autoconf-early' not in self.cache:
            if section not in self.content:
                result = string()
            else:  # if section in self.content
                snippet = self.content.split(section)[-1]
                snippet = snippet.replace('\r\n', '\n')
                lines = ['%s\n' % line for line in snippet.split('\n')]
                parts = list()
                for line in lines:
                    regex = '^(Description|Comment|Status|Notice|Applicability|'
                    regex += 'Files|Depends-on|configure\\.ac-early|configure\\.ac|'
                    regex += 'Makefile\\.am|Include|Link|License|Maintainer):$'
                    pattern = compiler(regex)
                    findflag = pattern.findall(line)
                    if findflag:
                        break
                    parts += [line]
                result = ''.join(parts)
            self.cache['autoconf-early'] = result
        return(self.cache['autoconf-early'])

    def getAutoconfSnippet(self):
        '''GLModule.getAutoconfSnippet() -> string

        Return autoconf snippet.'''
        section = 'configure.ac:'
        if 'autoconf' not in self.cache:
            if section not in self.content:
                result = string()
            else:  # if section in self.content
                snippet = self.content.split(section)[-1]
                snippet = snippet.replace('\r\n', '\n')
                lines = ['%s\n' % line for line in snippet.split('\n')]
                parts = list()
                for line in lines:
                    regex = '^(Description|Comment|Status|Notice|Applicability|'
                    regex += 'Files|Depends-on|configure\\.ac-early|configure\\.ac|'
                    regex += 'Makefile\\.am|Include|Link|License|Maintainer):$'
                    pattern = compiler(regex)
                    findflag = pattern.findall(line)
                    if findflag:
                        break
                    parts += [line]
                result = ''.join(parts)
            self.cache['autoconf'] = result
        return(self.cache['autoconf'])

    def getAutomakeSnippet(self):
        '''getAutomakeSnippet() -> string

        Get automake snippet.
        GLConfig: auxdir, ac_version.'''
        result = string()  # Define stack variable
        conditional = self.getAutomakeSnippet_Conditional()
        if conditional.strip():
            result += self.getAutomakeSnippet_Conditional()
        else:  # if not conditional.strip()
            result += '\n'
        result += self.getAutomakeSnippet_Unconditional()
        return(result)

    def getAutomakeSnippet_Conditional(self):
        '''GLModule.getAutomakeSnippet_Conditional() -> string

        Return conditional automake snippet.'''
        section = 'Makefile.am:'
        if 'makefile-conditional' not in self.cache:
            if section not in self.content:
                result = string()
            else:  # if section in self.content
                snippet = self.content.split(section)[-1]
                snippet = snippet.replace('\r\n', '\n')
                lines = ['%s\n' % line for line in snippet.split('\n')]
                parts = list()
                for line in lines:
                    regex = '^(Description|Comment|Status|Notice|Applicability|'
                    regex += 'Files|Depends-on|configure\\.ac-early|configure\\.ac|'
                    regex += 'Makefile\\.am|Include|Link|License|Maintainer):$'
                    pattern = compiler(regex)
                    findflag = pattern.findall(line)
                    if findflag:
                        break
                    parts += [line]
                result = ''.join(parts)
            self.cache['makefile-conditional'] = result
        return(self.cache['makefile-conditional'])

    def getAutomakeSnippet_Unconditional(self):
        '''GLModule.getAutomakeSnippet_Unconditional() -> string

        Return unconditional automake snippet.
        GLConfig: auxdir, ac_version.'''
        auxdir = self.config['auxdir']
        ac_version = self.config['ac_version']
        result = string()
        if 'makefile-unconditional' not in self.cache:
            if self.isTests():
                files = self.getFiles()
                extra_files = filter_filelist(constants.NL, files,
                                              'tests/', '', 'tests/', '').split(constants.NL)
                extra_files = sorted(set(extra_files))
                if extra_files:
                    result += string('EXTRA_DIST += %s' %
                                     ' '.join(extra_files))
                    result += constants.NL * 2
            else:  # if not tests module
                # TODO: unconditional automake snippet for nontests modules
                snippet = self.getAutomakeSnippet_Conditional()
                snippet = constants.combine_lines(snippet)
                pattern = compiler(
                    '^lib_SOURCES[\t ]*\\+=[\t ]*(.*?)$', re.S | re.M)
                mentioned_files = pattern.findall(snippet)
                if mentioned_files != list():
                    mentioned_files = mentioned_files[-1].split(' ')
                    mentioned_files = [f.strip() for f in mentioned_files]
                    mentioned_files = [f for f in mentioned_files if f != '']
                    mentioned_files = sorted(set(mentioned_files))
                all_files = self.getFiles()
                lib_files = filter_filelist(constants.NL, all_files,
                                            'lib/', '', 'lib/', '').split(constants.NL)
                extra_files = [
                    f for f in lib_files if f not in mentioned_files]
                extra_files = sorted(set(extra_files))
                if extra_files != [''] and extra_files:
                    result += string('EXTRA_DIST += %s' %
                                     ' '.join(extra_files))
                    result += '\n\n'
                # Synthesize also an EXTRA_lib_SOURCES augmentation
                if str(self) != 'relocatable-prog-wrapper' and str(self) != 'pt_chown':
                    extra_files = filter_filelist(constants.NL, extra_files,
                                                  '', '.c', '', '').split(constants.NL)
                    extra_files = sorted(set(extra_files))
                    if extra_files != ['']:
                        result += string('EXTRA_lib_SOURCES += %s' %
                                         ' '.join(extra_files))
                        result += '\n\n'
                # Synthesize an EXTRA_DIST augmentation also for the files in build-aux
                buildaux_files = filter_filelist(constants.NL, all_files,
                                                 'build-aux/', '', 'build-aux/', '').split(constants.NL)
                buildaux_files = sorted(set(buildaux_files))
                if buildaux_files != ['']:
                    buildaux_files = ''.join(buildaux_files)
                    buildaux_files = joinpath(
                        '$(top_srcdir)', auxdir, buildaux_files)
                    result += string('EXTRA_DIST += %s' % buildaux_files)
                    result += '\n\n'
                # Synthesize an EXTRA_DIST augmentation also for the files from top/.
                top_files = filter_filelist(constants.NL, all_files,
                                            'top/', '', 'top/', '').split(constants.NL)
                top_files = sorted(set(top_files))
                if top_files != ['']:
                    top_files = ''.join(top_files)
                    top_files = joinpath('$(top_srcdir)', top_files)
                    result += string('EXTRA_DIST += %s' % top_files)
                    result += '\n\n'
            result = constants.nlconvert(result)
            self.cache['makefile-unconditional'] = result
        return(self.cache['makefile-unconditional'])

    def getInclude(self):
        '''GLModule.getInclude() -> string

        Return include directive.'''
        section = 'Include:'
        if 'include' not in self.cache:
            if section not in self.content:
                result = string()
            else:  # if section in self.content
                snippet = self.content.split(section)[-1]
                snippet = snippet.replace('\r\n', '\n')
                lines = ['%s\n' % line for line in snippet.split('\n')]
                parts = list()
                for line in lines:
                    regex = '^(Description|Comment|Status|Notice|Applicability|'
                    regex += 'Files|Depends-on|configure\\.ac-early|configure\\.ac|'
                    regex += 'Makefile\\.am|Include|Link|License|Maintainer):$'
                    pattern = compiler(regex)
                    findflag = pattern.findall(line)
                    if findflag:
                        break
                    parts += [line]
                result = ''.join(parts)
            result = result.strip()
            pattern = compiler('^(["<].*?[>"])', re.S | re.M)
            result = pattern.sub('#include \\1', result)
            self.cache['include'] = result
        return(self.cache['include'])

    def getLink(self):
        '''GLModule.getLink() -> string

        Return link directive.'''
        section = 'Link:'
        if 'link' not in self.cache:
            parts = list()
            if section in self.content:
                snippet = self.content.split(section)[-1]
                snippet = snippet.replace('\r\n', '\n')
                lines = ['%s\n' % line for line in snippet.split('\n')]
                for line in lines:
                    regex = '^(Description|Comment|Status|Notice|Applicability|'
                    regex += 'Files|Depends-on|configure\\.ac-early|configure\\.ac|'
                    regex += 'Makefile\\.am|Include|Link|License|Maintainer):$'
                    pattern = compiler(regex)
                    findflag = pattern.findall(line)
                    if findflag:
                        break
                    parts += [line]
                parts = [part.strip() for part in parts if part.strip()]
                # result = ' '.join(parts)
            self.cache['link'] = parts
        return(self.cache['link'])

    def getLicense(self):
        '''GLModule.getLicense(self) -> string

        Get license and warn user if module lacks a license.'''
        license = self.getLicense_Raw()
        if not self.isTests():
            if not license:
                if self.config['errors']:
                    raise(GLError(18, string(self)))
                else:  # if not self.config['errors']
                    sys.stderr.write('gnulib-tool: warning: ')
                    sys.stderr.write('module %s lacks a license\n' % str(self))
        if not license:
            license = 'GPL'
        return(license)

    def getLicense_Raw(self):
        '''GLModule.getLicense_Raw() -> string

        Return module license.'''
        section = 'License:'
        if 'license' not in self.cache:
            if section not in self.content:
                result = string()
            else:  # if section in self.content
                pattern = '^%s[\t ]*(.*?)%s' % (section, self.regex)
                pattern = compiler(pattern, re.S | re.M)
                result = pattern.findall(self.content)
                if type(result) is list:
                    if not result:
                        result = string()
                    else:  # if result
                        result = result[-1]
            result = result.strip()
            self.cache['license'] = result
        return(self.cache['license'])

    def getMaintainer(self):
        '''GLModule.getMaintainer() -> string

        Return maintainer directive.'''
        section = 'Maintainer:'
        if 'maintainer' not in self.cache:
            if section not in self.content:
                result = string()
            else:  # if section in self.content
                snippet = self.content.split(section)[-1]
                snippet = snippet.replace('\r\n', '\n')
                lines = ['%s\n' % line for line in snippet.split('\n')]
                parts = list()
                for line in lines:
                    regex = '^(Description|Comment|Status|Notice|Applicability|'
                    regex += 'Files|Depends-on|configure\\.ac-early|configure\\.ac|'
                    regex += 'Makefile\\.am|Include|Link|License|Maintainer):$'
                    pattern = compiler(regex)
                    findflag = pattern.findall(line)
                    if findflag:
                        break
                    parts += [line]
                result = ''.join(parts)
            result = result.strip()
            self.cache['maintainer'] = result
        return(self.cache['maintainer'])


#===============================================================================
# Define GLModuleTable class
#===============================================================================
class GLModuleTable(object):
    '''GLModuleTable is used to work with the list of the modules.'''

    def __init__(self, config, avoids=list()):
        '''GLModuleTable.__init__(config, avoids) -> GLModuleTable

        Create new GLModuleTable instance. If modules are specified, then add
        every module from iterable as unconditional module. If avoids is specified,
        then in transitive_closure every dependency which is in avoids won't be
        included in the final modules list. If testflags iterable is enabled, then
        don't add module which status is in the testflags. If conddeps are enabled,
        then store condition for each dependency if it has a condition.
        The only necessary argument is localdir, which is needed just to create
        modulesystem instance to look for dependencies.'''
        self.avoids = list()  # Avoids
        self.dependers = dict()  # Dependencies
        self.conditionals = dict()  # Conditional modules
        self.unconditionals = dict()  # Unconditional modules
        self.base_modules = list()  # Base modules
        self.main_modules = list()  # Main modules
        self.tests_modules = list()  # Tests modules
        self.final_modules = list()  # Final modules
        if type(config) is not GLConfig:
            raise(TypeError('config must be a GLConfig, not %s' %
                            type(config).__name__))
        for avoid in avoids:
            if type(avoid) is not GLModule:
                raise(TypeError('each avoid must be a GLModule instance'))
            self.avoids += [avoids]
        self.config = config
        self.filesystem = GLFileSystem(self.config)
        self.modulesystem = GLModuleSystem(self.config)

    def __repr__(self):
        '''x.__repr__() <==> repr(x)'''
        result = '<pygnulib.GLModuleTable %s>' % hex(id(self))
        return(result)

    def __getitem__(self, y):
        '''x.__getitem__(y) <==> x[y]'''
        if y in ['base', 'final', 'main', 'tests', 'avoids']:
            if y == 'base':
                return(self.getBaseModules())
            elif y == 'final':
                return(self.getFinalModules())
            elif y == 'main':
                return(self.getMainModules())
            elif y == 'tests':
                return(self.getTestsModules())
            else:  # if y == 'avoids'
                return(self.getAvoids())
        else:  # if y is not in list
            raise(KeyError('GLModuleTable does not contain key: %s' % repr(y)))

    def addConditional(self, parent, module, condition):
        '''GLModuleTable.addConditional(module, condition)

        Add new conditional dependency from parent to module with condition.'''
        if type(parent) is not GLModule:
            raise(TypeError('parent must be a GLModule, not %s' %
                            type(parent).__name__))
        if type(module) is not GLModule:
            raise(TypeError('module must be a GLModule, not %s' %
                            type(module).__name__))
        if type(condition) is bytes or type(condition) is string \
                or condition == True:
            if type(condition) is bytes:
                condition = condition.decode(ENCS['default'])
        else:  # if condition has not bytes or string type or is not True
            raise(TypeError('condition must be a string or True, not %s' %
                            type(condition).__name__))
        if not str(module) in self.unconditionals:
            if str(module) not in self.dependers:
                self.dependers[module] = list()
            self.dependers[module] += [module]
            key = '%s---%s' % (str(parent), str(module))
            self.conditionals[key] = condition

    def addUnconditional(self, module):
        '''GLModuleTable.addUnconditional(module)

        Add module as unconditional dependency.'''
        if type(module) is not GLModule:
            raise(TypeError('module must be a GLModule, not %s' %
                            type(module).__name__))
        if str(module) in self.dependers:
            self.dependers.pop(str(module))
        self.unconditionals[str(module)] = True

    def isConditional(self, module):
        '''GLModuleTable.isConditional(module) -> bool

        Check whether module is unconditional.'''
        if type(module) is not GLModule:
            raise(TypeError('module must be a GLModule, not %s' %
                            type(module).__name__))
        result = str(module) in self.dependers
        return(result)

    def getCondition(self, parent, module):
        '''GLModuleTable.getCondition(module) -> string or True

        Return condition from parent to module. Condition can be string or True.
        If module is not in the list of conddeps, method returns None.'''
        if type(parent) is not GLModule:
            raise(TypeError('parent must be a GLModule, not %s' %
                            type(parent).__name__))
        if type(module) is not GLModule:
            raise(TypeError('module must be a GLModule, not %s' %
                            type(module).__name__))
        key = '%s---%s' % (str(parent), str(module))
        result = None
        if key in self.conditionals:
            result = self.conditionals[key]
        return(result)

    def transitive_closure(self, modules):
        '''GLModuleTable.transitive_closure(modules) -> list

        Use transitive closure to add module and its dependencies. Add every
        module and its dependencies from modules list, but do not add dependencies
        which contain in avoids list. If any testflag is enabled, then do not add
        dependencies which have the status as this flag. If conddeps are enabled,
        then store condition for each dependency if it has a condition. This method
        is used to update final list of modules. Method returns list of modules.
        GLConfig: testflags.'''
        for module in modules:
            if type(module) is not GLModule:
                raise(TypeError('each module must be a GLModule instance'))
        handledmodules = list()
        inmodules = modules
        outmodules = list()
        if self.config['conddeps']:
            for module in modules:
                self.addUnconditional(module)
        while inmodules:
            inmodules_this_round = inmodules
            inmodules = list()
            for module in inmodules_this_round:
                outmodules += [module]
                if self.config['conddeps']:
                    automake_snippet = \
                        module.getAutomakeSnippet_Conditional()
                    pattern = compiler('^if')
                    if not pattern.findall(automake_snippet):
                        self.addUnconditional(module)
                    conditional = self.isConditional(module)
                dependencies = module.getDependencies()
                depmodules = [pair[0] for pair in dependencies]
                conditions = [pair[1] for pair in dependencies]
                if TESTS['tests'] in self.config['testflags']:
                    testsname = module.getTestsName()
                    if self.modulesystem.exists(testsname):
                        testsmodule = self.modulesystem.find(testsname)
                        depmodules += [testsmodule]
                        conditions += [None]
                for depmodule in depmodules:
                    include = True
                    includes = list()
                    status = depmodule.getStatus()
                    for word in status:
                        if word == 'obsolete':
                            if TESTS['obsolete'] in self.config['testflags'] or \
                                    TESTS['all-test'] in self.config['testflags']:
                                includes += [False]
                        elif word == 'c++-test':
                            if TESTS['c++-test'] in self.config['testflags'] or \
                                    TESTS['all-test'] in self.config['testflags']:
                                includes += [False]
                        elif word == 'longrunning-test':
                            if TESTS['longrunning-test'] in self.config['testflags'] or \
                                    TESTS['all-test'] in self.config['testflags']:
                                includes += [False]
                        elif word == 'privileged-test':
                            if TESTS['privileged-test'] in self.config['testflags'] or \
                                    TESTS['all-test'] in self.config['testflags']:
                                includes += [False]
                        elif word == 'all-test':
                            if TESTS['all-test'] in self.config['testflags'] or \
                                    TESTS['all-test'] in self.config['testflags']:
                                includes += [False]
                        else:  # if any other word
                            if word.endswith('-tests'):
                                if TESTS['all-test'] in self.config['testflags']:
                                    includes += [False]
                        include = any(includes)
                    if include and depmodule not in self.avoids:
                        inmodules += [depmodule]
                        if self.config['conddeps']:
                            index = depmodules.index(depmodule)
                            condition = conditions[index]
                            if condition:
                                self.addConditional(
                                    module, depmodule, condition)
                            else:  # if condition
                                if conditional:
                                    self.addConditional(
                                        module, depmodule, True)
                                else:  # if not conditional
                                    self.addUnconditional(module)
            listing = list()  # Create empty list
            inmodules = sorted(set(inmodules))
            handledmodules = sorted(set(handledmodules + inmodules_this_round))
            inmodules = \
                [  # Begin to filter inmodules
                    module for module in inmodules if module not in handledmodules
                ]  # Finish to filter inmodules
            inmodules = sorted(set(inmodules))
        modules = sorted(set(outmodules))
        self.modules = modules
        return(list(modules))

    def transitive_closure_separately(self, basemodules, finalmodules):
        '''GLModuleTable.transitive_closure_separately(*args, **kwargs) -> tuple

        Determine main module list and tests-related module list separately.
        The main module list is the transitive closure of the specified modules,
        ignoring tests modules. Its lib/* sources go into $sourcebase/. If lgpl is
        specified, it will consist only of LGPLed source.
        The tests-related module list is the transitive closure of the specified
        modules, including tests modules, minus the main module list excluding
        modules of applicability 'all'. Its lib/* sources (brought in through
        dependencies of *-tests modules) go into $testsbase/. It may contain GPLed
        source, even if lgpl is specified.
        Arguments are basemodules and finalmodules, where basemodules argument
        represents modules specified by user and finalmodules represents modules
        list after previous transitive_closure.
        Method returns tuple which contains two lists: the list of main modules and
        the list of tests-related modules. Both lists contain dependencies.
        GLConfig: testflags.'''
        inctests = False
        main_modules = list()
        tests_modules = list()
        if TESTS['tests'] in self.config['testflags']:
            self.config['testflags'].pop(TESTS['tests'])
            inctests = True
        for module in basemodules:
            if type(module) is not GLModule:
                raise(TypeError('each module must be a GLModule instance'))
        for module in finalmodules:
            if type(module) is not GLModule:
                raise(TypeError('each module must be a GLModule instance'))
        main_modules = self.transitive_closure(basemodules)
        tests_modules = \
            [m for m in finalmodules if m not in main_modules] + \
            [m for m in main_modules if m.getApplicability() != 'main']
        tests_modules = sorted(set(tests_modules))
        if inctests:
            testflags = sorted(
                set(self.config['testflags'] + [TESTS['tests']]))
            self.config.setTestFlags(testflags)
        result = tuple([main_modules, tests_modules])
        return(result)

    def add_dummy(self, modules):
        '''GLModuleTable.add_dummy(modules) -> list

        Add dummy package to list of modules if dummy package is needed. If not,
        return original list of modules.
        GLConfig: auxdir, ac_version.'''
        auxdir = self.config['auxdir']
        ac_version = self.config['ac_version']
        have_lib_sources = False
        for module in modules:
            if type(module) is not GLModule:
                raise(TypeError('each module must be a GLModule instance'))
            snippet = module.getAutomakeSnippet()
            snippet = constants.remove_backslash_newline(snippet)
            pattern = compiler(
                '^lib_SOURCES[\t ]*\\+=[\t ]*(.*?)$', re.S | re.M)
            files = pattern.findall(snippet)
            if files:  # if source files were found
                files = files[-1].split(' ')
                for file in files:
                    if not file.endswith('.h'):
                        have_lib_sources = True
                        break
        if not have_lib_sources:
            dummy = self.modulesystem.find('dummy')
            modules = sorted(set(modules + [dummy]))
        return(list(modules))

    def filelist(self, modules):
        '''GLModuleTable.filelist(modules) -> list

        Determine the final file list for the given list of modules. The list of
        modules must already include dependencies.
        GLConfig: ac_version.'''
        ac_version = self.config['ac_version']
        filelist = list()
        for module in modules:
            if type(module) is not GLModule:
                raise(TypeError('each module must be a GLModule instance'))
        listings = [module.getFiles() for module in modules]
        for listing in listings:
            for file in listing:
                if file not in filelist:
                    filelist += [file]
        return(filelist)

    def filelist_separately(self, main_modules, tests_modules):
        '''GLModuleTable.filelist_separately(**kwargs) -> list

        Determine the final file lists. They must be computed separately, because
        files in lib/* go into $sourcebase/ if they are in the main file list but
        into $testsbase/ if they are in the tests-related file list. Furthermore
        lib/dummy.c can be in both.'''
        ac_version = self.config['ac_version']
        main_filelist = self.filelist(main_modules)
        tests_filelist = self.filelist(tests_modules)
        tests_filelist = \
            [  # Begin to sort filelist
                file.replace('lib/', 'tests=lib/', 1) \
                if file.startswith('lib/') else file
                for file in tests_filelist
            ]  # Finish to sort filelist
        result = tuple([main_filelist, tests_filelist])
        return(result)

    def getAvoids(self):
        '''GLModuleTable.getAvoids() -> list

        Return list of avoids.'''
        return(list(self.avoids))

    def setAvoids(self, modules):
        '''GLModuleTable.setAvoids(modules)

        Specify list of avoids.'''
        for module in modules:
            if type(module) is not GLModule:
                raise(TypeError('each module must be a GLModule instance'))
        self.avoids = sorted(set(modules))

    def getBaseModules(self):
        '''GLModuleTable.getBaseModules() -> list

        Return list of base modules.'''
        return(list(self.base_modules))

    def setBaseModules(self, modules):
        '''GLModuleTable.setBaseModules(modules)

        Specify list of base modules.'''
        for module in modules:
            if type(module) is not GLModule:
                raise(TypeError('each module must be a GLModule instance'))
        self.base_modules = sorted(set(modules))

    def getFinalModules(self):
        '''GLModuleTable.getFinalModules() -> list

        Return list of final modules.'''
        return(list(self.final_modules))

    def setFinalModules(self, modules):
        '''GLModuleTable.setFinalModules(modules)

        Specify list of final modules.'''
        for module in modules:
            if type(module) is not GLModule:
                raise(TypeError('each module must be a GLModule instance'))
        self.final_modules = sorted(set(modules))

    def getMainModules(self):
        '''GLModuleTable.getMainModules() -> list

        Return list of main modules.'''
        return(list(self.main_modules))

    def setMainModules(self, modules):
        '''GLModuleTable.setMainModules(modules)

        Specify list of main modules.'''
        for module in modules:
            if type(module) is not GLModule:
                raise(TypeError('each module must be a GLModule instance'))
        self.main_modules = sorted(set(modules))

    def getTestsModules(self):
        '''GLModuleTable.getTestsModules() -> list

        Return list of tests modules.'''
        return(list(self.tests_modules))

    def setTestsModules(self, modules):
        '''GLModuleTable.setTestsModules(modules)

        Specify list of tests modules.'''
        for module in modules:
            if type(module) is not GLModule:
                raise(TypeError('each module must be a GLModule instance'))
        self.tests_modules = sorted(set(modules))
