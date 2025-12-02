#!/usr/bin/python
# encoding: UTF-8

'''An easy access to pygnulib classes.'''

#===============================================================================
# Define global imports
#===============================================================================
__all__ = list()

try:
    # Constants
    from . import constants

    # Main classes
    from .GLConfig import GLConfig
    from .GLError import GLError
    from .GLInfo import GLInfo

    # File system
    from .GLFileSystem import GLFileSystem
    from .GLFileSystem import GLFileAssistant

    # Module system
    from .GLModuleSystem import GLModule
    from .GLModuleSystem import GLModuleSystem
    from .GLModuleSystem import GLModuleTable

    # Different modes
    from .GLImport import GLImport
    from .GLEmiter import GLEmiter
    from .GLTestDir import GLTestDir
    from .GLTestDir import GLMegaTestDir

    # Other modules
    from .GLMakefileTable import GLMakefileTable
except ValueError as error:
    # Constants
    import constants

    # Main classes
    from GLConfig import GLConfig
    from GLError import GLError
    from GLInfo import GLInfo

    # File system
    from GLFileSystem import GLFileSystem
    from GLFileSystem import GLFileAssistant

    # Module system
    from GLModuleSystem import GLModule
    from GLModuleSystem import GLModuleSystem
    from GLModuleSystem import GLModuleTable

    # Different modes
    from GLImport import GLImport
    from GLEmiter import GLEmiter
    from GLTestDir import GLTestDir
    from GLTestDir import GLMegaTestDir

    # Other modules
    from GLMakefileTable import GLMakefileTable

# Append modules to namespace.
__all__ += ['GLConfig', 'GLError', 'GLInfo']
__all__ += ['GLFileSystem', 'GLFileAssistant']
__all__ += ['GLModule', 'GLModuleSystem', 'GLModuleTable']
__all__ += ['GLImport', 'GLEmiter', 'GLTestDir']
__all__ += ['GLMakefileTable']

#===============================================================================
# Define module information
#===============================================================================
__author__ = constants.__author__
__license__ = constants.__license__
__copyright__ = constants.__copyright__
