cmake_minimum_required(VERSION 3.2)

if(POLICY CMP0048)
  cmake_policy(SET CMP0048 NEW)
endif()

if(LINUX)
  set(PYEXT_SUFFIX
      ".so"
      CACHE STRING "Suffix for Python extension modules")
  set(DYNLIB_SUFFIX
      ".so"
      CACHE STRING "Suffix for dynamic libraries")
elseif(APPLE)
  set(PYEXT_SUFFIX
      ".so"
      CACHE STRING "Suffix for Python extension modules")
  set(DYNLIB_SUFFIX
      ".dylib"
      CACHE STRING "Suffix for dynamic libraries")
elseif(WIN32)
  set(PYEXT_SUFFIX
      ".pyd"
      CACHE STRING "Suffix for Python extension modules" FORCE)
  set(DYNLIB_SUFFIX
      ".dll"
      CACHE STRING "Suffix for dynamic libraries")
else()
  message(FATAL_ERROR "Unsupported platform")
endif()

find_package(Git QUIET)

file(READ "${CMAKE_SOURCE_DIR}/retro/VERSION.txt" PROJECT_VERSION)
string(REGEX REPLACE "\n$" "" PROJECT_VERSION "${PROJECT_VERSION}")
set_property(
  DIRECTORY
  APPEND
  PROPERTY CMAKE_CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/VERSION")

project("Gym Retro" VERSION ${PROJECT_VERSION})
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_MACOSX_DEPLOYMENT_TARGET 10.7)
set(CMAKE_CXX_FLAGS
    "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-sign-compare -Wno-missing-field-initializers -fvisibility=hidden"
)

if(CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64" OR CMAKE_SYSTEM_PROCESSOR STREQUAL
                                               "AMD64")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mssse3")
endif()

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

option(BUILD_TESTS "Should tests be built" ON)
option(BUILD_UI "Should integration UI be built" OFF)
option(BUILD_LUAJIT "Should static LuaJIT be used instead of system Lua" ON)
option(BUILD_MANYLINUX "Should use static libraries compatible with manylinux1"
       OFF)

set(BUILD_PYTHON
    ON
    CACHE BOOL "Build Python module")
mark_as_advanced(BUILD_PYTHON)

if(BUILD_PYTHON)
  find_package(Python 3 COMPONENTS Interpreter Development.Module)
else()
  find_package(Python 3 COMPONENTS Interpreter)
endif()

if(WIN32 OR BUILD_MANYLINUX)
  set(CMAKE_FIND_LIBRARY_SUFFIXES .a ${CMAKE_FIND_LIBRARY_SUFFIXES})
endif()

find_package(ZLIB REQUIRED)
find_package(PkgConfig)

if(NOT BUILD_MANYLINUX)
  # CapnProto requires a newer kernel than manylinux1 provides
  find_package(CapnProto QUIET)
endif()

pkg_search_module(LIBZIP QUIET libzip)

if(NOT BUILD_LUAJIT)
  find_package(Lua 5.1 EXACT REQUIRED)
  set(LUA_INCLUDE_DIRS "${LUA_INCLUDE_DIR}")
else()
  set(LUA_INCLUDE_DIRS "${CMAKE_SOURCE_DIR}/third-party/luajit/src")

  if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(LUA_FLAGS ${LUA_FLAGS} CC=${CMAKE_C_COMPILER}
                  LDFLAGS=-mmacosx-version-min=10.7)
    set(LUA_CFLAGS -mmacosx-version-min=10.7)
  endif()

  if(CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64" OR CMAKE_SYSTEM_PROCESSOR
                                                 STREQUAL "AMD64")
    set(LUA_FLAGS ${LUA_FLAGS}
                  "CFLAGS=-fPIC -DLUAJIT_ENABLE_GC64 ${LUA_CFLAGS}")
  endif()

  if(CMAKE_CROSSCOMPILING)
    if(CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64" OR CMAKE_SYSTEM_PROCESSOR
                                                   STREQUAL "AMD64")
      set(BITS 64)
    else()
      set(BITS 32)
    endif()

    set(LUA_FLAGS ${LUA_FLAGS} "HOST_CC=gcc -m${BITS}" CROSS="${cross_prefix}"
                  TARGET_SYS="${CMAKE_SYSTEM_NAME}")
  endif()

  set(LUA_LIBRARY "${LUA_INCLUDE_DIRS}/libluajit.a")
  add_custom_command(
    OUTPUT "${LUA_LIBRARY}"
    COMMAND $(MAKE) ${LUA_FLAGS} libluajit.a
    WORKING_DIRECTORY "${LUA_INCLUDE_DIRS}")
  add_custom_target(
    clean-luajit
    COMMAND $(MAKE) clean
    WORKING_DIRECTORY "${LUA_INCLUDE_DIRS}")
endif()

if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
  set(STATIC_LDFLAGS "-static-libstdc++ -Wl,--exclude-libs,ALL")
endif()

set(PYLIB_DIRECTORY
    "${CMAKE_CURRENT_BINARY_DIR}"
    CACHE PATH "Build directory that contains retro module")
file(MAKE_DIRECTORY "${PYLIB_DIRECTORY}/retro/cores")

set(CORES)
set(COREINFO)
set(CORE_TARGETS)

function(add_core platform core_name)
  set(target_name ${core_name}_libretro${CMAKE_SHARED_LIBRARY_SUFFIX})
  get_filename_component(
    TARGET_PATH "${PYLIB_DIRECTORY}/retro/cores/${target_name}" ABSOLUTE)
  list(APPEND CORES "${TARGET_PATH}")
  list(APPEND COREINFO
       "${CMAKE_CURRENT_SOURCE_DIR}/retro/cores/${core_name}.json")
  set(CORES
      "${CORES}"
      PARENT_SCOPE)
  set(COREINFO
      "${COREINFO}"
      PARENT_SCOPE)
  set(subdir)

  if(EXISTS "cores/${platform}/Makefile.libretro")
    set(makefile Makefile.libretro)
  elseif(EXISTS "cores/${platform}/Makefile")
    set(makefile Makefile)
  elseif(EXISTS "cores/${platform}/libretro/Makefile")
    set(makefile Makefile)
    set(subdir libretro)
  else()
    message(FATAL_ERROR "Could not find Makefile.")
  endif()

  add_custom_command(
    OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/retro/cores/${core_name}-version"
    COMMAND ${CMAKE_COMMAND} -E touch
            "${CMAKE_CURRENT_SOURCE_DIR}/retro/cores/${core_name}-version")

  if(WIN32)
    set(libretro_platform platform=win)
  endif()

  set(core_ldflags "-w ${STATIC_LDFLAGS}")
  set(core_cflags "-w")
  set(core_cxxflags "-w")

  if(WIN32 AND NOT CMAKE_CROSSCOMPILING)
    set(core_ldflags "${core_ldflags} ${STATIC_LDFLAGS}")
  endif()

  if(NOT WIN32)
    set(core_fpic_flags "-fPIC")
  endif()

  if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(core_fpic_flags
        "${core_fpic_flags} -mmacosx-version-min=10.7 -stdlib=libc++")
  endif()

  add_custom_command(
    OUTPUT "${TARGET_PATH}"
    COMMAND
      ${CMAKE_COMMAND} -E env CFLAGS=${core_cflags} CXXFLAGS=${core_cxxflags}
      LDFLAGS=${core_ldflags} $(MAKE) -f ${makefile} CC="${CMAKE_C_COMPILER}"
      CXX="${CMAKE_CXX_COMPILER}" fpic=${core_fpic_flags} ${libretro_platform}
    COMMAND ${CMAKE_COMMAND} -E copy "${core_name}_libretro${DYNLIB_SUFFIX}"
            "${TARGET_PATH}"
    WORKING_DIRECTORY "cores/${platform}/${subdir}"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/retro/cores/${core_name}-version")
  unset(core_ldflags)
  add_custom_command(
    OUTPUT "${PYLIB_DIRECTORY}/retro/cores/${core_name}.json"
    COMMAND
      ${CMAKE_COMMAND} -E copy
      "${CMAKE_CURRENT_SOURCE_DIR}/cores/${platform}.json"
      "${PYLIB_DIRECTORY}/retro/cores/${core_name}.json"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/retro/cores/${core_name}-version")

  add_custom_target(
    ${platform} ALL
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/retro/cores/${core_name}-version"
            "${PYLIB_DIRECTORY}/retro/cores/${core_name}.json" "${TARGET_PATH}")
  list(APPEND CORE_TARGETS "${platform}")
  set(CORE_TARGETS
      "${CORE_TARGETS}"
      PARENT_SCOPE)

  add_custom_target(
    clean-${platform}
    COMMAND $(MAKE) -f ${makefile} clean
    COMMAND ${CMAKE_COMMAND} -E remove "${TARGET_PATH}"
    WORKING_DIRECTORY "cores/${platform}/${subdir}")
endfunction()

add_core(snes snes9x)
add_core(genesis genesis_plus_gx)
add_core(nes fceumm)
add_core(atari2600 stella)

if(APPLE AND CMAKE_SYSTEM_PROCESSOR MATCHES "arm64")
  message(warning "Detected Apple Silicon, skipping gambatte (gb)")
else()
  add_core(gb gambatte)
endif()

add_core(gba mgba)
add_core(pce mednafen_pce_fast)
add_core(32x picodrive)
add_core(saturn mednafen_saturn)

set(CLEAN_CORES)

foreach(core IN LISTS CORE_TARGETS)
  list(APPEND CLEAN_CORES "clean-${core}")
endforeach()

add_custom_target(clean-cores DEPENDS ${CLEAN_CORES})
add_custom_target(cores DEPENDS ${CORE_TARGETS})
unset(CLEAN_CORES)

if(CMAKE_CROSSCOMPILING)
  find_program(CAPNP_EXECUTABLE capnp)
  find_program(CAPNPC_CXX_EXECUTABLE capnpc-c++)
endif()

if(NOT CAPN_PROTO_FOUND AND NOT BUILD_MANYLINUX)
  if(NOT CMAKE_CROSSCOMPILING OR CAPNP_EXECUTABLE)
    set(CAPN_PROTO_FOUND ON)
    set(BUILD_TESTING
        OFF
        CACHE BOOL "" FORCE)

    if(CAPNP_EXECUTABLE)
      set(CAPNP_LITE ON)
    endif()

    set(CAPNP_INCLUDE_DIRECTORY third-party/capnproto/c++/src)
    add_subdirectory(third-party/capnproto/c++)
  endif()
endif()

set(CAPN_PROTO_FOUND OFF)

if(CAPN_PROTO_FOUND)
  add_definitions(-DUSE_CAPNP)

  if(CAPNP_LIB_CAPNP)
    add_library(CapnProto::capnp SHARED IMPORTED)
    add_library(CapnProto::kj SHARED IMPORTED)
    set_target_properties(CapnProto::kj PROPERTIES IMPORTED_LOCATION
                                                   "${CAPNP_LIB_KJ}")
    set_target_properties(
      CapnProto::capnp PROPERTIES IMPORTED_LOCATION "${CAPNP_LIB_CAPNP}"
                                  INTERFACE_LINK_LIBRARIES CapnProto::kj)
  endif()

  if(CMAKE_CROSSCOMPILING)
    if(TARGET CapnProto::capnp_tool)
      set_target_properties(CapnProto::capnp_tool
                            PROPERTIES IMPORTED_LOCATION "${CAPNP_EXECUTABLE}")
    endif()

    if(TARGET CapnProto::capnpc_cpp)
      set_target_properties(
        CapnProto::capnpc_cpp PROPERTIES IMPORTED_LOCATION
                                         "${CAPNPC_CXX_EXECUTABLE}")
    endif()
  endif()
else()
  message(WARNING "Could not find CapnProto, disabling search save/load")
endif()

set(BUILD_SHARED_LIBS
    OFF
    CACHE BOOL "" FORCE)
set(BUILD_STATIC_LIBS
    ON
    CACHE BOOL "" FORCE)

if(LIBZIP_FOUND AND NOT LIBZIP_VERSION VERSION_LESS 1.0.0)
  include_directories(${LIBZIP_INCLUDE_DIRS})
  link_directories(${LIBZIP_LIBRARY_DIRS})
else()
  set(LIBZIP_LIBRARIES zip)
  add_subdirectory(third-party/libzip)
  include_directories(third-party/libzip third-party/libzip/lib)
endif()

include_directories(${LUA_INCLUDE_DIRS})

add_library(
  retro-base STATIC
  src/coreinfo.cpp
  src/data.cpp
  src/emulator.cpp
  src/imageops.cpp
  src/memory.cpp
  src/movie.cpp
  src/movie-bk2.cpp
  src/movie-fm2.cpp
  src/script.cpp
  src/script-lua.cpp
  src/search.cpp
  src/utils.cpp
  src/zipfile.cpp
  ${LUA_LIBRARY})
target_link_libraries(retro-base ${ZLIB_LIBRARY} ${LIBZIP_LIBRARIES}
                      ${LUA_LIBRARY} ${LUA_LIBRRAY})
add_dependencies(retro-base ${CORE_TARGETS})

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  target_link_libraries(retro-base dl)
endif()

if(CAPN_PROTO_FOUND)
  capnp_generate_cpp(CAPNP_SRC CAPNP_H src/serialize.capnp)
  add_library(retro-capnp STATIC ${CAPNP_SRC} src/data-capnp.cpp)
  target_link_libraries(retro-capnp retro-base CapnProto::capnp)
endif()

include_directories(src retro third-party/pybind11/include third-party
                    third-party/gtest/googletest/include ${Python_INCLUDE_DIRS})

if(BUILD_PYTHON)
  add_library(retro SHARED src/retro.cpp)
  set_target_properties(
    retro
    PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${PYLIB_DIRECTORY}/retro"
               RUNTIME_OUTPUT_DIRECTORY "${PYLIB_DIRECTORY}/retro"
               OUTPUT_NAME _retro
               PREFIX ""
               SUFFIX ${PYEXT_SUFFIX})

  if(APPLE)
    set(PYBIND_LIBS "-undefined dynamic_lookup")
  elseif(WIN32)
    if(CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64" OR CMAKE_SYSTEM_PROCESSOR
                                                   STREQUAL "AMD64")
      add_definitions(-DMS_WIN64)
    endif()

    set(PYBIND_LIBS "${Python_LIBRARY}")
  endif()

  target_link_libraries(retro retro-base ${PYBIND_LIBS} ${STATIC_LDFLAGS})
endif()

if(BUILD_TESTS)
  enable_testing()
  add_subdirectory(third-party/gtest/googlemock)
  add_subdirectory(tests)
endif()

if(BUILD_UI)
  add_subdirectory(src/ui)
endif()

execute_process(
  COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_SOURCE_DIR}/setup.py --version
  OUTPUT_VARIABLE CPACK_PACKAGE_VERSION
  OUTPUT_STRIP_TRAILING_WHITESPACE)
set(CPACK_PACKAGE_VENDOR OpenAI)
set(CPACK_ZIP_COMPONENT_INSTALL ON)

if(APPLE)
  set(CPACK_GENERATOR DragNDrop)
elseif(WIN32)
  set(CPACK_GENERATOR ZIP)
endif()

set(CPACK_COMPONENTS_ALL gym-retro-integration)
include(CPack)
