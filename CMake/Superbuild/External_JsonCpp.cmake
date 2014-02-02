##############################################################################
#
# Library:   TubeTK
#
# Copyright 2010 Kitware Inc. 28 Corporate Drive,
# Clifton Park, NY, 12065, USA.
#
# All rights reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
##############################################################################

# Make sure this file is included only once.
get_filename_component( CMAKE_CURRENT_LIST_FILENAME ${CMAKE_CURRENT_LIST_FILE}
  NAME_WE )
if( ${CMAKE_CURRENT_LIST_FILENAME}_FILE_INCLUDED )
  return()
endif( ${CMAKE_CURRENT_LIST_FILENAME}_FILE_INCLUDED )
set( ${CMAKE_CURRENT_LIST_FILENAME}_FILE_INCLUDED 1 )

set( proj JsonCpp )

# Sanity checks.
if( DEFINED ${proj}_DIR AND NOT EXISTS ${${proj}_DIR} )
  message( FATAL_ERROR
    "${proj}_DIR variable is defined as ${${proj}_DIR} which corresponds to a nonexistent directory" )
endif( DEFINED ${proj}_DIR AND NOT EXISTS ${${proj}_DIR} )

set( ${proj}_DEPENDENCIES "" )

if( UNIX )
  set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-strict-aliasing"
    CACHE STRING "Flags used by all build types." FORCE )
  set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-strict-aliasing"
    CACHE STRING "Flags used by all build types." FORCE )
  if( ${CMAKE_SIZEOF_VOID_P} EQUAL 8 )
    set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC"
         CACHE STRING "Flags used by all build types." FORCE )
    set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC"
         CACHE STRING "Flags used by all build types." FORCE )
  endif( ${CMAKE_SIZEOF_VOID_P} EQUAL 8 )
endif( UNIX )

# Include dependent projects, if any.
TubeTKMacroCheckExternalProjectDependency( ${proj} )

if( NOT DEFINED ${proj}_DIR AND NOT ${USE_SYSTEM_JSONCPP} )
  set( ${proj}_SOURCE_DIR ${CMAKE_BINARY_DIR}/${proj} )
  set( ${proj}_DIR ${CMAKE_BINARY_DIR}/${proj}-build )

  ExternalProject_Add( ${proj}
    URL ${${proj}_URL}
    URL_MD5 ${${proj}_HASH_OR_TAG}
    DOWNLOAD_DIR ${${proj}_SOURCE_DIR}
    SOURCE_DIR ${${proj}_SOURCE_DIR}
    BINARY_DIR ${${proj}_DIR}
    INSTALL_DIR ${${proj}_DIR}
    LOG_DOWNLOAD 1
    LOG_UPDATE 0
    LOG_CONFIGURE 0
    LOG_BUILD 0
    LOG_TEST 0
    LOG_INSTALL 0
    CMAKE_GENERATOR ${gen}
    CMAKE_ARGS
      -DCMAKE_C_COMPILER:FILEPATH=${CMAKE_C_COMPILER}
      -DCMAKE_CXX_COMPILER:FILEPATH=${CMAKE_CXX_COMPILER}
      -DCMAKE_CXX_FLAGS:STRING=${CMAKE_CXX_FLAGS}
      -DCMAKE_INSTALL_PREFIX:PATH=${${proj}_DIR}
      -DCMAKE_BUILD_TYPE:STRING=${build_type}
      ${CMAKE_OSX_EXTERNAL_PROJECT_ARGS}
      -DJSONCPP_LIB_BUILD_SHARED:BOOL=${shared}
      -DJSONCPP_WITH_TESTS:BOOL=OFF )

else( NOT DEFINED ${proj}_DIR AND NOT ${USE_SYSTEM_JSONCPP} )
  if( ${USE_SYSTEM_JSONCPP} )
    find_package( ${proj} REQUIRED )
  endif( ${USE_SYSTEM_JSONCPP} )

  TubeTKMacroEmptyExternalProject( ${proj} "${${proj}_DEPENDENCIES}" )
endif( NOT DEFINED ${proj}_DIR AND NOT ${USE_SYSTEM_JSONCPP} )

list( APPEND TubeTK_EXTERNAL_PROJECTS_ARGS -D${proj}_DIR:PATH=${${proj}_DIR} )
