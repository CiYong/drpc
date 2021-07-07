# CMake build script for drpc
project(drpc)

cmake_minimum_required(VERSION 2.8.12)

option(BUILD_SHARED "Whether or not to build the shared object" ON)
option(BUILD_STATIC "Whether or not to build the static archive" ON)
option(WITH_DOCS "Build html docs" OFF)
option(ENABLE_ANALYSIS "Build with static analysis(make take very long)" OFF)

include(CheckIncludeFiles)
include(CheckCCompilerFlag)
include(CheckCXXCompilerFlag)
include(CheckLibraryExists)
include(CheckCSourceCompiles)
include(CheckCSourceRuns)
include(CMakeDependentOption)
include(CheckCXXSymbolExists)
include(CheckTypeSize)
include(FindThreads)
include(GNUInstallDirs)
include(CheckTypeSize)
include(CMakePackageConfigHelpers)

find_package(PkgConfig)
find_package(Threads)

set(sources)
set(cxx-sources)
set(html-docs)
set(target_outputs)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/core/cpp/include)

set(public_headers ${CMAKE_CURRENT_SOURCE_DIR}/src/core/cpp/include/drpc.hpp)

set(DRPC_OUTPUT_BASENAME
    "drpc"
    CACHE STRING "Output drpc library base name")

set(prefix ${CMAKE_INSTALL_PREFIX})
set(exec_prefix ${prefix})
set(libdir ${prefix}/lib)
set(includedir ${prefix}/include)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/version.rc.in ${CMAKE_CURRENT_BINARY_DIR}/version.rc)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/libdrpc.pc.in ${CMAKE_CURRENT_BINARY_DIR}/libdrpc.pc @ONLY)
set(drpc-pkgconfig ${CMAKE_CURRENT_BINARY_DIR}/libdrpc.pc)

set(SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

if(WIN32)
  if(CMAKE_SYSTEM_NAME STREQUAL "WindowsStore" AND CMAKE_SYSTEM_VERSION MATCHES "^10.0")
    set(DRPC_HAVE_WINDOWS_UWP ON)
    # to remove compile warninging "D9002 ignoring unknown option"
    string(REPLACE "/Zi" "" CMAKE_CXX_FLAGS_DEBUG ${CMAKE_CXX_FLAGS_DEBUG})
    set(CMAKE_CXX_FLAGS_DEBUG
        ${CMAKE_CXX_FLAGS_DEBUG}
        CACHE STRING "" FORCE)
    string(REPLACE "/Zi" "" CMAKE_CXX_FLAGS_RELWITHDEBINFO ${CMAKE_CXX_FLAGS_RELWITHDEBINFO})
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO
        ${CMAKE_CXX_FLAGS_RELWITHDEBINFO}
        CACHE STRING "" FORCE)
    string(REPLACE "/Zi" "" CMAKE_CXX_FLAGS_DEBUG ${CMAKE_CXX_FLAGS_DEBUG})
  endif()
  # from https://stackoverflow.com/a/40217291/2019765
  macro(get_WIN32_WINNT version)
    if(CMAKE_SYSTEM_VERSION)
      set(ver ${CMAKE_SYSTEM_VERSION})
      string(REGEX MATCH "^([0-9]+).([0-9])" ver ${ver})
      string(REGEX MATCH "^([0-9]+)" verMajor ${ver})
      # Check for Windows 10, b/c we'll need to convert to hex 'A'.
      if("${verMajor}" MATCHES "10")
        set(verMajor "A")
        string(REGEX REPLACE "^([0-9]+)" ${verMajor} ver ${ver})
      endif("${verMajor}" MATCHES "10")
      # Remove all remaining '.' characters.
      string(REPLACE "." "" ver ${ver})
      # Prepend each digit with a zero.
      string(REGEX REPLACE "([0-9A-Z])" "0\\1" ver ${ver})
      set(${version} "0x${ver}")
    endif(CMAKE_SYSTEM_VERSION)
  endmacro(get_WIN32_WINNT)

  get_win32_winnt(DRPC_WIN32_WINNT_DEFAULT)
  message(STATUS "Detected _WIN32_WINNT from CMAKE_SYSTEM_VERSION: ${DRPC_WIN32_WINNT_DEFAULT}")

  # TODO limit _WIN32_WINNT to the actual Windows SDK version, which might be different from the default version
  # installed with Visual Studio
  if(MSVC_VERSION STREQUAL "1500" AND CMAKE_SYSTEM_VERSION VERSION_GREATER "6.0")
    set(DRPC_WIN32_WINNT_LIMIT "0x0600")
  elseif(MSVC_VERSION STREQUAL "1600" AND CMAKE_SYSTEM_VERSION VERSION_GREATER "6.1")
    set(DRPC_WIN32_WINNT_LIMIT "0x0601")
  elseif(MSVC_VERSION STREQUAL "1700" AND CMAKE_SYSTEM_VERSION VERSION_GREATER "6.1")
    set(DRPC_WIN32_WINNT_LIMIT "0x0601")
  elseif(MSVC_VERSION STREQUAL "1800" AND CMAKE_SYSTEM_VERSION VERSION_GREATER "6.2")
    set(DRPC_WIN32_WINNT_LIMIT "0x0602")
  endif()
  if(DRPC_WIN32_WINNT_LIMIT)
    message(
      STATUS
        "Mismatch of Visual Studio Version (${MSVC_VERSION}) and CMAKE_SYSTEM_VERSION (${CMAKE_SYSTEM_VERSION}), limiting _WIN32_WINNT to ${DRPC_WIN32_WINNT_LIMIT}, you may override this by setting DRPC_WIN32_WINNT"
    )
    set(DRPC_WIN32_WINNT_DEFAULT "${DRPC_WIN32_WINNT_LIMIT}")
  endif()

  set(DRPC_WIN32_WINNT
      "${DRPC_WIN32_WINNT_DEFAULT}"
      CACHE STRING "Value to set _WIN32_WINNT to for building [default=autodetect from build environment]")

  add_definitions(-D_WIN32_WINNT=${DRPC_WIN32_WINNT})
endif(WIN32)


# -----------------------------------------------------------------------------
# default to Release build

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  # CMAKE_BUILD_TYPE is not used for multi-configuration generators like Visual Studio/XCode which instead use
  # CMAKE_CONFIGURATION_TYPES
  set(CMAKE_BUILD_TYPE
      Release
      CACHE STRING "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel." FORCE)
endif()

# -----------------------------------------------------------------------------
# output directories

drpc_set_with_default(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${drpc_BINARY_DIR}/bin")
if(UNIX)
  set(drpc_library_directory "lib")
else()
  set(drpc_library_directory "bin")
endif()

drpc_set_with_default(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${drpc_BINARY_DIR}/${drpc_library_directory}")
drpc_set_with_default(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${drpc_BINARY_DIR}/lib")

set(cxx-sources
    drpc.cpp
    channel.cpp
    handler.cpp)

foreach(source ${cxx-sources})
  list(APPEND sources ${CMAKE_CURRENT_SOURCE_DIR}/src/core/cpp/src/${source})
endforeach()

# check c++ version.
if (NOT MSVC)
  if(NOT CMAKE_CXX_FLAGS MATCHES "-std=")
    # use C++11 by default if supported
    check_cxx_compiler_flag("-std=gnu++11" COMPILER_SUPPORTS_CXX11)
    if(COMPILER_SUPPORTS_CXX11)
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11")
    endif()
  endif()
  if(NOT CMAKE_C_FLAGS MATCHES "-std=")
    check_c_compiler_flag("-std=gnu11" COMPILER_SUPPORTS_C11)
    if(COMPILER_SUPPORTS_C11)
      set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu11")
    else()
      set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99")
    endif()
  endif()
endif()


# -----------------------------------------------------------------------------
# output

if(MSVC)
  # Suppress linker warnings caused by #ifdef omission of file content.
  set(CMAKE_STATIC_LINKER_FLAGS "${CMAKE_STATIC_LINKER_FLAGS} /ignore:4221")
  set(PDB_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/bin")
  set(PDB_NAME
      "lib${DRPC_OUTPUT_BASENAME}${MSVC_TOOLSET}-mt-gd-${DRPC_VERSION_MAJOR}_${DRPC_VERSION_MINOR}_${DRPC_VERSION_PATCH}")
  function(enable_vs_guideline_checker target)
    set_target_properties(
      ${target} PROPERTIES VS_GLOBAL_EnableCppCoreCheck true VS_GLOBAL_CodeAnalysisRuleSet CppCoreCheckRules.ruleset
                           VS_GLOBAL_RunCodeAnalysis true)
  endfunction()
  if(BUILD_SHARED)
    add_library(libdrpc SHARED ${sources} ${public_headers} ${html-docs} ${readme-docs}
                              ${CMAKE_CURRENT_BINARY_DIR}/NSIS.template.in ${CMAKE_CURRENT_BINARY_DIR}/version.rc)
    if(ENABLE_ANALYSIS)
      enable_vs_guideline_checker(libdrpc)
    endif()
    set_target_properties(
      libdrpc
      PROPERTIES PUBLIC_HEADER "${public_headers}"
                 RELEASE_POSTFIX "${MSVC_TOOLSET}-mt-${DRPC_VERSION_MAJOR}_${DRPC_VERSION_MINOR}_${DRPC_VERSION_PATCH}"
                 RELWITHDEBINFO_POSTFIX
                 "${MSVC_TOOLSET}-mt-${DRPC_VERSION_MAJOR}_${DRPC_VERSION_MINOR}_${DRPC_VERSION_PATCH}"
                 MINSIZEREL_POSTFIX "${MSVC_TOOLSET}-mt-${DRPC_VERSION_MAJOR}_${DRPC_VERSION_MINOR}_${DRPC_VERSION_PATCH}"
                 DEBUG_POSTFIX "${MSVC_TOOLSET}-mt-gd-${DRPC_VERSION_MAJOR}_${DRPC_VERSION_MINOR}_${DRPC_VERSION_PATCH}"
                 RUNTIME_OUTPUT_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
                 COMPILE_DEFINITIONS "DLL_EXPORT"
                 OUTPUT_NAME "lib${DRPC_OUTPUT_BASENAME}")
    if(DRPC_HAVE_WINDOWS_UWP)
      set_target_properties(libdrpc PROPERTIES LINK_FLAGS_DEBUG "/OPT:NOICF /OPT:NOREF")
    endif()
  endif()

  if(BUILD_STATIC)
    add_library(libdrpc-static STATIC ${sources} ${CMAKE_CURRENT_BINARY_DIR}/version.rc)
    set_target_properties(
      libdrpc-static
      PROPERTIES PUBLIC_HEADER "${public_headers}"
                 RELEASE_POSTFIX "${MSVC_TOOLSET}-mt-s-${DRPC_VERSION_MAJOR}_${DRPC_VERSION_MINOR}_${DRPC_VERSION_PATCH}"
                 RELWITHDEBINFO_POSTFIX
                 "${MSVC_TOOLSET}-mt-s-${DRPC_VERSION_MAJOR}_${DRPC_VERSION_MINOR}_${DRPC_VERSION_PATCH}"
                 MINSIZEREL_POSTFIX
                 "${MSVC_TOOLSET}-mt-s-${DRPC_VERSION_MAJOR}_${DRPC_VERSION_MINOR}_${DRPC_VERSION_PATCH}"
                 DEBUG_POSTFIX "${MSVC_TOOLSET}-mt-sgd-${DRPC_VERSION_MAJOR}_${DRPC_VERSION_MINOR}_${DRPC_VERSION_PATCH}"
                 COMPILE_FLAGS "/DDRPC_STATIC"
                 OUTPUT_NAME "lib${DRPC_OUTPUT_BASENAME}")
  endif()
else()
  # avoid building everything twice for shared + static only on *nix, as Windows needs different preprocessor defines in
  # static builds
  if(NOT MINGW)
    add_library(objects OBJECT ${sources})
    set_property(TARGET objects PROPERTY POSITION_INDEPENDENT_CODE ON)
    target_include_directories(
      objects PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
                     $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}> $<INSTALL_INTERFACE:include>)
  endif()

  if(BUILD_SHARED)
    if(MINGW)
      add_library(libdrpc SHARED ${sources} ${public_headers} ${html-docs} ${readme-docs} ${drpc-pkgconfig}
                                ${CMAKE_CURRENT_BINARY_DIR}/version.rc)
    else()
        if (CMAKE_GENERATOR STREQUAL "Xcode")
           add_library(libdrpc SHARED ${sources} ${public_headers} ${html-docs} ${readme-docs}
                                     ${drpc-pkgconfig} ${CMAKE_CURRENT_BINARY_DIR}/version.rc)
        else()
           add_library(libdrpc SHARED $<TARGET_OBJECTS:objects> ${public_headers} ${html-docs} ${readme-docs}
                                     ${drpc-pkgconfig} ${CMAKE_CURRENT_BINARY_DIR}/version.rc)
        endif()
    endif()
    set_target_properties(
      libdrpc PROPERTIES COMPILE_DEFINITIONS "DLL_EXPORT" PUBLIC_HEADER "${public_headers}" VERSION "0.1.0"
                        SOVERSION "5" OUTPUT_NAME "${DRPC_OUTPUT_BASENAME}" PREFIX "lib")
  endif()

  if(BUILD_STATIC)
    if(MINGW)
      add_library(libdrpc-static STATIC ${sources} ${public_headers} ${html-docs} ${readme-docs} ${drpc-pkgconfig}
                                       ${CMAKE_CURRENT_BINARY_DIR}/version.rc)
    else()
      if (CMAKE_GENERATOR STREQUAL "Xcode")
        add_library(libdrpc-static STATIC ${sources} ${public_headers} ${html-docs} ${readme-docs}
                                         ${drpc-pkgconfig} ${CMAKE_CURRENT_BINARY_DIR}/version.rc)
      else()
        add_library(libdrpc-static STATIC $<TARGET_OBJECTS:objects> ${public_headers} ${html-docs} ${readme-docs}
                                         ${drpc-pkgconfig} ${CMAKE_CURRENT_BINARY_DIR}/version.rc)
      endif()
    endif()
    if(CMAKE_SYSTEM_NAME MATCHES "QNX")
      target_link_libraries(libdrpc-static m)
    endif()
    set_target_properties(
      libdrpc-static PROPERTIES PUBLIC_HEADER "${public_headers}" OUTPUT_NAME "${DRPC_OUTPUT_BASENAME}" PREFIX "lib")
  endif()
endif()

# -----------------------------------------------------------------------------
# targets outputs

list(APPEND target_outputs "")

if(BUILD_SHARED)
  list(APPEND target_outputs "libdrpc")
endif()

if(BUILD_STATIC)
  list(APPEND target_outputs "libdrpc-static")
endif()

# -----------------------------------------------------------------------------
# collect include_directories

foreach(target ${target_outputs})
  target_include_directories(
    ${target} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
                     $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}> $<INSTALL_INTERFACE:include>)
endforeach()

# -----------------------------------------------------------------------------
# link options

if(BUILD_SHARED)
  target_link_libraries(libdrpc ${CMAKE_THREAD_LIBS_INIT})
  if(GNUTLS_FOUND)
    target_link_libraries(libdrpc ${GNUTLS_LIBRARIES})
  endif()
endif()

# -----------------------------------------------------------------------------
# install directories

if(MSVC AND (BUILD_SHARED OR BUILD_STATIC))
  install(
    TARGETS ${target_outputs}
    EXPORT ${PROJECT_NAME}-targets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR} COMPONENT SDK)
  if(MSVC_IDE)
    install(
      FILES ${PDB_OUTPUT_DIRECTORY}/\${CMAKE_INSTALL_CONFIG_NAME}/${PDB_NAME}.pdb
      DESTINATION ${CMAKE_INSTALL_BINDIR}
      COMPONENT SDK
      OPTIONAL)
  else()
    install(
      FILES ${PDB_OUTPUT_DIRECTORY}/${PDB_NAME}.pdb
      DESTINATION ${CMAKE_INSTALL_BINDIR}
      COMPONENT SDK
      OPTIONAL)
  endif()
  if(BUILD_SHARED)
    install(
      TARGETS libdrpc
      RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
      PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR} COMPONENT Runtime)
  endif()
elseif(BUILD_SHARED OR BUILD_STATIC)
  install(
    TARGETS ${target_outputs}
    EXPORT ${PROJECT_NAME}-targets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    FRAMEWORK DESTINATION "Library/Frameworks"
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
endif()

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/libdrpc.pc DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)


