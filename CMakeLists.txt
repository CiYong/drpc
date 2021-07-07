# CMake build script for drpc
project(drpc)

cmake_minimum_required(VERSION 2.8.12)

option(BUILD_SHARED "Whether or not to build the shared object" ON)
option(BUILD_STATIC "Whether or not to build the static archive" ON)
option(WITH_DOCS "Build html docs" OFF)

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

set(cxx-sources
    drpc.cpp
    channel.cpp
    handler.cpp)

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


# ---------------------------------------------
# output

if(MSVC)

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

# targets outputs
list(APPEND target_outputs "")

if(BUILD_SHARED)
  list(APPEND target_outputs "libdrpc")
endif()

if(BUILD_STATIC)
  list(APPEND target_outputs "libdrpc-static")
endif()

# collect include_directories
foreach(target ${target_outputs})
  target_include_directories(
    ${target} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
                     $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}> $<INSTALL_INTERFACE:include>)
endforeach()


# link options
if(BUILD_SHARED)
  target_link_libraries(libdrpc ${CMAKE_THREAD_LIBS_INIT})
  if(GNUTLS_FOUND)
    target_link_libraries(libdrpc ${GNUTLS_LIBRARIES})
  endif()
endif()


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


