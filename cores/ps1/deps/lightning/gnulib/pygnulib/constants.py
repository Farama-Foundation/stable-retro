#!/usr/bin/python
# encoding: UTF-8

'''An easy access to pygnulib constants.'''

from __future__ import unicode_literals
#===============================================================================
# Define global imports
#===============================================================================
import re
import os
import sys
import platform
import tempfile
import subprocess as sp


#===============================================================================
# Define module information
#===============================================================================
__all__ = list()
__author__ = \
    [
        'Bruno Haible',
        'Paul Eggert',
        'Simon Josefsson',
        'Dmitriy Selyutin',
    ]
__license__ = 'GNU GPLv3+'
__copyright__ = '2002-2017 Free Software Foundation, Inc.'


#===============================================================================
# Backward compatibility
#===============================================================================
# Check for Python version
if sys.version_info.major == 2:
    PYTHON3 = False
else:
    PYTHON3 = True

# Create string compatibility
if not PYTHON3:
    string = unicode
else:  # if PYTHON3
    string = str

# Current working directory
if not PYTHON3:
    os.getcwdb = os.getcwd
    os.getcwd = os.getcwdu


#===============================================================================
# Define global constants
#===============================================================================
# Declare necessary variables
APP = dict()  # Application
DIRS = dict()  # Directories
UTILS = dict()  # Utilities
ENCS = dict()  # Encodings
MODES = dict()  # Modes
TESTS = dict()  # Tests
NL = '''
'''  # Newline character
ALPHANUMERIC = 'abcdefghijklmnopqrstuvwxyz\
ABCDEFGHIJKLMNOPQRSTUVWXYZ\
0123456789'  # Alphanumeric characters

# Set ENCS dictionary
import __main__ as interpreter
if not hasattr(interpreter, '__file__'):
    if sys.stdout.encoding != None:
        ENCS['default'] = sys.stdout.encoding
    else:  # sys.stdout.encoding == None
        ENCS['default'] = 'UTF-8'
else:  # if hasattr(interpreter, '__file__'):
    ENCS['default'] = 'UTF-8'
ENCS['system'] = sys.getfilesystemencoding()
ENCS['shell'] = sys.stdout.encoding
if ENCS['shell'] == None:
    ENCS['shell'] = 'UTF-8'

# Set APP dictionary
APP['name'] = sys.argv[0]
if not APP['name']:
    APP['name'] = 'gnulib-tool.py'
APP['path'] = os.path.realpath(sys.argv[0])
if type(APP['name']) is bytes:
    APP['name'] = string(APP['name'], ENCS['system'])
if type(APP['path']) is bytes:
    APP['path'] = string(APP['path'], ENCS['system'])

# Set DIRS dictionary
DIRS['root'] = os.path.dirname(APP['path'])
DIRS['cwd'] = os.getcwd()
DIRS['build-aux'] = os.path.join(DIRS['root'], 'build-aux')
DIRS['config'] = os.path.join(DIRS['root'], 'config')
DIRS['doc'] = os.path.join(DIRS['root'], 'doc')
DIRS['lib'] = os.path.join(DIRS['root'], 'lib')
DIRS['m4'] = os.path.join(DIRS['root'], 'm4')
DIRS['modules'] = os.path.join(DIRS['root'], 'modules')
DIRS['tests'] = os.path.join(DIRS['root'], 'tests')
DIRS['git'] = os.path.join(DIRS['root'], '.git')
DIRS['cvs'] = os.path.join(DIRS['root'], 'CVS')

# Set MODES dictionary
MODES = \
    {
        'import': 0,
        'add-import': 1,
        'remove-import': 2,
        'update': 3,
    }
MODES['verbose-min'] = -2
MODES['verbose-default'] = 0
MODES['verbose-max'] = 2

# Set TESTS dictionary
TESTS = \
    {
        'tests':             0,
        'obsolete':          1,
        'c++-test':          2,
        'cxx-test':          2,
        'c++-tests':         2,
        'cxx-tests':         2,
        'longrunning-test':  3,
        'longrunning-tests': 3,
        'privileged-test':   4,
        'privileged-tests':  4,
        'unportable-test':   5,
        'unportable-tests':  5,
        'all-test':          6,
        'all-tests':         6,
    }

# Define AUTOCONF minimum version
DEFAULT_AUTOCONF_MINVERSION = 2.59
# You can set AUTOCONFPATH to empty if autoconf 2.57 is already in your PATH
AUTOCONFPATH = ''
# You can set AUTOMAKEPATH to empty if automake 1.9.x is already in your PATH
AUTOMAKEPATH = ''
# You can set GETTEXTPATH to empty if autopoint 0.15 is already in your PATH
GETTEXTPATH = ''
# You can set LIBTOOLPATH to empty if libtoolize 2.x is already in your PATH
LIBTOOLPATH = ''

# You can also set the variable AUTOCONF individually
if AUTOCONFPATH:
    UTILS['autoconf'] = AUTOCONFPATH + 'autoconf'
else:
    if os.getenv('AUTOCONF'):
        UTILS['autoconf'] = os.getenv('AUTOCONF')
    else:
        UTILS['autoconf'] = 'autoconf'

# You can also set the variable AUTORECONF individually
if AUTOCONFPATH:
    UTILS['autoreconf'] = AUTOCONFPATH + 'autoreconf'
else:
    if os.getenv('AUTORECONF'):
        UTILS['autoreconf'] = os.getenv('AUTORECONF')
    else:
        UTILS['autoreconf'] = 'autoreconf'

# You can also set the variable AUTOHEADER individually
if AUTOCONFPATH:
    UTILS['autoheader'] = AUTOCONFPATH + 'autoheader'
else:
    if os.getenv('AUTOHEADER'):
        UTILS['autoheader'] = os.getenv('AUTOHEADER')
    else:
        UTILS['autoheader'] = 'autoheader'

# You can also set the variable AUTOMAKE individually
if AUTOMAKEPATH:
    UTILS['automake'] = AUTOMAKEPATH + 'automake'
else:
    if os.getenv('AUTOMAKE'):
        UTILS['automake'] = os.getenv('AUTOMAKE')
    else:
        UTILS['automake'] = 'automake'

# You can also set the variable ACLOCAL individually
if AUTOMAKEPATH:
    UTILS['aclocal'] = AUTOMAKEPATH + 'aclocal'
else:
    if os.getenv('ACLOCAL'):
        UTILS['aclocal'] = os.getenv('ACLOCAL')
    else:
        UTILS['aclocal'] = 'aclocal'

# You can also set the variable AUTOPOINT individually
if GETTEXTPATH:
    UTILS['autopoint'] = GETTEXTPATH + 'autopoint'
else:
    if os.getenv('AUTOPOINT'):
        UTILS['autopoint'] = os.getenv('AUTOPOINT')
    else:
        UTILS['autopoint'] = 'autopoint'

# You can also set the variable LIBTOOLIZE individually
if LIBTOOLPATH:
    UTILS['libtoolize'] = LIBTOOLPATH + 'libtoolize'
else:
    if os.getenv('LIBTOOLIZE'):
        UTILS['libtoolize'] = os.getenv('LIBTOOLIZE')
    else:
        UTILS['libtoolize'] = 'libtoolize'

# You can also set the variable MAKE
if os.getenv('MAKE'):
    UTILS['make'] = os.getenv('MAKE')
else:
    UTILS['make'] = 'make'


#===============================================================================
# Define global functions
#===============================================================================
def execute(args, verbose):
    '''Execute the given shell command.'''
    if verbose >= 0:
        print("executing %s" % ' '.join(args))
        try:  # Try to run
            retcode = sp.call(args)
        except Exception as error:
            print(error)
            sys.exit(1)
    else:
        # Commands like automake produce output to stderr even when they succeed.
        # Turn this output off if the command succeeds.
        temp = tempfile.mktemp()
        if type(temp) is bytes:
            temp = temp.decode(ENCS['system'])
        xargs = '%s > %s 2>&1' % (' '.join(args), temp)
        try:  # Try to run
            retcode = sp.call(xargs, shell=True)
        except Exception as error:
            print(error)
            sys.exit(1)
        if retcode == 0:
            os.remove(temp)
        else:
            print("executing %s" % ' '.join(args))
            with codecs.open(temp, 'rb') as file:
                cmdout = file.read()
            print(cmdout)
            os.remove(temp)
            sys.exit(retcode)


def compiler(pattern, flags=0):
    '''Compile regex pattern depending on version of Python.'''
    if not PYTHON3:
        pattern = re.compile(pattern, re.UNICODE | flags)
    else:  # if PYTHON3
        pattern = re.compile(pattern, flags)
    return(pattern)


def cleaner(sequence):
    '''Clean string or list of strings after using regex.'''
    if type(sequence) is string:
        sequence = sequence.replace('[', '')
        sequence = sequence.replace(']', '')
    elif type(sequence) is list:
        sequence = [value.replace('[', '').replace(']', '')
                    for value in sequence]
        sequence = [value.replace('(', '').replace(')', '')
                    for value in sequence]
        sequence = [False if value == 'false' else value for value in sequence]
        sequence = [True if value == 'true' else value for value in sequence]
        sequence = [value.strip() for value in sequence]
    return(sequence)


def joinpath(head, *tail):
    '''joinpath(head, *tail) -> string

    Join two or more pathname components, inserting '/' as needed. If any
    component is an absolute path, all previous path components will be
    discarded. The second argument may be string or list of strings.'''
    newtail = list()
    if type(head) is bytes:
        head = head.decode(ENCS['default'])
    for item in tail:
        if type(item) is bytes:
            item = item.decode(ENCS['default'])
        newtail += [item]
    result = os.path.normpath(os.path.join(head, *tail))
    if type(result) is bytes:
        result = result.decode(ENCS['default'])
    return(result)


def relativize(dir1, dir2):
    '''Compute a relative pathname reldir such that dir1/reldir = dir2.'''
    dir0 = os.getcwd()
    if type(dir1) is bytes:
        dir1 = dir1.decode(ENCS['default'])
    if type(dir2) is bytes:
        dir2 = dir2.decode(ENCS['default'])
    while dir1:
        dir1 = '%s%s' % (os.path.normpath(dir1), os.path.sep)
        dir2 = '%s%s' % (os.path.normpath(dir2), os.path.sep)
        if dir1.startswith(os.path.sep):
            first = dir1[:dir1.find(os.path.sep, 1)]
        else:  # if not dir1.startswith('/')
            first = dir1[:dir1.find(os.path.sep)]
        if first != '.':
            if first == '..':
                dir2 = os.path.basename(joinpath(dir0, dir2))
                dir0 = os.path.dirname(dir0)
            else:  # if first != '..'
                # Get first component of dir2
                if dir2.startswith(os.path.sep):
                    first2 = dir2[:dir2.find(os.path.sep, 1)]
                else:  # if not dir1.startswith('/')
                    first2 = dir2[:dir2.find(os.path.sep)]
                if first == first2:
                    dir2 = dir2[dir2.find(os.path.sep) + 1:]
                else:  # if first != first2
                    dir2 = joinpath('..', dir2)
                dir0 = joinpath(dir0, first)
        dir1 = dir1[dir1.find(os.path.sep) + 1:]
    result = os.path.normpath(dir2)
    return(result)


def link_relative(src, dest):
    '''Like ln -s, except that src is given relative to the current directory
    (or absolute), not given relative to the directory of dest.'''
    if type(src) is bytes or type(src) is string:
        if type(src) is bytes:
            src = src.decode(ENCS['default'])
    else:  # if src has not bytes or string type
        raise(TypeError(
            'src must be a string, not %s' % (type(src).__name__)))
    if type(dest) is bytes or type(dest) is string:
        if type(dest) is bytes:
            dest = dest.decode(ENCS['default'])
    else:  # if dest has not bytes or string type
        raise(TypeError(
            'dest must be a string, not %s' % (type(dest).__name__)))
    if src.startswith('/') or (len(src) >= 2 and src[1] == ':'):
        os.symlink(src, dest)
    else:  # if src is not absolute
        if dest.startswith('/') or (len(dest) >= 2 and dest[1] == ':'):
            if not constants.PYTHON3:
                cwd = os.getcwdu()
            else:  # if constants.PYTHON3
                cwd = os.getcwd()
            os.symlink(joinpath(cwd, src), dest)
        else:  # if dest is not absolute
            destdir = os.path.dirname(dest)
            if not destdir:
                destdir = '.'
            if type(destdir) is bytes:
                destdir = destdir.decode(ENCS['default'])
            src = relativize(destdir, src)
            os.symlink(src, dest)


def link_if_changed(src, dest):
    '''Create a symlink, but avoids munging timestamps if the link is correct.'''
    if type(src) is bytes:
        src = src.decode(ENCS['default'])
    if type(dest) is bytes:
        dest = dest.decode(ENCS['default'])
    ln_target = os.path.realpath(src)
    if not (os.path.islink(dest) and src == ln_target):
        os.remove(dest)
        link_relative(src, dest)


def filter_filelist(separator, filelist,
                    prefix, suffix, removed_prefix, removed_suffix,
                    added_prefix=string(), added_suffix=string()):
    '''filter_filelist(*args) -> list

    Filter the given list of files. Filtering: Only the elements starting with
    prefix and ending with suffix are considered. Processing: removed_prefix
    and removed_suffix are removed from each element, added_prefix and
    added_suffix are added to each element.'''
    listing = list()
    for filename in filelist:
        if filename.startswith(prefix) and filename.endswith(suffix):
            pattern = compiler('^%s(.*?)%s$' %
                               (removed_prefix, removed_suffix))
            result = pattern.sub('%s\\1%s' %
                                 (added_prefix, added_suffix), filename)
            listing += [result]
    result = separator.join(listing)
    return(result)


def substart(orig, repl, data):
    '''Replaces the start portion of a string.

    Returns data with orig replaced by repl, but only at the beginning of data.
    Like data.replace(orig,repl), except only at the beginning of data.'''
    result = data
    if data.startswith(orig):
        result = repl + data[len(orig):]
    return(result)


def subend(orig, repl, data):
    '''Replaces the end portion of a string.

    Returns data with orig replaced by repl, but only at the end of data.
    Like data.replace(orig,repl), except only at the end of data.'''
    result = data
    if data.endswith(orig):
        result = data[:-len(orig)] + repl
    return(result)


def nlconvert(text):
    '''Convert line-endings to specific for this platform.'''
    system = platform.system().lower()
    text = text.replace('\r\n', '\n')
    if system == 'windows':
        text = text.replace('\n', '\r\n')
    return(text)


def nlremove(text):
    '''Remove empty lines from the source text.'''
    text = nlconvert(text)
    text = text.replace('\r\n', '\n')
    lines = [line for line in text.split('\n') if line != '']
    text = '\n'.join(lines)
    text = nlconvert(text)
    return(text)


def remove_backslash_newline(text):
    '''Given a multiline string text, join lines:
    When a line ends in a backslash, remove the backslash and join the next
    line to it.'''
    return text.replace('\\\n', '')

def combine_lines(text):
    '''Given a multiline string text, join lines by spaces:
    When a line ends in a backslash, remove the backslash and join the next
    line to it, inserting a space between them.'''
    return text.replace('\\\n', ' ')

def combine_lines_matching(pattern, text):
    '''Given a multiline string text, join lines by spaces, when the first
    such line matches a given RegexObject pattern.
    When a line that matches the pattern ends in a backslash, remove the
    backslash and join the next line to it, inserting a space between them.
    When a line that is the result of such a join ends in a backslash,
    proceed likewise.'''
    outerpos = 0
    match = pattern.search(text, outerpos)
    while match:
        (startpos, pos) = match.span()
        # Look how far the continuation lines extend.
        pos = text.find('\n',pos)
        while pos > 0 and text[pos-1] == '\\':
            pos = text.find('\n',pos+1)
        if pos < 0:
            pos = len(text)
        # Perform a combine_lines throughout the continuation lines.
        partdone = text[:startpos] + combine_lines(text[startpos:pos])
        outerpos = len(partdone)
        text = partdone + text[pos:]
        # Next round.
        match = pattern.search(text, outerpos)
    return text


__all__ += ['APP', 'DIRS', 'MODES', 'UTILS']
