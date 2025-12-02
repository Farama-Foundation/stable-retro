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
# Define GLMakefileTable class
#===============================================================================
class GLMakefileTable(object):
    '''This class is used to edit Makefile and store edits as table.
    When user creates  Makefile, he may need to use this class.'''

    def __init__(self, config):
        '''GLMakefileTable.__init__(config) -> GLMakefileTable

        Create GLMakefileTable instance.'''
        if type(config) is not GLConfig:
            raise(TypeError('config must be a GLConfig, not %s' %
                            type(config).__name__))
        self.config = config
        self.table = list()

    def __getitem__(self, y):
        '''x.__getitem__(y) = x[y]'''
        if type(y) is not int:
            raise(TypeError('indices must be integers, not %s' %
                            type(y).__name__))
        result = self.table[y]
        return(dict(result))

    def editor(self, dir, var, val):
        '''GLMakefileTable.editor(dir, var, val)

        This method is used to remember that ${dir}Makefile.am needs to be edited
        to that ${var} mentions ${val}.'''
        if type(dir) is bytes or type(dir) is string:
            if type(dir) is bytes:
                dir = dir.decode(ENCS['default'])
        else:  # if dir has not bytes or string type
            raise(TypeError(
                'dir must be a string, not %s' % (type(dir).__name__)))
        if type(var) is bytes or type(var) is string:
            if type(var) is bytes:
                var = var.decode(ENCS['default'])
        else:  # if var has not bytes or string type
            raise(TypeError(
                'var must be a string, not %s' % (type(var).__name__)))
        if type(val) is bytes or type(val) is string:
            if type(val) is bytes:
                val = val.decode(ENCS['default'])
        else:  # if val has not bytes or string type
            raise(TypeError(
                'val must be a string, not %s' % (type(val).__name__)))
        dictionary = {'dir': dir, 'var': var, 'val': val}
        self.table += [dictionary]

    def parent(self):
        '''GLMakefileTable.parent()

        Add a special row to Makefile.am table with the first parent directory
        which contains or will contain Makefile.am file.
        GLConfig: sourcebase, m4base, testsbase, testflags, makefile.'''
        m4base = self.config['m4base']
        sourcebase = self.config['sourcebase']
        testsbase = self.config['testsbase']
        makefile = self.config['makefile']
        inctests = self.config.checkTestFlag(TESTS['tests'])
        dir1 = string('%s%s' % (m4base, os.path.sep))
        mfd = string('Makefile.am')
        if not makefile:
            mfx = string('Makefile.am')
        else:  # if makefile
            mfx = makefile
        dir2 = string()
        while dir1 and \
            (joinpath(self.config['destdir'], dir1, mfd) or
             joinpath(dir1, mfd) == joinpath(sourcebase, mfx) or
             (inctests and joinpath(dir1, mfd) == joinpath(testsbase, mfx))):
            dir2 = joinpath(os.path.basename(dir1), dir2)
            dir1 = os.path.dirname(dir1)
        self.editor(dir1, 'EXTRA_DIST', joinpath(dir2, 'gnulib-cache.m4'))

    def count(self):
        '''GLMakefileTable.count() -> int

        Count number of edits which were applied.'''
        return(len(self.table))
