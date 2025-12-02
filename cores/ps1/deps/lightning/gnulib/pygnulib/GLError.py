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
from . import constants


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


#===============================================================================
# Define GLError class
#===============================================================================
class GLError(Exception):
    '''Exception handler for pygnulib classes.'''

    def __init__(self, errno, errinfo=None):
        '''Each error has following parameters:
        errno: code of error; used to catch error type
          1: file does not exist in GLFileSystem: <file>
          2: cannot patch file inside GLFileSystem: <file>
          3: configure file does not exist: <configure.ac>
          4: minimum supported autoconf version is 2.59, not <version>
          5: <gnulib-comp.m4> is expected to contain gl_M4_BASE([<m4base>])
          6: missing sourcebase argument
          7: missing docbase argument
          8: missing testsbase argument
          9: missing libname argument
         10: conddeps are not supported with inctests
         11: incompatible licenses on modules: <modules>
         12: cannot process empty filelist
         13: cannot create the given directory: <directory>
         14: cannot delete the given file: <file>
         15: cannot create the given file: <file>
         16: cannot transform the given file: <file>
         17: cannot update the given file: <file>
         18: module lacks a license: <module>
         19: could not create destination directory: <directory>
        errinfo: additional information;
        style: 0 or 1, whether old-style'''
        self.errno = errno
        self.errinfo = errinfo
        self.args = (self.errno, self.errinfo)

    def __repr__(self):
        errinfo = self.errinfo
        errors = \
            [  # Begin list of errors
                "file does not exist in GLFileSystem: %s" % repr(errinfo),
                "cannot patch file inside GLFileSystem: %s" % repr(errinfo),
                "configure file does not exist: %s" % repr(errinfo),
                "minimum supported autoconf version is 2.59, not %s" % repr(
                    errinfo),
                "%s is expected to contain gl_M4_BASE([%s])" % \
                (repr(os.path.join(errinfo, 'gnulib-comp.m4')), repr(errinfo)),
                "missing sourcebase argument; cache file doesn't contain it,"
                + " so you might have to set this argument",
                "missing docbase argument; you might have to create GLImport" \
                + " instance with mode 0 and docbase argument",
                "missing testsbase argument; cache file doesn't contain it,"
                + " so you might have to set this argument"
                "missing libname argument; cache file doesn't contain it,"
                + " so you might have to set this argument",
                "conddeps are not supported with inctests",
                "incompatible licenses on modules: %s" % repr(errinfo),
                "cannot process empty filelist",
                "cannot create the given directory: %s" % repr(errinfo),
                "cannot remove the given file: %s" % repr(errinfo),
                "cannot create the given file: %s" % repr(errinfo),
                "cannot transform the given file: %s" % repr(errinfo),
                "cannot update/replace the given file: %s" % repr(errinfo),
                "module lacks a license: %s" % repr(errinfo),
                "error when running subprocess: %s" % repr(errinfo),
            ]  # Complete list of errors
        if not PYTHON3:
            self.message = (b'[Errno %d] %s' %
                            (self.errno, errors[self.errno - 1].encode(ENCS['default'])))
        else:  # if PYTHON3
            self.message = ('[Errno %d] %s' %
                            (self.errno, errors[self.errno - 1]))
        return(self.message)
